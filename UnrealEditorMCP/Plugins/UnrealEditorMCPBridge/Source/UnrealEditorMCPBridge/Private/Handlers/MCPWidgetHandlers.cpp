#include "Handlers/MCPWidgetHandlers.h"
#include "MCPBridgeHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "WidgetBlueprint.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPWidget, Log, All);

// 递归遍历 Widget 树
static void CollectWidgetTree(const UWidget* Widget, TArray<TSharedPtr<FJsonValue>>& OutArray, int32 Depth = 0)
{
	if (!Widget) return;
	TSharedPtr<FJsonObject> WObj = MakeShareable(new FJsonObject());
	WObj->SetStringField(TEXT("name"), Widget->GetName());
	WObj->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
	WObj->SetNumberField(TEXT("depth"), Depth);

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		TArray<TSharedPtr<FJsonValue>> Children;
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			CollectWidgetTree(Panel->GetChildAt(i), Children, Depth + 1);
		}
		if (Children.Num() > 0)
			WObj->SetArrayField(TEXT("children"), Children);
	}

	OutArray.Add(MakeShareable(new FJsonValueObject(WObj)));
}

// ====== ListWidgets ======
bool FMCPListWidgetsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString SearchPath = TEXT("/Game");
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("path"), SearchPath);

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	FARFilter Filter;
	Filter.PackagePaths.Add(*SearchPath);
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	OutResult = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> WidgetsArray;
	for (const FAssetData& Asset : AssetList)
	{
		TSharedPtr<FJsonObject> WObj = MakeShareable(new FJsonObject());
		WObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		WObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		WObj->SetStringField(TEXT("package"), Asset.PackageName.ToString());
		WidgetsArray.Add(MakeShareable(new FJsonValueObject(WObj)));
	}
	OutResult->SetArrayField(TEXT("widgets"), WidgetsArray);
	OutResult->SetNumberField(TEXT("count"), WidgetsArray.Num());
	OutResult->SetStringField(TEXT("path"), SearchPath);
	return true;
}

// ====== GetWidgetInfo ======
bool FMCPGetWidgetInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget Blueprint not found: '%s'"), *AssetPath), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("name"), WidgetBP->GetName());
	OutResult->SetStringField(TEXT("path"), WidgetBP->GetPathName());
	if (WidgetBP->ParentClass)
		OutResult->SetStringField(TEXT("parent_class"), WidgetBP->ParentClass->GetName());

	// Widget 树
	TArray<TSharedPtr<FJsonValue>> TreeArray;
	if (WidgetBP->WidgetTree)
	{
		CollectWidgetTree(WidgetBP->WidgetTree->RootWidget, TreeArray);
	}
	OutResult->SetArrayField(TEXT("widget_tree"), TreeArray);

	return true;
}

// ====== CreateWidgetBlueprint ======
bool FMCPCreateWidgetBlueprintHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString WidgetName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("name"), WidgetName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString PackagePath = TEXT("/Game/UI");
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("path"), PackagePath);

	FString PackageName = PackagePath / WidgetName;

	// 冲突检测：通过 AssetRegistry
	{
		FString ObjectPath = PackageName + TEXT(".") + WidgetName;
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		FAssetData Existing = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath));
		if (Existing.IsValid())
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_CONFLICT"), FString::Printf(TEXT("Asset already exists: '%s'"), *ObjectPath), OutErrorCode, OutErrorMessage);
			return false;
		}
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to create package"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
		FKismetEditorUtilities::CreateBlueprint(UUserWidget::StaticClass(), Package, *WidgetName,
			BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));

	if (!WidgetBP)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to create Widget Blueprint"), OutErrorCode, OutErrorMessage);
		return false;
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(WidgetBP);

	FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, WidgetBP, *FilePath, SaveArgs);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("name"), WidgetBP->GetName());
	OutResult->SetStringField(TEXT("path"), WidgetBP->GetPathName());
	OutResult->SetBoolField(TEXT("created"), true);
	OutResult->SetBoolField(TEXT("saved"), bSaved);

	if (!bSaved)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SAVE_FAILED"), TEXT("Widget Blueprint created but failed to save to disk"), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}

// ====== AddChild（新增：向 Widget 树添加控件）======
bool FMCPWidgetAddChildHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetClass;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("widget_class"), WidgetClass))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and widget_class required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UClass* WidgetClassObj = FindFirstObject<UClass>(*WidgetClass, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!WidgetClassObj)
	{
		FString WithPrefix = TEXT("U") + WidgetClass;
		WidgetClassObj = FindFirstObject<UClass>(*WithPrefix, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	}
	if (!WidgetClassObj || !WidgetClassObj->IsChildOf(UWidget::StaticClass()))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), FString::Printf(TEXT("Widget class '%s' not found"), *WidgetClass), OutErrorCode, OutErrorMessage); return false; }

	UPanelWidget* ParentPanel = nullptr;
	FString ParentName;
	if (Payload->TryGetStringField(TEXT("parent_widget"), ParentName))
	{
		UWidget* ParentWidget = WidgetBP->WidgetTree->FindWidget(FName(*ParentName));
		ParentPanel = Cast<UPanelWidget>(ParentWidget);
		if (!ParentPanel)
		{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PARENT_NOT_FOUND"), FString::Printf(TEXT("Panel parent '%s' not found"), *ParentName), OutErrorCode, OutErrorMessage); return false; }
	}
	else
	{
		ParentPanel = Cast<UPanelWidget>(WidgetBP->WidgetTree->RootWidget);
		if (!ParentPanel)
		{ MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_PANEL"), TEXT("Root widget is not a panel; specify parent_widget"), OutErrorCode, OutErrorMessage); return false; }
	}

	FString ChildName;
	Payload->TryGetStringField(TEXT("widget_name"), ChildName);
	if (ChildName.IsEmpty()) ChildName = WidgetClass + TEXT("_MCP");

	WidgetBP->Modify();
	UWidget* NewWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WidgetClassObj, FName(*ChildName));
	if (!NewWidget)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to construct widget"), OutErrorCode, OutErrorMessage); return false; }

	ParentPanel->AddChild(NewWidget);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("blueprint"), WidgetBP->GetName());
	OutResult->SetStringField(TEXT("widget_name"), NewWidget->GetName());
	OutResult->SetStringField(TEXT("widget_class"), WidgetClass);
	OutResult->SetStringField(TEXT("parent"), ParentPanel->GetName());
	OutResult->SetBoolField(TEXT("added"), true);
	return true;
}

// ====== RemoveChild（新增：从 Widget 树移除控件）======
bool FMCPWidgetRemoveChildHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("widget_name"), WidgetName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and widget_name required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	WidgetBP->Modify();
	Target->RemoveFromParent();
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("blueprint"), WidgetBP->GetName());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);
	OutResult->SetBoolField(TEXT("removed"), true);
	return true;
}

// ====== SetProperty（新增：设置 Widget 属性）======
bool FMCPWidgetSetPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName, PropName, Value;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("widget_name"), WidgetName)
		|| !Payload->TryGetStringField(TEXT("property_name"), PropName) || !Payload->TryGetStringField(TEXT("value"), Value))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, widget_name, property_name, value required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	FProperty* Property = Target->GetClass()->FindPropertyByName(*PropName);
	if (!Property)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Property '%s' not found"), *PropName), OutErrorCode, OutErrorMessage); return false; }

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Target);
	if (!ValuePtr)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_SET_FAILED"), TEXT("Failed to access property memory"), OutErrorCode, OutErrorMessage); return false; }

	WidgetBP->Modify();
	const TCHAR* Result = Property->ImportText_Direct(*Value, ValuePtr, Target, PPF_None);
	if (!Result)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_SET_FAILED"), TEXT("Failed to parse property value"), OutErrorCode, OutErrorMessage); return false; }

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	FString ReadBack;
	Property->ExportTextItem_Direct(ReadBack, ValuePtr, nullptr, Target, PPF_None);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("blueprint"), WidgetBP->GetName());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);
	OutResult->SetStringField(TEXT("property_name"), PropName);
	OutResult->SetStringField(TEXT("value"), ReadBack);
	OutResult->SetBoolField(TEXT("set"), true);
	return true;
}
