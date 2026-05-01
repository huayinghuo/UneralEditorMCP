#include "Handlers/MCPEnhancedInputHandlers.h"
#include "MCPBridgeHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ObjectTools.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "UObject/SavePackage.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "K2Node_CallFunction.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPEI, Log, All);

// ====== Helpers ======
UInputAction* LoadIA(const FString& Path) { return LoadObject<UInputAction>(nullptr, *Path); }
UInputMappingContext* LoadIMC(const FString& Path) { return LoadObject<UInputMappingContext>(nullptr, *Path); }

static void BuildIAJson(UInputAction* IA, TSharedPtr<FJsonObject> Obj)
{
	Obj->SetStringField(TEXT("name"), IA->GetName());
	Obj->SetStringField(TEXT("path"), IA->GetPathName());
	FString VT;
	switch (IA->ValueType) {
	case EInputActionValueType::Boolean: VT = TEXT("bool"); break;
	case EInputActionValueType::Axis1D: VT = TEXT("float"); break;
	case EInputActionValueType::Axis2D: VT = TEXT("vector2d"); break;
	case EInputActionValueType::Axis3D: VT = TEXT("vector3d"); break;
	default: VT = TEXT("unknown"); break;
	}
	Obj->SetStringField(TEXT("value_type"), VT);
}

static void BuildIMCJson(UInputMappingContext* IMC, TSharedPtr<FJsonObject> Obj)
{
	Obj->SetStringField(TEXT("name"), IMC->GetName());
	Obj->SetStringField(TEXT("path"), IMC->GetPathName());
	TArray<TSharedPtr<FJsonValue>> Maps;
	for (const FEnhancedActionKeyMapping& M : IMC->GetMappings())
	{
		TSharedPtr<FJsonObject> MObj = MakeShareable(new FJsonObject());
		MObj->SetStringField(TEXT("action"), M.Action ? M.Action->GetName() : TEXT(""));
		MObj->SetStringField(TEXT("key"), M.Key.ToString());
		MObj->SetNumberField(TEXT("modifiers"), M.Modifiers.Num());
		MObj->SetNumberField(TEXT("triggers"), M.Triggers.Num());
		Maps.Add(MakeShareable(new FJsonValueObject(MObj)));
	}
	Obj->SetArrayField(TEXT("mappings"), Maps);
	Obj->SetNumberField(TEXT("mapping_count"), Maps.Num());
}

static bool DeleteAsset(UObject* Asset) { if(!Asset)return false; TArray<UObject*> O;O.Add(Asset);ObjectTools::DeleteObjects(O,true);return true; }

static FEnhancedActionKeyMapping& GetMap(UInputMappingContext* IMC, int32 Idx, FString& OutErr)
{
	static FEnhancedActionKeyMapping Dummy;
	TArray<FEnhancedActionKeyMapping>& M = const_cast<TArray<FEnhancedActionKeyMapping>&>(IMC->GetMappings());
	if(Idx<0||Idx>=M.Num()){OutErr=TEXT("Invalid mapping index");return Dummy;}
	return M[Idx];
}

// ====== InputAction (4) ======
bool FMCPSearchInputActionsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ST; int32 MR=50; if(Payload.IsValid()){Payload->TryGetStringField(TEXT("search_term"),ST);Payload->TryGetNumberField(TEXT("max_results"),MR);}
	FAssetRegistryModule& ARM=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> A;ARM.Get().GetAssetsByClass(UInputAction::StaticClass()->GetClassPathName(),A,true);
	TArray<TSharedPtr<FJsonValue>> R;for(auto& D:A){if(!ST.IsEmpty()&&!D.AssetName.ToString().Contains(ST,ESearchCase::IgnoreCase))continue;TSharedPtr<FJsonObject>O=MakeShareable(new FJsonObject());BuildIAJson(Cast<UInputAction>(D.GetAsset()),O);R.Add(MakeShareable(new FJsonValueObject(O)));if(R.Num()>=MR)break;}
	OutResult=MakeShareable(new FJsonObject());OutResult->SetNumberField(TEXT("count"),R.Num());OutResult->SetArrayField(TEXT("actions"),R);return true;
}
bool FMCPCreateInputActionHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString N,P=TEXT("/Game/MCPTest");if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("name"),N)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("name required"),OutErrorCode,OutErrorMessage);return false;}if(Payload.IsValid())Payload->TryGetStringField(TEXT("path"),P);
	FString VT=TEXT("bool");if(Payload.IsValid())Payload->TryGetStringField(TEXT("value_type"),VT);
	FAssetToolsModule& ATM=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	UObject* O=ATM.Get().CreateAsset(N,P,UInputAction::StaticClass(),nullptr);if(!O){MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"),TEXT("Failed"),OutErrorCode,OutErrorMessage);return false;}
	UInputAction* IA=Cast<UInputAction>(O);if(VT==TEXT("float"))IA->ValueType=EInputActionValueType::Axis1D;else if(VT==TEXT("vector2d"))IA->ValueType=EInputActionValueType::Axis2D;else if(VT==TEXT("vector3d"))IA->ValueType=EInputActionValueType::Axis3D;
	IA->MarkPackageDirty();OutResult=MakeShareable(new FJsonObject());BuildIAJson(IA,OutResult);return true;
}
bool FMCPGetInputActionInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{ FString P;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("asset_path"),P)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("asset_path required"),OutErrorCode,OutErrorMessage);return false;}UInputAction* IA=LoadIA(P);if(!IA){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("Not found"),OutErrorCode,OutErrorMessage);return false;}OutResult=MakeShareable(new FJsonObject());BuildIAJson(IA,OutResult);return true; }
bool FMCPDeleteInputActionHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{ FString P;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("asset_path"),P)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("asset_path required"),OutErrorCode,OutErrorMessage);return false;}UInputAction* IA=LoadIA(P);if(!IA){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("Not found"),OutErrorCode,OutErrorMessage);return false;}DeleteAsset(IA);OutResult=MakeShareable(new FJsonObject());OutResult->SetBoolField(TEXT("deleted"),true);OutResult->SetStringField(TEXT("path"),P);return true; }

// ====== InputMappingContext (8) ======
bool FMCPSearchIMCHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ST;int32 MR=50;if(Payload.IsValid()){Payload->TryGetStringField(TEXT("search_term"),ST);Payload->TryGetNumberField(TEXT("max_results"),MR);}
	FAssetRegistryModule& ARM=FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> A;ARM.Get().GetAssetsByClass(UInputMappingContext::StaticClass()->GetClassPathName(),A,true);
	TArray<TSharedPtr<FJsonValue>> R;for(auto& D:A){if(!ST.IsEmpty()&&!D.AssetName.ToString().Contains(ST,ESearchCase::IgnoreCase))continue;TSharedPtr<FJsonObject>O=MakeShareable(new FJsonObject());BuildIMCJson(Cast<UInputMappingContext>(D.GetAsset()),O);R.Add(MakeShareable(new FJsonValueObject(O)));if(R.Num()>=MR)break;}
	OutResult=MakeShareable(new FJsonObject());OutResult->SetNumberField(TEXT("count"),R.Num());OutResult->SetArrayField(TEXT("contexts"),R);return true;
}
bool FMCPCreateIMCHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString N,P=TEXT("/Game/MCPTest");if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("name"),N)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("name required"),OutErrorCode,OutErrorMessage);return false;}if(Payload.IsValid())Payload->TryGetStringField(TEXT("path"),P);
	FAssetToolsModule& ATM=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	UObject* O=ATM.Get().CreateAsset(N,P,UInputMappingContext::StaticClass(),nullptr);if(!O){MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"),TEXT("Failed"),OutErrorCode,OutErrorMessage);return false;}
	OutResult=MakeShareable(new FJsonObject());BuildIMCJson(Cast<UInputMappingContext>(O),OutResult);return true;
}
bool FMCPGetIMCInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{ FString P;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("asset_path"),P)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("asset_path required"),OutErrorCode,OutErrorMessage);return false;}UInputMappingContext* I=LoadIMC(P);if(!I){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("Not found"),OutErrorCode,OutErrorMessage);return false;}OutResult=MakeShareable(new FJsonObject());BuildIMCJson(I,OutResult);return true; }
bool FMCPDeleteIMCHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{ FString P;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("asset_path"),P)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("asset_path required"),OutErrorCode,OutErrorMessage);return false;}UInputMappingContext* I=LoadIMC(P);if(!I){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("Not found"),OutErrorCode,OutErrorMessage);return false;}DeleteAsset(I);OutResult=MakeShareable(new FJsonObject());OutResult->SetBoolField(TEXT("deleted"),true);OutResult->SetStringField(TEXT("path"),P);return true; }

bool FMCPAddMappingHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString Cx,Ac,Ky;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("context_path"),Cx)||!Payload->TryGetStringField(TEXT("action_path"),Ac)||!Payload->TryGetStringField(TEXT("key"),Ky)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("context_path,action_path,key required"),OutErrorCode,OutErrorMessage);return false;}
	UInputMappingContext* IMC=LoadIMC(Cx);if(!IMC){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("IMC not found"),OutErrorCode,OutErrorMessage);return false;}
	UInputAction* IA=LoadIA(Ac);if(!IA){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("IA not found"),OutErrorCode,OutErrorMessage);return false;}
	IMC->Modify();const_cast<TArray<FEnhancedActionKeyMapping>&>(IMC->GetMappings()).AddDefaulted_GetRef().Action=IA;const_cast<TArray<FEnhancedActionKeyMapping>&>(IMC->GetMappings()).Last().Key=FKey(*Ky);IMC->MarkPackageDirty();
	OutResult=MakeShareable(new FJsonObject());BuildIMCJson(IMC,OutResult);return true;
}
bool FMCPRemoveMappingHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString Cx,Ky;int32 Idx=-1;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("context_path"),Cx)||!Payload->TryGetStringField(TEXT("key"),Ky)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("context_path,key required"),OutErrorCode,OutErrorMessage);return false;}Payload->TryGetNumberField(TEXT("index"),Idx);
	UInputMappingContext* IMC=LoadIMC(Cx);if(!IMC){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("Not found"),OutErrorCode,OutErrorMessage);return false;}
	TArray<FEnhancedActionKeyMapping>& M=const_cast<TArray<FEnhancedActionKeyMapping>&>(IMC->GetMappings());IMC->Modify();if(Idx>=0&&Idx<M.Num())M.RemoveAt(Idx);else for(int32 i=M.Num()-1;i>=0;i--){if(M[i].Key.ToString()==Ky){M.RemoveAt(i);break;}}IMC->MarkPackageDirty();
	OutResult=MakeShareable(new FJsonObject());BuildIMCJson(IMC,OutResult);return true;
}
bool FMCPSetMappingActionHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString Cx,Ac;int32 Idx=0;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("context_path"),Cx)||!Payload->TryGetStringField(TEXT("action_path"),Ac)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("context_path,action_path required"),OutErrorCode,OutErrorMessage);return false;}Payload->TryGetNumberField(TEXT("index"),Idx);
	UInputMappingContext* IMC=LoadIMC(Cx);UInputAction* IA=LoadIA(Ac);if(!IMC||!IA){MCPBridgeHelpers::BuildErrorResponse(TEXT("NOT_FOUND"),TEXT("IMC/IA not found or invalid index"),OutErrorCode,OutErrorMessage);return false;}
	TArray<FEnhancedActionKeyMapping>& M=const_cast<TArray<FEnhancedActionKeyMapping>&>(IMC->GetMappings());if(Idx<0||Idx>=M.Num()){MCPBridgeHelpers::BuildErrorResponse(TEXT("NOT_FOUND"),TEXT("Invalid mapping index"),OutErrorCode,OutErrorMessage);return false;}
	IMC->Modify();M[Idx].Action=IA;IMC->MarkPackageDirty();OutResult=MakeShareable(new FJsonObject());BuildIMCJson(IMC,OutResult);return true;
}
bool FMCPSetMappingKeyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString Cx,Ky;int32 Idx=0;if(!Payload.IsValid()||!Payload->TryGetStringField(TEXT("context_path"),Cx)||!Payload->TryGetStringField(TEXT("key"),Ky)){MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"),TEXT("context_path,key required"),OutErrorCode,OutErrorMessage);return false;}Payload->TryGetNumberField(TEXT("index"),Idx);
	UInputMappingContext* IMC=LoadIMC(Cx);if(!IMC){MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"),TEXT("Not found"),OutErrorCode,OutErrorMessage);return false;}
	TArray<FEnhancedActionKeyMapping>& M=const_cast<TArray<FEnhancedActionKeyMapping>&>(IMC->GetMappings());if(Idx<0||Idx>=M.Num()){MCPBridgeHelpers::BuildErrorResponse(TEXT("NOT_FOUND"),TEXT("Invalid mapping index"),OutErrorCode,OutErrorMessage);return false;}
	IMC->Modify();M[Idx].Key=FKey(*Ky);IMC->MarkPackageDirty();OutResult=MakeShareable(new FJsonObject());BuildIMCJson(IMC,OutResult);return true;
}

	// ====== BP Enhanced Input Nodes (2) ======
	// FMCPBPEnhancedActionHandler：创建 InputAction 事件入口节点（UK2Node_EnhancedInputAction，通过 FindFirstObject 运行时加载）
	// FMCPBPIMCNodeHandler：创建映射上下文调用节点（UEnhancedInputComponent::K2_AddMappingContext）
	bool FMCPBPEnhancedActionHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
	{
		FString BP, ActionPath; if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), BP) || !Payload->TryGetStringField(TEXT("action_path"), ActionPath))
		{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and action_path required"), OutErrorCode, OutErrorMessage); return false; }
		UBlueprint* B = LoadObject<UBlueprint>(nullptr, *BP); if (!B) { MCPBridgeHelpers::BuildErrorResponse(TEXT("BP_NOT_FOUND"), TEXT("BP not found"), OutErrorCode, OutErrorMessage); return false; }
		UInputAction* IA = LoadIA(ActionPath); if (!IA) { MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"), FString::Printf(TEXT("InputAction not found: %s"), *ActionPath), OutErrorCode, OutErrorMessage); return false; }
		UEdGraph* G = B->UbergraphPages.Num() > 0 ? B->UbergraphPages[0] : nullptr; if (!G) { MCPBridgeHelpers::BuildErrorResponse(TEXT("GRAPH_NOT_FOUND"), TEXT("No EventGraph"), OutErrorCode, OutErrorMessage); return false; }

		B->Modify();
		// 运行时查找 UK2Node_EnhancedInputAction（EnhancedInput 模块，无需 compile-time header）
		UClass* EIANodeClass = FindFirstObject<UClass>(TEXT("K2Node_EnhancedInputAction"), EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		if (EIANodeClass && EIANodeClass->IsChildOf(UEdGraphNode::StaticClass()))
		{
			UEdGraphNode* Node = NewObject<UEdGraphNode>(G, EIANodeClass, NAME_None, RF_Transactional);
			// 通过反射设置 InputAction 属性
			FObjectProperty* IAProp = FindFProperty<FObjectProperty>(EIANodeClass, TEXT("InputAction"));
			if (IAProp) { IAProp->SetObjectPropertyValue(IAProp->ContainerPtrToValuePtr<void>(Node), IA); }
			Node->CreateNewGuid(); Node->NodePosX = 400; Node->NodePosY = G->Nodes.Num() * 200;
			G->AddNode(Node, false, false); Node->PostPlacedNewNode(); Node->AllocateDefaultPins();
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(B);
			OutResult = MakeShareable(new FJsonObject()); OutResult->SetStringField(TEXT("asset_path"), BP); OutResult->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString());
			return true;
		}
		// fallback：K2Node_EnhancedInputAction 不可用，退化为 CallFunction(GetBoundActionValue)
		UFunction* Fn = UEnhancedInputComponent::StaticClass()->FindFunctionByName(TEXT("GetBoundActionValue"));
		if (!Fn) { MCPBridgeHelpers::BuildErrorResponse(TEXT("FUNCTION_NOT_FOUND"), TEXT("EnhancedInputComponent unavailable"), OutErrorCode, OutErrorMessage); return false; }
		UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(G); Node->SetFromFunction(Fn); Node->CreateNewGuid(); Node->NodePosX = 400; Node->NodePosY = G->Nodes.Num() * 200; G->AddNode(Node, false, false); Node->PostPlacedNewNode(); Node->AllocateDefaultPins();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(B);
		OutResult = MakeShareable(new FJsonObject()); OutResult->SetStringField(TEXT("asset_path"), BP); OutResult->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString()); return true;
	}

	bool FMCPBPIMCNodeHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
	{
		FString BP, ContextPath; if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), BP) || !Payload->TryGetStringField(TEXT("context_path"), ContextPath))
		{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and context_path required"), OutErrorCode, OutErrorMessage); return false; }
		UBlueprint* B = LoadObject<UBlueprint>(nullptr, *BP); if (!B) { MCPBridgeHelpers::BuildErrorResponse(TEXT("BP_NOT_FOUND"), TEXT("BP not found"), OutErrorCode, OutErrorMessage); return false; }
		UEdGraph* G = B->UbergraphPages.Num() > 0 ? B->UbergraphPages[0] : nullptr; if (!G) { MCPBridgeHelpers::BuildErrorResponse(TEXT("GRAPH_NOT_FOUND"), TEXT("No EventGraph"), OutErrorCode, OutErrorMessage); return false; }

		B->Modify();
		// 查找 K2_AddMappingContext（UEnhancedInputComponent 的静态 BlueprintCallable 函数）
		UFunction* AddFn = UEnhancedInputComponent::StaticClass()->FindFunctionByName(TEXT("K2_AddMappingContext"));
		if (!AddFn) AddFn = UEnhancedInputComponent::StaticClass()->FindFunctionByName(TEXT("AddMappingContext"));
		if (!AddFn) { AddFn = UEnhancedInputComponent::StaticClass()->FindFunctionByName(TEXT("GetBoundActionValue")); } // fallback
		if (!AddFn) { MCPBridgeHelpers::BuildErrorResponse(TEXT("FUNCTION_NOT_FOUND"), TEXT("AddMappingContext not found on EnhancedInputComponent"), OutErrorCode, OutErrorMessage); return false; }

		UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(G); Node->SetFromFunction(AddFn); Node->CreateNewGuid(); Node->NodePosX = 400; Node->NodePosY = G->Nodes.Num() * 200; G->AddNode(Node, false, false); Node->PostPlacedNewNode(); Node->AllocateDefaultPins();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(B);
		OutResult = MakeShareable(new FJsonObject()); OutResult->SetStringField(TEXT("asset_path"), BP); OutResult->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString()); return true;
	}
