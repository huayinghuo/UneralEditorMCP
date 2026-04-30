#include "Handlers/MCPWidgetHandlers.h"
#include "MCPBridgeHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/UserWidget.h"
#include "WidgetBlueprint.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "UObject/SavePackage.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Components/Overlay.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/GridPanel.h"
#include "Components/UniformGridPanel.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPWidget, Log, All);

// 递归遍历 Widget 树（增强版：含 parent / slot_class / is_variable）
static void CollectWidgetTree(const UWidget* Widget, TArray<TSharedPtr<FJsonValue>>& OutArray, int32 Depth = 0, FString ParentName = TEXT(""))
{
	if (!Widget) return;
	TSharedPtr<FJsonObject> WObj = MakeShareable(new FJsonObject());
	WObj->SetStringField(TEXT("name"), Widget->GetName());
	WObj->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
	WObj->SetNumberField(TEXT("depth"), Depth);
	WObj->SetStringField(TEXT("parent"), ParentName);
	WObj->SetBoolField(TEXT("is_variable"), Widget->bIsVariable);

	// Slot class for this widget (if parent is a panel)
	if (Widget->Slot)
	{
		WObj->SetStringField(TEXT("slot_class"), Widget->Slot->GetClass()->GetName());
	}

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		TArray<TSharedPtr<FJsonValue>> Children;
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			CollectWidgetTree(Panel->GetChildAt(i), Children, Depth + 1, Widget->GetName());
		}
		if (Children.Num() > 0)
			WObj->SetArrayField(TEXT("children"), Children);
	}

	OutArray.Add(MakeShareable(new FJsonValueObject(WObj)));
}

/** 递归搜索 Widget 树 */
static void FindWidgetsInTree(const UWidget* Widget, const FString& Query, TArray<TSharedPtr<FJsonValue>>& OutArray, int32 Depth = 0)
{
	if (!Widget) return;
	bool bMatch = Query.IsEmpty() ||
		Widget->GetName().Contains(Query) ||
		Widget->GetClass()->GetName().Contains(Query);

	if (bMatch)
	{
		TSharedPtr<FJsonObject> WObj = MakeShareable(new FJsonObject());
		WObj->SetStringField(TEXT("name"), Widget->GetName());
		WObj->SetStringField(TEXT("class"), Widget->GetClass()->GetName());
		WObj->SetNumberField(TEXT("depth"), Depth);
		WObj->SetBoolField(TEXT("is_variable"), Widget->bIsVariable);
		OutArray.Add(MakeShareable(new FJsonValueObject(WObj)));
	}

	if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
		{
			FindWidgetsInTree(Panel->GetChildAt(i), Query, OutArray, Depth + 1);
		}
	}
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

	// Root widget info
	if (WidgetBP->WidgetTree && WidgetBP->WidgetTree->RootWidget)
	{
		UWidget* Root = WidgetBP->WidgetTree->RootWidget;
		OutResult->SetStringField(TEXT("root_widget"), Root->GetName());
		OutResult->SetStringField(TEXT("root_widget_class"), Root->GetClass()->GetName());
	}

	// Widget 树（增强：含 parent / slot_class / is_variable）
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

	// Pre-validate root_widget_class (before creating asset, to avoid partial-failure side effects)
	UClass* RootClass = nullptr;
	FString RootWidgetClass;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("root_widget_class"), RootWidgetClass);
	if (!RootWidgetClass.IsEmpty())
	{
		RootClass = FindFirstObject<UClass>(*RootWidgetClass, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		if (!RootClass) RootClass = FindFirstObject<UClass>(*(FString(TEXT("U") + RootWidgetClass)), EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		if (!RootClass || !RootClass->IsChildOf(UWidget::StaticClass()))
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), FString::Printf(TEXT("Root widget class '%s' not found or not a widget"), *RootWidgetClass), OutErrorCode, OutErrorMessage);
			return false;
		}
	}

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

	// 可选 root widget 设定（class 已在 BP 创建前通过预校验）
	if (RootClass)
	{
		WidgetBP->Modify();
		WidgetBP->WidgetTree->RootWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(RootClass, FName(*RootWidgetClass));
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
		OutResult->SetStringField(TEXT("root_widget"), WidgetBP->WidgetTree->RootWidget->GetName());
		OutResult->SetStringField(TEXT("root_widget_class"), RootWidgetClass);

		// Re-save after root creation so root is persisted across reload
		bSaved = UPackage::SavePackage(Package, WidgetBP, *FilePath, SaveArgs);
		OutResult->SetBoolField(TEXT("saved"), bSaved);
	}

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

	// Support optional index for insertion position
	int32 InsertIndex = -1;
	Payload->TryGetNumberField(TEXT("index"), InsertIndex);
	if (InsertIndex >= 0)
	{
		ParentPanel->InsertChildAt(InsertIndex, NewWidget);
	}
	else
	{
		ParentPanel->AddChild(NewWidget);
	}
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

// ====== SetProperty（增强：typed value 支持）======
bool FMCPWidgetSetPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName, PropName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("widget_name"), WidgetName)
		|| !Payload->TryGetStringField(TEXT("property_name"), PropName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, widget_name, property_name required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// Typed value: prefer bool/number, fall back to string
	FString Value;
	if (!Payload->TryGetStringField(TEXT("value"), Value))
	{
		bool bBoolVal = false;
		double dNumVal = 0.0;
		if (Payload->TryGetBoolField(TEXT("value"), bBoolVal))
			Value = bBoolVal ? TEXT("true") : TEXT("false");
		else if (Payload->TryGetNumberField(TEXT("value"), dNumVal))
			Value = FString::Printf(TEXT("%f"), dNumVal);
		else
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.value is required"), OutErrorCode, OutErrorMessage);
			return false;
		}
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

// ====== Stage 16 Slice 1 — Query & Schema Handlers ======

// ====== GetPropertySchema（新增：查询 Widget 可编辑属性列表）======
bool FMCPWidgetGetPropertySchemaHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath)
		|| !Payload->TryGetStringField(TEXT("widget_name"), WidgetName))
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

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);
	OutResult->SetStringField(TEXT("widget_class"), Target->GetClass()->GetName());

	TArray<TSharedPtr<FJsonValue>> PropertiesArray;
	for (TFieldIterator<FProperty> It(Target->GetClass()); It; ++It)
	{
		FProperty* Prop = *It;
		TSharedPtr<FJsonObject> PropObj = MakeShareable(new FJsonObject());
		PropObj->SetStringField(TEXT("name"), Prop->GetName());
		PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());
		PropObj->SetBoolField(TEXT("editable"), Prop->HasAnyPropertyFlags(CPF_Edit));
		PropObj->SetBoolField(TEXT("read_only"), Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
		PropertiesArray.Add(MakeShareable(new FJsonValueObject(PropObj)));
	}
	OutResult->SetArrayField(TEXT("properties"), PropertiesArray);
	OutResult->SetNumberField(TEXT("count"), PropertiesArray.Num());

	return true;
}

// ====== GetSlotSchema（新增：查询 Widget Slot 属性列表）======
bool FMCPWidgetGetSlotSchemaHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath)
		|| !Payload->TryGetStringField(TEXT("widget_name"), WidgetName))
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

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);

	if (Target->Slot)
	{
		UClass* SlotClass = Target->Slot->GetClass();
		OutResult->SetStringField(TEXT("slot_class"), SlotClass->GetName());

		TArray<TSharedPtr<FJsonValue>> SlotProps;
		for (TFieldIterator<FProperty> It(SlotClass); It; ++It)
		{
			FProperty* Prop = *It;
			TSharedPtr<FJsonObject> PropObj = MakeShareable(new FJsonObject());
			PropObj->SetStringField(TEXT("name"), Prop->GetName());
			PropObj->SetStringField(TEXT("type"), Prop->GetCPPType());
			PropObj->SetBoolField(TEXT("editable"), Prop->HasAnyPropertyFlags(CPF_Edit));
			SlotProps.Add(MakeShareable(new FJsonValueObject(PropObj)));
		}
		OutResult->SetArrayField(TEXT("slot_properties"), SlotProps);
		OutResult->SetNumberField(TEXT("count"), SlotProps.Num());
	}
	else
	{
		OutResult->SetStringField(TEXT("slot_class"), TEXT("None"));
		OutResult->SetNumberField(TEXT("count"), 0);
	}

	return true;
}

// ====== WidgetFind（新增：搜索 Widget 树节点）======
bool FMCPWidgetFindHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	FString Query;
	Payload->TryGetStringField(TEXT("query"), Query);

	TArray<TSharedPtr<FJsonValue>> Results;
	FindWidgetsInTree(WidgetBP->WidgetTree->RootWidget, Query, Results);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetArrayField(TEXT("widgets"), Results);
	OutResult->SetNumberField(TEXT("count"), Results.Num());
	OutResult->SetStringField(TEXT("query"), Query);
	return true;
}

// ====== Stage 16 Slice 2 — Structure Editing ======

// ====== SetRoot ======
bool FMCPWidgetSetRootHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetClass;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("widget_class"), WidgetClass))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and widget_class required"), OutErrorCode, OutErrorMessage); return false; }

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UClass* WClass = FindFirstObject<UClass>(*WidgetClass, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!WClass) WClass = FindFirstObject<UClass>(*(FString(TEXT("U") + WidgetClass)), EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!WClass || !WClass->IsChildOf(UWidget::StaticClass()))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), FString::Printf(TEXT("Widget class '%s' not found"), *WidgetClass), OutErrorCode, OutErrorMessage); return false; }

	WidgetBP->Modify();
	bool bReplaced = false;

	if (WidgetBP->WidgetTree->RootWidget)
	{
		// Replace: set new root directly (children of old root are discarded)
		WidgetBP->WidgetTree->RootWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WClass, FName(*WidgetClass));
		bReplaced = true;
	}
	else
	{
		WidgetBP->WidgetTree->RootWidget = WidgetBP->WidgetTree->ConstructWidget<UWidget>(WClass, FName(*WidgetClass));
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("blueprint"), WidgetBP->GetName());
	OutResult->SetStringField(TEXT("root_widget"), WidgetBP->WidgetTree->RootWidget->GetName());
	OutResult->SetStringField(TEXT("root_widget_class"), WidgetClass);
	OutResult->SetBoolField(TEXT("set"), true);
	OutResult->SetBoolField(TEXT("replaced"), bReplaced);
	return true;
}

// ====== Reparent ======
bool FMCPWidgetReparentHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName, NewParentName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath)
		|| !Payload->TryGetStringField(TEXT("widget_name"), WidgetName)
		|| !Payload->TryGetStringField(TEXT("parent_widget"), NewParentName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, widget_name, parent_widget required"), OutErrorCode, OutErrorMessage); return false; }

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	UPanelWidget* NewParent = Cast<UPanelWidget>(WidgetBP->WidgetTree->FindWidget(FName(*NewParentName)));
	if (!NewParent)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PARENT_NOT_PANEL"), FString::Printf(TEXT("Panel parent '%s' not found or not a panel"), *NewParentName), OutErrorCode, OutErrorMessage); return false; }

	// Circular check: cannot reparent into own subtree
	UWidget* Check = NewParent;
	while (Check)
	{
		if (Check == Target)
		{ MCPBridgeHelpers::BuildErrorResponse(TEXT("REPARENT_CYCLE_FORBIDDEN"), TEXT("Cannot reparent widget into its own subtree"), OutErrorCode, OutErrorMessage); return false; }
		Check = Check->GetParent();
	}

	WidgetBP->Modify();
	Target->RemoveFromParent();
	NewParent->AddChild(Target);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);
	OutResult->SetStringField(TEXT("new_parent"), NewParentName);
	OutResult->SetBoolField(TEXT("reparented"), true);
	return true;
}

// ====== ReorderChild ======
bool FMCPWidgetReorderChildHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("widget_name"), WidgetName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and widget_name required"), OutErrorCode, OutErrorMessage); return false; }

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	UPanelWidget* Parent = Target->GetParent();
	if (!Parent)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PARENT_NOT_FOUND"), TEXT("Widget has no parent to reorder within"), OutErrorCode, OutErrorMessage); return false; }

	int32 NewIndex = 0;
	Payload->TryGetNumberField(TEXT("index"), NewIndex);

	WidgetBP->Modify();
	Target->RemoveFromParent();
	Parent->InsertChildAt(NewIndex, Target);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);
	OutResult->SetNumberField(TEXT("new_index"), NewIndex);
	OutResult->SetBoolField(TEXT("reordered"), true);
	return true;
}

// ====== Rename ======
bool FMCPWidgetRenameHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName, NewName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath)
		|| !Payload->TryGetStringField(TEXT("widget_name"), WidgetName)
		|| !Payload->TryGetStringField(TEXT("new_name"), NewName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, widget_name, new_name required"), OutErrorCode, OutErrorMessage); return false; }

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	if (WidgetBP->WidgetTree->FindWidget(FName(*NewName)))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NAME_CONFLICT"), FString::Printf(TEXT("Widget name '%s' already exists"), *NewName), OutErrorCode, OutErrorMessage); return false; }

	WidgetBP->Modify();
	Target->Rename(*NewName);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("old_name"), WidgetName);
	OutResult->SetStringField(TEXT("new_name"), NewName);
	OutResult->SetBoolField(TEXT("renamed"), true);
	return true;
}

// ====== Stage 16 Slice 3 — Slot Editing ======

// ====== SetSlotProperty ======
bool FMCPWidgetSetSlotPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName, PropName, PropValue;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath)
		|| !Payload->TryGetStringField(TEXT("widget_name"), WidgetName)
		|| !Payload->TryGetStringField(TEXT("property_name"), PropName)
		|| !Payload->TryGetStringField(TEXT("value"), PropValue))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, widget_name, property_name, value required"), OutErrorCode, OutErrorMessage); return false; }

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	if (!Target->Slot)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("SLOT_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' has no slot"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	FProperty* Property = Target->Slot->GetClass()->FindPropertyByName(*PropName);
	if (!Property)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("SLOT_PROPERTY_NOT_SUPPORTED"), FString::Printf(TEXT("Slot property '%s' not found on %s"), *PropName, *Target->Slot->GetClass()->GetName()), OutErrorCode, OutErrorMessage); return false; }

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Target->Slot);
	if (!ValuePtr)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_SET_FAILED"), TEXT("Failed to access slot property memory"), OutErrorCode, OutErrorMessage); return false; }

	WidgetBP->Modify();
	const TCHAR* Result = Property->ImportText_Direct(*PropValue, ValuePtr, Target->Slot, PPF_None);
	if (!Result)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_SET_FAILED"), TEXT("Failed to parse slot property value"), OutErrorCode, OutErrorMessage); return false; }

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	FString ReadBack;
	Property->ExportTextItem_Direct(ReadBack, ValuePtr, nullptr, Target->Slot, PPF_None);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);
	OutResult->SetStringField(TEXT("property_name"), PropName);
	OutResult->SetStringField(TEXT("value"), ReadBack);
	OutResult->SetBoolField(TEXT("set"), true);
	return true;
}

// ====== Stage 16 Slice 5 — Advanced Tree Operations ======

// ====== Duplicate ======
bool FMCPWidgetDuplicateHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("widget_name"), WidgetName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and widget_name required"), OutErrorCode, OutErrorMessage); return false; }

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	UPanelWidget* Parent = Target->GetParent();
	if (!Parent)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PARENT_NOT_FOUND"), TEXT("Cannot duplicate root widget"), OutErrorCode, OutErrorMessage); return false; }

	FString NewName;
	Payload->TryGetStringField(TEXT("new_name"), NewName);
	if (NewName.IsEmpty()) NewName = WidgetName + TEXT("_Copy");

	if (WidgetBP->WidgetTree->FindWidget(FName(*NewName)))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NAME_CONFLICT"), FString::Printf(TEXT("Widget name '%s' already exists"), *NewName), OutErrorCode, OutErrorMessage); return false; }

	WidgetBP->Modify();
	UWidget* Duplicated = DuplicateObject<UWidget>(Target, WidgetBP->WidgetTree, FName(*NewName));
	if (!Duplicated)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_DUPLICATE_FAILED"), TEXT("Failed to duplicate widget"), OutErrorCode, OutErrorMessage); return false; }

	Parent->AddChild(Duplicated);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("original_name"), WidgetName);
	OutResult->SetStringField(TEXT("new_name"), Duplicated->GetName());
	OutResult->SetBoolField(TEXT("duplicated"), true);
	return true;
}

// ====== WrapWithPanel ======
bool FMCPWidgetWrapWithPanelHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, WidgetName, PanelClass;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath)
		|| !Payload->TryGetStringField(TEXT("widget_name"), WidgetName)
		|| !Payload->TryGetStringField(TEXT("panel_class"), PanelClass))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, widget_name, panel_class required"), OutErrorCode, OutErrorMessage); return false; }

	UWidgetBlueprint* WidgetBP = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
	if (!WidgetBP || !WidgetBP->WidgetTree)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), TEXT("Widget Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	UWidget* Target = WidgetBP->WidgetTree->FindWidget(FName(*WidgetName));
	if (!Target)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NOT_FOUND"), FString::Printf(TEXT("Widget '%s' not found"), *WidgetName), OutErrorCode, OutErrorMessage); return false; }

	UPanelWidget* OldParent = Target->GetParent();
	if (!OldParent)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PARENT_NOT_FOUND"), TEXT("Cannot wrap root widget"), OutErrorCode, OutErrorMessage); return false; }

	// Find panel class
	UClass* PClass = FindFirstObject<UClass>(*PanelClass, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!PClass) PClass = FindFirstObject<UClass>(*(FString(TEXT("U") + PanelClass)), EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!PClass || !PClass->IsChildOf(UPanelWidget::StaticClass()))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), FString::Printf(TEXT("Panel class '%s' not found"), *PanelClass), OutErrorCode, OutErrorMessage); return false; }

	// Generate wrapper name
	FString WrapperName;
	Payload->TryGetStringField(TEXT("wrapper_name"), WrapperName);
	if (WrapperName.IsEmpty()) WrapperName = PanelClass + TEXT("_Wrapper");

	if (WidgetBP->WidgetTree->FindWidget(FName(*WrapperName)))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_NAME_CONFLICT"), FString::Printf(TEXT("Wrapper name '%s' already exists"), *WrapperName), OutErrorCode, OutErrorMessage); return false; }

	WidgetBP->Modify();
	// Create wrapper panel
	UPanelWidget* Wrapper = Cast<UPanelWidget>(WidgetBP->WidgetTree->ConstructWidget<UWidget>(PClass, FName(*WrapperName)));
	if (!Wrapper)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("WIDGET_WRAP_FAILED"), TEXT("Failed to construct wrapper panel"), OutErrorCode, OutErrorMessage); return false; }

	// Insert wrapper at target's position, then move target into wrapper
	int32 TargetIndex = OldParent->GetChildIndex(Target);
	OldParent->RemoveChild(Target);
	OldParent->InsertChildAt(TargetIndex, Wrapper);
	Wrapper->AddChild(Target);

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("widget_name"), WidgetName);
	OutResult->SetStringField(TEXT("wrapper_name"), WrapperName);
	OutResult->SetStringField(TEXT("panel_class"), PanelClass);
	OutResult->SetBoolField(TEXT("wrapped"), true);
	return true;
}
