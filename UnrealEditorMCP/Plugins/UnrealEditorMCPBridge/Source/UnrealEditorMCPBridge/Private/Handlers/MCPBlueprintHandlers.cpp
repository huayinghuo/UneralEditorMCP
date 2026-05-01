#include "Handlers/MCPBlueprintHandlers.h"
#include "MCPBridgeHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "UObject/SavePackage.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBlueprint, Log, All);

// ====== ListBlueprints ======
bool FMCPListBlueprintsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString SearchPath = TEXT("/Game");
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("path"), SearchPath);
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(*SearchPath);
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	OutResult = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> BPsArray;

	for (const FAssetData& Asset : AssetList)
	{
		TSharedPtr<FJsonObject> BPObj = MakeShareable(new FJsonObject());
		BPObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		BPObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		BPObj->SetStringField(TEXT("package"), Asset.PackageName.ToString());

		FString ParentClass;
		if (Asset.GetTagValue(FName("NativeParentClass"), ParentClass) && !ParentClass.IsEmpty())
		{
			BPObj->SetStringField(TEXT("parent_class"), ParentClass);
		}
		BPsArray.Add(MakeShareable(new FJsonValueObject(BPObj)));
	}

	OutResult->SetArrayField(TEXT("blueprints"), BPsArray);
	OutResult->SetNumberField(TEXT("count"), BPsArray.Num());
	OutResult->SetStringField(TEXT("path"), SearchPath);
	return true;
}

// ====== GetBlueprintInfo ======
bool FMCPGetBlueprintInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!BP)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("BP_NOT_FOUND"), FString::Printf(TEXT("Blueprint not found: '%s'"), *AssetPath), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("name"), BP->GetName());
	OutResult->SetStringField(TEXT("path"), BP->GetPathName());
	OutResult->SetStringField(TEXT("blueprint_type"), UEnum::GetValueAsString(BP->BlueprintType));

	if (BP->ParentClass)
	{
		OutResult->SetStringField(TEXT("parent_class"), BP->ParentClass->GetName());
	}

	// 变量列表
	TArray<TSharedPtr<FJsonValue>> VarsArray;
	for (const FBPVariableDescription& Var : BP->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShareable(new FJsonObject());
		VarObj->SetStringField(TEXT("name"), Var.VarName.ToString());
		VarObj->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
		VarObj->SetBoolField(TEXT("editable"), (Var.PropertyFlags & CPF_Edit) != 0);
		VarsArray.Add(MakeShareable(new FJsonValueObject(VarObj)));
	}
	OutResult->SetArrayField(TEXT("variables"), VarsArray);

	// 函数/图表列表
	TArray<TSharedPtr<FJsonValue>> FuncsArray;
	for (UEdGraph* Graph : BP->FunctionGraphs)
	{
		TSharedPtr<FJsonObject> FuncObj = MakeShareable(new FJsonObject());
		FuncObj->SetStringField(TEXT("name"), Graph->GetName());
		FuncsArray.Add(MakeShareable(new FJsonValueObject(FuncObj)));
	}
	OutResult->SetArrayField(TEXT("functions"), FuncsArray);

	// 实现的接口
	TArray<TSharedPtr<FJsonValue>> InterfacesArray;
	for (const FBPInterfaceDescription& Interface : BP->ImplementedInterfaces)
	{
		if (Interface.Interface)
		{
			TSharedPtr<FJsonObject> IntfObj = MakeShareable(new FJsonObject());
			IntfObj->SetStringField(TEXT("name"), Interface.Interface->GetName());
			InterfacesArray.Add(MakeShareable(new FJsonValueObject(IntfObj)));
		}
	}
	OutResult->SetArrayField(TEXT("interfaces"), InterfacesArray);

	return true;
}

// ====== CreateBlueprint ======
bool FMCPCreateBlueprintHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString BlueprintName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("name"), BlueprintName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString ParentClassName = TEXT("Actor");
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("parent_class"), ParentClassName);
	}

	// 查找父类 UClass（优先精确，再补 A 前缀）
	UClass* ParentClass = FindFirstObject<UClass>(*ParentClassName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!ParentClass)
	{
		FString WithPrefix = TEXT("A") + ParentClassName;
		ParentClass = FindFirstObject<UClass>(*WithPrefix, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	}
	if (!ParentClass || !ParentClass->IsChildOf(AActor::StaticClass()))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), FString::Printf(TEXT("Parent class '%s' not found or not an Actor subclass"), *ParentClassName), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 确定包路径
	FString PackagePath = TEXT("/Game/MCPTest");
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("path"), PackagePath);
	}

	FString PackageName = PackagePath / BlueprintName;

	// 冲突检测：通过 AssetRegistry 检查资产是否已存在
	{
		FString ObjectPath = PackageName + TEXT(".") + BlueprintName;
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

	// 创建 Blueprint 资产
	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(
		ParentClass, Package, *BlueprintName,
		BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass()
	);

	if (!BP)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to create Blueprint asset"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 标记包脏并通知 AssetRegistry
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BP);

	// 保存包
	FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, BP, *FilePath, SaveArgs);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("name"), BP->GetName());
	OutResult->SetStringField(TEXT("path"), BP->GetPathName());
	OutResult->SetStringField(TEXT("parent_class"), ParentClass->GetName());
	OutResult->SetBoolField(TEXT("created"), true);
	OutResult->SetBoolField(TEXT("saved"), bSaved);

	if (!bSaved)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SAVE_FAILED"), TEXT("Blueprint created but failed to save to disk"), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}

// ====== AddBlueprintVariable（新增）======
bool FMCPAddBlueprintVariableHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, VarName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("var_name"), VarName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and var_name required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!BP)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("BP_NOT_FOUND"), TEXT("Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	FString VarType = TEXT("bool");
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("var_type"), VarType);

	FEdGraphPinType PinType;
	if (VarType == TEXT("bool"))       PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	else if (VarType == TEXT("int"))   PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	else if (VarType == TEXT("float")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Real; PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float; }
	else if (VarType == TEXT("string"))PinType.PinCategory = UEdGraphSchema_K2::PC_String;
	else if (VarType == TEXT("Vector")){ PinType.PinCategory = UEdGraphSchema_K2::PC_Struct; PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get(); }
	else if (VarType == TEXT("Rotator")){ PinType.PinCategory = UEdGraphSchema_K2::PC_Struct; PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get(); }
	else if (VarType == TEXT("Color")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Struct; PinType.PinSubCategoryObject = TBaseStructure<FLinearColor>::Get(); }
	else { MCPBridgeHelpers::BuildErrorResponse(TEXT("UNSUPPORTED_TYPE"), FString::Printf(TEXT("Unsupported type: %s"), *VarType), OutErrorCode, OutErrorMessage); return false; }

	FString DefaultValue;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("default_value"), DefaultValue);

	// 检查变量是否重名
	for (const FBPVariableDescription& Existing : BP->NewVariables)
	{
		if (Existing.VarName == FName(*VarName))
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("DUPLICATE_VARIABLE"), FString::Printf(TEXT("Variable '%s' already exists"), *VarName), OutErrorCode, OutErrorMessage);
			return false;
		}
	}

	BP->Modify();
	FBlueprintEditorUtils::AddMemberVariable(BP, FName(*VarName), PinType, DefaultValue);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("blueprint"), BP->GetName());
	OutResult->SetStringField(TEXT("var_name"), VarName);
	OutResult->SetStringField(TEXT("var_type"), VarType);
	OutResult->SetBoolField(TEXT("added"), true);
	return true;
}

// ====== AddBlueprintFunction（新增）======
bool FMCPAddBlueprintFunctionHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, FuncName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("func_name"), FuncName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and func_name required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!BP)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("BP_NOT_FOUND"), TEXT("Blueprint not found"), OutErrorCode, OutErrorMessage); return false; }

	// 检查函数/图表是否重名
	for (UEdGraph* Graph : BP->FunctionGraphs)
	{
		if (Graph->GetFName() == FName(*FuncName))
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("DUPLICATE_FUNCTION"), FString::Printf(TEXT("Function '%s' already exists"), *FuncName), OutErrorCode, OutErrorMessage);
			return false;
		}
	}

	for (const FBPVariableDescription& Var : BP->NewVariables)
	{
		if (Var.VarName == FName(*FuncName))
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("NAME_CONFLICT"), FString::Printf(TEXT("A variable named '%s' already exists"), *FuncName), OutErrorCode, OutErrorMessage);
			return false;
		}
	}

	BP->Modify();
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(BP, FName(*FuncName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddFunctionGraph(BP, NewGraph, false, (UClass*)nullptr);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("blueprint"), BP->GetName());
	OutResult->SetStringField(TEXT("func_name"), NewGraph->GetName());
	OutResult->SetBoolField(TEXT("added"), true);
	return true;
}

// ====== BlueprintAddComponent（新增：向 BP 的 SCS 添加组件）======
bool FMCPBlueprintAddComponentHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	// 1. 参数校验
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString ComponentClassName;
	if (!Payload->TryGetStringField(TEXT("component_class"), ComponentClassName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.component_class is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 2. 加载 Blueprint
	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
	if (!BP)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("BP_NOT_FOUND"), FString::Printf(TEXT("Blueprint not found: %s"), *AssetPath), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 3. 查找组件类
	UClass* ComponentClass = nullptr;
	// 尝试完整路径：/Script/Engine.StaticMeshComponent
	if (!ComponentClass)
	{
		ComponentClass = FindFirstObject<UClass>(*(TEXT("Class /Script/Engine.") + ComponentClassName), EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	}
	if (!ComponentClass)
	{
		// 尝试短类名查找
		ComponentClass = FindFirstObject<UClass>(*ComponentClassName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	}
	if (!ComponentClass || !ComponentClass->IsChildOf(UActorComponent::StaticClass()))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), FString::Printf(TEXT("Component class not found or not a component: %s"), *ComponentClassName), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 4. 组件名称（可选，默认用类名）
	FString ComponentName;
	if (!Payload->TryGetStringField(TEXT("component_name"), ComponentName) || ComponentName.IsEmpty())
	{
		ComponentName = ComponentClassName;
	}

	// 5. 获取或创建 SimpleConstructionScript
	BP->Modify();
	if (!BP->SimpleConstructionScript)
	{
		BP->SimpleConstructionScript = NewObject<USimpleConstructionScript>(BP, NAME_None, RF_Transactional);
	}

	// 6. 创建 SCS 节点
	USCS_Node* NewNode = BP->SimpleConstructionScript->CreateNode(ComponentClass, FName(*ComponentName));
	if (!NewNode)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), FString::Printf(TEXT("Failed to create SCS node for component class: %s"), *ComponentClassName), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 7. 添加到 SCS 根节点
	BP->SimpleConstructionScript->AddNode(NewNode);

	// 8. 编译 Blueprint
	FKismetEditorUtilities::CompileBlueprint(BP);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("blueprint"), BP->GetName());
	OutResult->SetStringField(TEXT("component_name"), NewNode->GetVariableName().ToString());
	OutResult->SetStringField(TEXT("component_class"), ComponentClassName);
	OutResult->SetBoolField(TEXT("added"), true);
	return true;
}
