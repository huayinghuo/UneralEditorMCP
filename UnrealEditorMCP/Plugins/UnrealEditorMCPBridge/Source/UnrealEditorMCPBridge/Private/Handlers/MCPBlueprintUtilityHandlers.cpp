#include "Handlers/MCPBlueprintUtilityHandlers.h"
#include "MCPBlueprintGraphHelpers.h"
#include "MCPBridgeHelpers.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/BlueprintEditorUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBPUtil, Log, All);

// ====== 1. blueprint_search_functions ======
/** 搜索引擎中 BlueprintCallable 函数：TObjectIterator 遍历所有 UFunction → 按 search_term 子串匹配 → 返回匹配函数的基本签名 */
bool FMCPBlueprintSearchFunctionsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString SearchTerm;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("search_term"), SearchTerm))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.search_term is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString ClassName;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("class_name"), ClassName);

	bool bWithContext = false;
	if (Payload.IsValid()) Payload->TryGetBoolField(TEXT("with_context"), bWithContext);

	int32 MaxResults = 50;
	if (Payload.IsValid()) Payload->TryGetNumberField(TEXT("max_results"), MaxResults);

	TArray<TSharedPtr<FJsonValue>> Results;
	for (TObjectIterator<UFunction> It; It; ++It)
	{
		UFunction* Func = *It;
		if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable)) continue;

		FString FuncName = Func->GetName();
		if (!FuncName.Contains(SearchTerm, ESearchCase::IgnoreCase)) continue;

		if (!ClassName.IsEmpty() && Func->GetOwnerClass())
		{
			if (!Func->GetOwnerClass()->GetName().Contains(ClassName, ESearchCase::IgnoreCase)) continue;
		}

		TSharedPtr<FJsonObject> FuncObj = MakeShareable(new FJsonObject());
		FuncObj->SetStringField(TEXT("name"), FuncName);
		FuncObj->SetStringField(TEXT("path"), Func->GetPathName());
		FuncObj->SetBoolField(TEXT("is_static"), Func->HasAnyFunctionFlags(FUNC_Static));
		if (bWithContext && Func->GetOwnerClass())
		{
			FuncObj->SetStringField(TEXT("class"), Func->GetOwnerClass()->GetName());
		}
		Results.Add(MakeShareable(new FJsonValueObject(FuncObj)));

		if (Results.Num() >= MaxResults) break;
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("search_term"), SearchTerm);
	OutResult->SetNumberField(TEXT("count"), Results.Num());
	OutResult->SetArrayField(TEXT("functions"), Results);
	return true;
}

// ====== 2. blueprint_set_variable_default ======
/** 设置 BP 变量 CDO 默认值：GetDefaultObject → 反射查找属性 → ImportText 写入 → MarkBlueprintAsModified */
bool FMCPBlueprintSetVariableDefaultHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, VariableName, DefaultValue;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and variable_name are required"), OutErrorCode, OutErrorMessage);
		return false;
	}
	Payload->TryGetStringField(TEXT("default_value"), DefaultValue);

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	bool bVarExists = false;
	for (const FBPVariableDescription& Var : BP->NewVariables)
	{
		if (Var.VarName.ToString() == VariableName) { bVarExists = true; break; }
	}
	if (!bVarExists)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Variable '%s' not found in Blueprint"), *VariableName), OutErrorCode, OutErrorMessage);
		return false;
	}

	if (!BP->GeneratedClass)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Blueprint has no GeneratedClass — compile BP first"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UObject* CDO = BP->GeneratedClass->GetDefaultObject();
	if (!CDO)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to get CDO from GeneratedClass"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FProperty* Prop = FindFProperty<FProperty>(BP->GeneratedClass, *VariableName);
	if (!Prop)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Property '%s' not found on CDO"), *VariableName), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();
	void* ValueAddr = Prop->ContainerPtrToValuePtr<void>(CDO);
	const TCHAR* Result = Prop->ImportText_Direct(*DefaultValue, ValueAddr, CDO, PPF_None);
	if (!Result)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SET_FAILED"), FString::Printf(TEXT("Failed to set default value for '%s'"), *VariableName), OutErrorCode, OutErrorMessage);
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	FString ReadBack;
	Prop->ExportTextItem_Direct(ReadBack, ValueAddr, nullptr, nullptr, PPF_None);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("variable_name"), VariableName);
	OutResult->SetStringField(TEXT("default_value"), ReadBack);
	return true;
}

// ====== 3. blueprint_set_component_default ======
/** 设置 BP 组件的默认属性值：查找 SCS 节点 → 获取组件模板 → 反射属性写入 → MarkBlueprintAsModified */
bool FMCPBlueprintSetComponentDefaultHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, ComponentName, PropertyName, Value;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("component_name"), ComponentName) || !Payload->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, component_name, and property_name are required"), OutErrorCode, OutErrorMessage);
		return false;
	}
	Payload->TryGetStringField(TEXT("value"), Value);

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	if (!BP->SimpleConstructionScript)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("COMPONENT_NOT_FOUND"), TEXT("Blueprint has no SimpleConstructionScript"), OutErrorCode, OutErrorMessage);
		return false;
	}

	USCS_Node* FoundNode = nullptr;
	for (USCS_Node* Node : BP->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->GetVariableName().ToString() == ComponentName)
		{
			FoundNode = Node;
			break;
		}
	}
	if (!FoundNode)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Component '%s' not found in SCS"), *ComponentName), OutErrorCode, OutErrorMessage);
		return false;
	}

	UActorComponent* ComponentTemplate = FoundNode->ComponentTemplate;
	if (!ComponentTemplate)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("COMPONENT_NOT_FOUND"), TEXT("Component template is null"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FProperty* Prop = FindFProperty<FProperty>(ComponentTemplate->GetClass(), *PropertyName);
	if (!Prop)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Property '%s' not found on component '%s'"), *PropertyName, *ComponentName), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();
	void* ValueAddr = Prop->ContainerPtrToValuePtr<void>(ComponentTemplate);
	const TCHAR* Result = Prop->ImportText_Direct(*Value, ValueAddr, ComponentTemplate, PPF_None);
	if (!Result)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SET_FAILED"), FString::Printf(TEXT("Failed to set '%s' on component '%s'"), *PropertyName, *ComponentName), OutErrorCode, OutErrorMessage);
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	FString ReadBack;
	Prop->ExportTextItem_Direct(ReadBack, ValueAddr, nullptr, nullptr, PPF_None);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("component_name"), ComponentName);
	OutResult->SetStringField(TEXT("property_name"), PropertyName);
	OutResult->SetStringField(TEXT("value"), ReadBack);
	return true;
}

// ====== 阶段 19 CDO 通用属性 (4 Handler) ======

bool FMCPBlueprintGetCDOPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, PropertyName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("property_name"), PropertyName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and property_name required"), OutErrorCode, OutErrorMessage); return false; }
	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage); if (!BP) return false;
	if (!BP->GeneratedClass) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Blueprint not compiled"), OutErrorCode, OutErrorMessage); return false; }
	UObject* CDO = BP->GeneratedClass->GetDefaultObject(); if (!CDO) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("No CDO"), OutErrorCode, OutErrorMessage); return false; }
	FProperty* Prop = BP->GeneratedClass->FindPropertyByName(FName(*PropertyName));
	if (!Prop) { MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Property '%s' not found"), *PropertyName), OutErrorCode, OutErrorMessage); return false; }

	void* Addr = Prop->ContainerPtrToValuePtr<void>(CDO);
	FString Value; Prop->ExportTextItem_Direct(Value, Addr, nullptr, nullptr, PPF_None);
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("property_name"), PropertyName); OutResult->SetStringField(TEXT("value"), Value);
	OutResult->SetStringField(TEXT("cpp_type"), Prop->GetCPPType());
	return true;
}

bool FMCPBlueprintSetCDOPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, PropertyName, Value;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("property_name"), PropertyName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and property_name required"), OutErrorCode, OutErrorMessage); return false; }
	Payload->TryGetStringField(TEXT("value"), Value);
	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage); if (!BP) return false;
	if (!BP->GeneratedClass) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Not compiled"), OutErrorCode, OutErrorMessage); return false; }
	UObject* CDO = BP->GeneratedClass->GetDefaultObject(); if (!CDO) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("No CDO"), OutErrorCode, OutErrorMessage); return false; }
	FProperty* Prop = BP->GeneratedClass->FindPropertyByName(FName(*PropertyName));
	if (!Prop) { MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_NOT_FOUND"), TEXT("Property not found"), OutErrorCode, OutErrorMessage); return false; }
	BP->Modify(); void* Addr = Prop->ContainerPtrToValuePtr<void>(CDO);
	if (!Prop->ImportText_Direct(*Value, Addr, CDO, PPF_None)) { MCPBridgeHelpers::BuildErrorResponse(TEXT("SET_FAILED"), TEXT("ImportText failed"), OutErrorCode, OutErrorMessage); return false; }
	FString ReadBack; Prop->ExportTextItem_Direct(ReadBack, Addr, nullptr, nullptr, PPF_None);
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("property_name"), PropertyName); OutResult->SetStringField(TEXT("value"), ReadBack);
	return true;
}

bool FMCPBlueprintAddCDOArrayHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, PropertyName, Value;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("property_name"), PropertyName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and property_name required"), OutErrorCode, OutErrorMessage); return false; }
	Payload->TryGetStringField(TEXT("value"), Value);
	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage); if (!BP) return false;
	if (!BP->GeneratedClass) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Not compiled"), OutErrorCode, OutErrorMessage); return false; }
	UObject* CDO = BP->GeneratedClass->GetDefaultObject(); if (!CDO) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("No CDO"), OutErrorCode, OutErrorMessage); return false; }
	FArrayProperty* ArrProp = CastField<FArrayProperty>(BP->GeneratedClass->FindPropertyByName(FName(*PropertyName)));
	if (!ArrProp) { MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_NOT_FOUND"), FString::Printf(TEXT("Array '%s' not found"), *PropertyName), OutErrorCode, OutErrorMessage); return false; }
	void* ArrAddr = ArrProp->ContainerPtrToValuePtr<void>(CDO); FScriptArrayHelper Helper(ArrProp, ArrAddr);
	BP->Modify(); int32 NewIdx = Helper.AddValue();
	if (!Value.IsEmpty()) { FProperty* Inner = ArrProp->Inner; void* Elem = Helper.GetRawPtr(NewIdx); Inner->ImportText_Direct(*Value, Elem, CDO, PPF_None); }
	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("property_name"), PropertyName); OutResult->SetNumberField(TEXT("new_index"), NewIdx); OutResult->SetNumberField(TEXT("new_count"), Helper.Num());
	return true;
}

bool FMCPBlueprintRemoveCDOArrayHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, PropertyName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("property_name"), PropertyName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and property_name required"), OutErrorCode, OutErrorMessage); return false; }
	int32 Index = 0; if (Payload.IsValid()) Payload->TryGetNumberField(TEXT("index"), Index);
	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage); if (!BP) return false;
	if (!BP->GeneratedClass) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Not compiled"), OutErrorCode, OutErrorMessage); return false; }
	UObject* CDO = BP->GeneratedClass->GetDefaultObject(); if (!CDO) { MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("No CDO"), OutErrorCode, OutErrorMessage); return false; }
	FArrayProperty* ArrProp = CastField<FArrayProperty>(BP->GeneratedClass->FindPropertyByName(FName(*PropertyName)));
	if (!ArrProp) { MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_NOT_FOUND"), TEXT("Array not found"), OutErrorCode, OutErrorMessage); return false; }
	void* ArrAddr = ArrProp->ContainerPtrToValuePtr<void>(CDO); FScriptArrayHelper Helper(ArrProp, ArrAddr);
	if (Index < 0 || Index >= Helper.Num()) { MCPBridgeHelpers::BuildErrorResponse(TEXT("NOT_FOUND"), TEXT("Index out of range"), OutErrorCode, OutErrorMessage); return false; }
	BP->Modify(); Helper.RemoveValues(Index, 1); FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("property_name"), PropertyName); OutResult->SetNumberField(TEXT("removed_index"), Index); OutResult->SetNumberField(TEXT("new_count"), Helper.Num());
	return true;
}
