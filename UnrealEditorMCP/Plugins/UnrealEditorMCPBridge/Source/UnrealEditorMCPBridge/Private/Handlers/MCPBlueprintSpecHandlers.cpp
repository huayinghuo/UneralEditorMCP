#include "Handlers/MCPBlueprintSpecHandlers.h"
#include "MCPBlueprintSpecTypes.h"
#include "MCPBlueprintGraphHelpers.h"
#include "MCPBridgeHelpers.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBPSpec, Log, All);

namespace MCPBlueprintSpecHelpers
{
	TSharedPtr<FJsonObject> BuildSpecJson(const FString& BlueprintName, const FString& ParentClass,
		const TArray<FBPNodeSpec>& Nodes, const TArray<FBPEdgeSpec>& Edges)
	{
		TSharedPtr<FJsonObject> Spec = MakeShareable(new FJsonObject());

		TSharedPtr<FJsonObject> BPObj = MakeShareable(new FJsonObject());
		BPObj->SetStringField(TEXT("name"), BlueprintName);
		BPObj->SetStringField(TEXT("parent_class"), ParentClass);
		BPObj->SetStringField(TEXT("path"), TEXT("/Game/MCPTest"));
		Spec->SetObjectField(TEXT("blueprint"), BPObj);

		TArray<TSharedPtr<FJsonValue>> GraphsArray;
		GraphsArray.Add(MakeShareable(new FJsonValueString(TEXT("EventGraph"))));
		Spec->SetArrayField(TEXT("graphs"), GraphsArray);

		TArray<TSharedPtr<FJsonValue>> NodesArray;
		for (const FBPNodeSpec& NodeSpec : Nodes)
		{
			TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject());
			NodeObj->SetStringField(TEXT("id"), NodeSpec.Id);
			NodeObj->SetStringField(TEXT("type"), NodeSpec.Type);
			if (!NodeSpec.EventName.IsEmpty())
			{
				NodeObj->SetStringField(TEXT("event_name"), NodeSpec.EventName);
			}
			if (!NodeSpec.FunctionName.IsEmpty())
			{
				NodeObj->SetStringField(TEXT("function_name"), NodeSpec.FunctionName);
			}
			if (NodeSpec.Params.Num() > 0)
			{
				TSharedPtr<FJsonObject> ParamsObj = MakeShareable(new FJsonObject());
				for (const auto& Pair : NodeSpec.Params)
				{
					ParamsObj->SetStringField(Pair.Key, Pair.Value);
				}
				NodeObj->SetObjectField(TEXT("params"), ParamsObj);
			}
			NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
		}
		Spec->SetArrayField(TEXT("nodes"), NodesArray);

		TArray<TSharedPtr<FJsonValue>> EdgesArray;
		for (const FBPEdgeSpec& EdgeSpec : Edges)
		{
			TSharedPtr<FJsonObject> EdgeObj = MakeShareable(new FJsonObject());
			EdgeObj->SetStringField(TEXT("from_node"), EdgeSpec.FromNode);
			EdgeObj->SetStringField(TEXT("from_pin"), EdgeSpec.FromPin);
			EdgeObj->SetStringField(TEXT("to_node"), EdgeSpec.ToNode);
			EdgeObj->SetStringField(TEXT("to_pin"), EdgeSpec.ToPin);
			EdgesArray.Add(MakeShareable(new FJsonValueObject(EdgeObj)));
		}
		Spec->SetArrayField(TEXT("edges"), EdgesArray);

		return Spec;
	}

	bool ParseSpecFromJson(TSharedPtr<FJsonObject> Payload, FString& OutName, FString& OutParentClass,
		TArray<FBPNodeSpec>& OutNodes, TArray<FBPEdgeSpec>& OutEdges,
		FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Payload.IsValid())
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.spec is required"), OutErrorCode, OutErrorMessage);
			return false;
		}

		const TSharedPtr<FJsonObject>* BpObj = nullptr;
		if (!Payload->TryGetObjectField(TEXT("blueprint"), BpObj) || !BpObj->IsValid())
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("spec.blueprint is required"), OutErrorCode, OutErrorMessage);
			return false;
		}

		(*BpObj)->TryGetStringField(TEXT("name"), OutName);
		(*BpObj)->TryGetStringField(TEXT("parent_class"), OutParentClass);
		if (OutParentClass.IsEmpty()) OutParentClass = TEXT("Actor");

		const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
		Payload->TryGetArrayField(TEXT("nodes"), NodesArray);
		if (NodesArray)
		{
			for (const TSharedPtr<FJsonValue>& NodeVal : *NodesArray)
			{
				const TSharedPtr<FJsonObject>* NodeObj = nullptr;
				if (!NodeVal->TryGetObject(NodeObj)) continue;

				FBPNodeSpec NodeSpec;
				(*NodeObj)->TryGetStringField(TEXT("id"), NodeSpec.Id);
				(*NodeObj)->TryGetStringField(TEXT("type"), NodeSpec.Type);
				(*NodeObj)->TryGetStringField(TEXT("event_name"), NodeSpec.EventName);
				(*NodeObj)->TryGetStringField(TEXT("function_name"), NodeSpec.FunctionName);
				(*NodeObj)->TryGetStringField(TEXT("node_class"), NodeSpec.NodeClass);
				(*NodeObj)->TryGetStringField(TEXT("var_name"), NodeSpec.VarName);

				const TSharedPtr<FJsonObject>* ParamsObj = nullptr;
				if ((*NodeObj)->TryGetObjectField(TEXT("params"), ParamsObj))
				{
					for (const auto& Pair : (*ParamsObj)->Values)
					{
						FString Value;
						if (Pair.Value->TryGetString(Value))
						{
							NodeSpec.Params.Add(Pair.Key, Value);
						}
					}
				}
				OutNodes.Add(NodeSpec);
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* EdgesArray = nullptr;
		Payload->TryGetArrayField(TEXT("edges"), EdgesArray);
		if (EdgesArray)
		{
			for (const TSharedPtr<FJsonValue>& EdgeVal : *EdgesArray)
			{
				const TSharedPtr<FJsonObject>* EdgeObj = nullptr;
				if (!EdgeVal->TryGetObject(EdgeObj)) continue;

				FBPEdgeSpec EdgeSpec;
				(*EdgeObj)->TryGetStringField(TEXT("from_node"), EdgeSpec.FromNode);
				(*EdgeObj)->TryGetStringField(TEXT("from_pin"), EdgeSpec.FromPin);
				(*EdgeObj)->TryGetStringField(TEXT("to_node"), EdgeSpec.ToNode);
				(*EdgeObj)->TryGetStringField(TEXT("to_pin"), EdgeSpec.ToPin);
				OutEdges.Add(EdgeSpec);
			}
		}

		return true;
	}

	TSharedPtr<FJsonObject> ExportSpecFromBP(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage)
	{
		TSharedPtr<FJsonObject> Spec = MakeShareable(new FJsonObject());

		TSharedPtr<FJsonObject> BpObj = MakeShareable(new FJsonObject());
		BpObj->SetStringField(TEXT("name"), BP->GetName());
		BpObj->SetStringField(TEXT("asset_path"), BP->GetPathName());
		if (BP->ParentClass)
		{
			BpObj->SetStringField(TEXT("parent_class"), BP->ParentClass->GetName());
		}
		Spec->SetObjectField(TEXT("blueprint"), BpObj);

		UEdGraph* EventGraph = BP->UbergraphPages.Num() > 0 ? BP->UbergraphPages[0] : nullptr;
		if (!EventGraph)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("Blueprint has no EventGraph");
			return Spec;
		}

		TArray<TSharedPtr<FJsonValue>> NodesArray;
		TMap<FString, int32> TypeIndex;
		TMap<FGuid, FString> GuidToSpecId;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject());

			int32 Idx = 0;
			if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
			{
				FName MemberName = EventNode->EventReference.GetMemberName();
				FString TypeKey = TEXT("event_") + MemberName.ToString();
				Idx = TypeIndex.FindOrAdd(TypeKey, 0);
				TypeIndex[TypeKey] = Idx + 1;
				NodeObj->SetStringField(TEXT("id"), FString::Printf(TEXT("%s_%d"), *TypeKey, Idx));
				NodeObj->SetStringField(TEXT("type"), TEXT("event"));
				NodeObj->SetStringField(TEXT("event_name"), MemberName.ToString());
			}
			else if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
			{
				FName FuncName = CallNode->FunctionReference.GetMemberName();
				FString TypeKey = TEXT("call_") + FuncName.ToString();
				Idx = TypeIndex.FindOrAdd(TypeKey, 0);
				TypeIndex[TypeKey] = Idx + 1;
				NodeObj->SetStringField(TEXT("id"), FString::Printf(TEXT("%s_%d"), *TypeKey, Idx));
				NodeObj->SetStringField(TEXT("type"), TEXT("call_function"));
				NodeObj->SetStringField(TEXT("function_name"), FuncName.ToString());
			}
			else if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
			{
				FName VarName = VarGet->VariableReference.GetMemberName();
				FString TypeKey = TEXT("var_get_") + VarName.ToString();
				Idx = TypeIndex.FindOrAdd(TypeKey, 0);
				TypeIndex[TypeKey] = Idx + 1;
				NodeObj->SetStringField(TEXT("id"), FString::Printf(TEXT("%s_%d"), *TypeKey, Idx));
				NodeObj->SetStringField(TEXT("type"), TEXT("variable_get"));
				NodeObj->SetStringField(TEXT("var_name"), VarName.ToString());
			}
			else if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
			{
				FName VarName = VarSet->VariableReference.GetMemberName();
				FString TypeKey = TEXT("var_set_") + VarName.ToString();
				Idx = TypeIndex.FindOrAdd(TypeKey, 0);
				TypeIndex[TypeKey] = Idx + 1;
				NodeObj->SetStringField(TEXT("id"), FString::Printf(TEXT("%s_%d"), *TypeKey, Idx));
				NodeObj->SetStringField(TEXT("type"), TEXT("variable_set"));
				NodeObj->SetStringField(TEXT("var_name"), VarName.ToString());
			}
			else
			{
				FString ClassName = Node->GetClass()->GetName();
				FString TypeKey = TEXT("node_") + ClassName;
				Idx = TypeIndex.FindOrAdd(TypeKey, 0);
				TypeIndex[TypeKey] = Idx + 1;
				NodeObj->SetStringField(TEXT("id"), FString::Printf(TEXT("%s_%d"), *TypeKey, Idx));
				NodeObj->SetStringField(TEXT("type"), TEXT("node_by_class"));
				NodeObj->SetStringField(TEXT("node_class"), ClassName);
			}

			NodeObj->SetStringField(TEXT("node_guid"), MCPBlueprintGraphHelpers::GuidToString(Node->NodeGuid));

			TArray<TSharedPtr<FJsonValue>> PinsArray;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject());
				PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
				PinObj->SetStringField(TEXT("direction"), (Pin->Direction == EGPD_Output) ? TEXT("output") : TEXT("input"));
				PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
				if (!Pin->DefaultValue.IsEmpty())
				{
					PinObj->SetStringField(TEXT("default"), Pin->DefaultValue);
				}
				PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
			}
			NodeObj->SetArrayField(TEXT("pins"), PinsArray);

			GuidToSpecId.Add(Node->NodeGuid, NodeObj->GetStringField(TEXT("id")));

			NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
		}
		Spec->SetArrayField(TEXT("nodes"), NodesArray);

		TArray<TSharedPtr<FJsonValue>> EdgesArray;
		for (UEdGraphNode* Node : EventGraph->Nodes)
		{
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->Direction != EGPD_Output) continue;
				for (UEdGraphPin* LinkedTo : Pin->LinkedTo)
				{
					if (!LinkedTo) continue;

					FGuid FromGuid = Node->NodeGuid;
					FGuid ToGuid = LinkedTo->GetOwningNode()->NodeGuid;
					FString* FromId = GuidToSpecId.Find(FromGuid);
					FString* ToId = GuidToSpecId.Find(ToGuid);
					if (!FromId || !ToId) continue;

					TSharedPtr<FJsonObject> EdgeObj = MakeShareable(new FJsonObject());
					EdgeObj->SetStringField(TEXT("from_node"), *FromId);
					EdgeObj->SetStringField(TEXT("from_pin"), Pin->PinName.ToString());
					EdgeObj->SetStringField(TEXT("to_node"), *ToId);
					EdgeObj->SetStringField(TEXT("to_pin"), LinkedTo->PinName.ToString());
					EdgesArray.Add(MakeShareable(new FJsonValueObject(EdgeObj)));
				}
			}
		}
		Spec->SetArrayField(TEXT("edges"), EdgesArray);

		Spec->SetNumberField(TEXT("node_count"), NodesArray.Num());
		Spec->SetNumberField(TEXT("edge_count"), EdgesArray.Num());

		return Spec;
	}
}

// ====== blueprint_apply_spec ======
bool FMCPApplyBlueprintSpecHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	const TSharedPtr<FJsonObject>* SpecPtr = nullptr;
	if (!Payload.IsValid() || !Payload->TryGetObjectField(TEXT("spec"), SpecPtr) || !SpecPtr->IsValid())
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.spec is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString BPName, ParentClass;
	TArray<FBPNodeSpec> Nodes;
	TArray<FBPEdgeSpec> Edges;
	if (!MCPBlueprintSpecHelpers::ParseSpecFromJson(*SpecPtr, BPName, ParentClass, Nodes, Edges, OutErrorCode, OutErrorMessage))
	{
		return false;
	}

	if (BPName.IsEmpty())
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("spec.blueprint.name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	if (!MCPBlueprintGraphHelpers::IsParentClassActorDerived(ParentClass, OutErrorCode, OutErrorMessage))
	{
		return false;
	}

	FString PackagePath = TEXT("/Game/MCPTest");
	const TSharedPtr<FJsonObject>* BpObj = nullptr;
	if ((*SpecPtr)->TryGetObjectField(TEXT("blueprint"), BpObj))
	{
		if ((*BpObj).IsValid()) (*BpObj)->TryGetStringField(TEXT("path"), PackagePath);
	}

	FString PackageName = PackagePath / BPName;
	FString ObjectPath = PackageName + TEXT(".") + BPName;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	FAssetData Existing = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(ObjectPath));
	if (Existing.IsValid())
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_CONFLICT"), FString::Printf(TEXT("Asset already exists: '%s'"), *ObjectPath), OutErrorCode, OutErrorMessage);
		return false;
	}

	UClass* ParentClassPtr = FindFirstObject<UClass>(*ParentClass, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!ParentClassPtr)
	{
		FString WithPrefix = TEXT("A") + ParentClass;
		ParentClassPtr = FindFirstObject<UClass>(*WithPrefix, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	}

	UPackage* Package = CreatePackage(*PackageName);
	if (!Package)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to create package"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(
		ParentClassPtr, Package, *BPName,
		BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass()
	);
	if (!BP)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to create Blueprint"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UEdGraph* EventGraph = MCPBlueprintGraphHelpers::GetEventGraph(BP, OutErrorCode, OutErrorMessage);
	if (!EventGraph)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("GRAPH_NOT_FOUND"), TEXT("Blueprint has no EventGraph"), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();

	TMap<FString, FString> IdToGuid;
	int32 CreatedNodes = 0;

	for (const FBPNodeSpec& NodeSpec : Nodes)
	{
		if (NodeSpec.Id.IsEmpty()) continue;

		UEdGraphNode* CreatedNode = nullptr;

		if (NodeSpec.Type == TEXT("event"))
		{
			FName EventName(NodeSpec.EventName.IsEmpty() ? TEXT("ReceiveBeginPlay") : *NodeSpec.EventName);
			UK2Node_Event* EventNode = MCPBlueprintGraphHelpers::FindEventNodeByMemberName(EventGraph, EventName);
			if (!EventNode)
			{
				EventNode = MCPBlueprintGraphHelpers::CreateEventNode(EventGraph, EventName, BP->ParentClass, OutErrorCode, OutErrorMessage);
				if (!EventNode) return false;
			}
			CreatedNode = EventNode;
		}
		else if (NodeSpec.Type == TEXT("call_function"))
		{
			if (NodeSpec.FunctionName.IsEmpty())
			{
				MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), FString::Printf(TEXT("Node '%s': function_name required"), *NodeSpec.Id), OutErrorCode, OutErrorMessage);
				return false;
			}

			UFunction* Function = FindFirstObject<UFunction>(*NodeSpec.FunctionName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
			if (!Function)
			{
				MCPBridgeHelpers::BuildErrorResponse(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Node '%s': function '%s' not found"), *NodeSpec.Id, *NodeSpec.FunctionName), OutErrorCode, OutErrorMessage);
				return false;
			}
			if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable))
			{
				MCPBridgeHelpers::BuildErrorResponse(TEXT("FUNCTION_NOT_CALLABLE"), FString::Printf(TEXT("Node '%s': '%s' not BlueprintCallable"), *NodeSpec.Id, *NodeSpec.FunctionName), OutErrorCode, OutErrorMessage);
				return false;
			}

			UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(EventGraph);
			CallNode->SetFromFunction(Function);
			CallNode->CreateNewGuid();
			CallNode->NodePosX = 400;
			CallNode->NodePosY = (CreatedNodes + 1) * 200;
			EventGraph->AddNode(CallNode, false, false);
			CallNode->PostPlacedNewNode();
			CallNode->AllocateDefaultPins();

			for (const auto& Param : NodeSpec.Params)
			{
				for (UEdGraphPin* Pin : CallNode->Pins)
				{
					if (Pin->Direction == EGPD_Input && Pin->PinName.ToString() == Param.Key)
					{
						Pin->DefaultValue = Param.Value;
						break;
					}
				}
			}

			CreatedNode = CallNode;
		}
		else if (NodeSpec.Type == TEXT("node_by_class"))
		{
			if (NodeSpec.NodeClass.IsEmpty())
			{
				MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), FString::Printf(TEXT("Node '%s': node_class required for node_by_class type"), *NodeSpec.Id), OutErrorCode, OutErrorMessage);
				return false;
			}

			UClass* NodeClassPtr = nullptr;
			if (!MCPBlueprintGraphHelpers::ValidateNodeClass(NodeSpec.NodeClass, NodeClassPtr, OutErrorCode, OutErrorMessage))
			{
				return false;
			}

			CreatedNode = MCPBlueprintGraphHelpers::CreateNodeByClass(EventGraph, NodeClassPtr, 400, (CreatedNodes + 1) * 200, OutErrorCode, OutErrorMessage);
			if (!CreatedNode) return false;
		}
		else if (NodeSpec.Type == TEXT("variable_get") || NodeSpec.Type == TEXT("variable_set"))
		{
			if (NodeSpec.VarName.IsEmpty())
			{
				MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), FString::Printf(TEXT("Node '%s': var_name required for %s type"), *NodeSpec.Id, *NodeSpec.Type), OutErrorCode, OutErrorMessage);
				return false;
			}

			bool bVarExists = false;
			for (const FBPVariableDescription& Var : BP->NewVariables)
			{
				if (Var.VarName.ToString() == NodeSpec.VarName) { bVarExists = true; break; }
			}
			if (!bVarExists)
			{
				MCPBridgeHelpers::BuildErrorResponse(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Node '%s': variable '%s' not found"), *NodeSpec.Id, *NodeSpec.VarName), OutErrorCode, OutErrorMessage);
				return false;
			}

			if (NodeSpec.Type == TEXT("variable_get"))
			{
				UK2Node_VariableGet* VG = MCPBlueprintGraphHelpers::CreateVariableGetNode(EventGraph, FName(*NodeSpec.VarName), BP, 400, (CreatedNodes + 1) * 200, OutErrorCode, OutErrorMessage);
				if (!VG) return false;
				CreatedNode = VG;
			}
			else
			{
				UK2Node_VariableSet* VS = MCPBlueprintGraphHelpers::CreateVariableSetNode(EventGraph, FName(*NodeSpec.VarName), BP, 400, (CreatedNodes + 1) * 200, OutErrorCode, OutErrorMessage);
				if (!VS) return false;
				CreatedNode = VS;
			}
		}
		else
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("UNSUPPORTED_TYPE"), FString::Printf(TEXT("Node '%s': unknown type '%s'"), *NodeSpec.Id, *NodeSpec.Type), OutErrorCode, OutErrorMessage);
			return false;
		}

		if (CreatedNode)
		{
			IdToGuid.Add(NodeSpec.Id, MCPBlueprintGraphHelpers::GuidToString(CreatedNode->NodeGuid));
			CreatedNodes++;
		}
	}

	int32 CreatedEdges = 0;
	for (const FBPEdgeSpec& EdgeSpec : Edges)
	{
		FString* SourceGuid = IdToGuid.Find(EdgeSpec.FromNode);
		if (!SourceGuid)
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("NODE_NOT_FOUND"), FString::Printf(TEXT("Edge from node '%s' not found"), *EdgeSpec.FromNode), OutErrorCode, OutErrorMessage);
			return false;
		}
		FString* TargetGuid = IdToGuid.Find(EdgeSpec.ToNode);
		if (!TargetGuid)
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("NODE_NOT_FOUND"), FString::Printf(TEXT("Edge to node '%s' not found"), *EdgeSpec.ToNode), OutErrorCode, OutErrorMessage);
			return false;
		}

		if (!MCPBlueprintGraphHelpers::TryConnectPins(EventGraph, *SourceGuid, EdgeSpec.FromPin, *TargetGuid, EdgeSpec.ToPin, OutErrorCode, OutErrorMessage))
		{
			return false;
		}
		CreatedEdges++;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(BP);

	FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, BP, *FilePath, SaveArgs);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("name"), BP->GetName());
	OutResult->SetStringField(TEXT("asset_path"), BP->GetPathName());
	OutResult->SetNumberField(TEXT("nodes_created"), CreatedNodes);
	OutResult->SetNumberField(TEXT("edges_created"), CreatedEdges);
	OutResult->SetBoolField(TEXT("compiled"), false);
	OutResult->SetBoolField(TEXT("saved"), bSaved);

	TArray<TSharedPtr<FJsonValue>> GuidMapping;
	for (const auto& Pair : IdToGuid)
	{
		TSharedPtr<FJsonObject> Map = MakeShareable(new FJsonObject());
		Map->SetStringField(TEXT("spec_id"), Pair.Key);
		Map->SetStringField(TEXT("node_guid"), Pair.Value);
		GuidMapping.Add(MakeShareable(new FJsonValueObject(Map)));
	}
	OutResult->SetArrayField(TEXT("id_guid_mapping"), GuidMapping);

	if (!bSaved)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SAVE_FAILED"), TEXT("Spec applied but failed to save to disk"), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}

// ====== blueprint_export_spec ======
bool FMCPExportBlueprintSpecHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	OutResult = MCPBlueprintSpecHelpers::ExportSpecFromBP(BP, OutErrorCode, OutErrorMessage);
	if (OutErrorCode == TEXT("GRAPH_NOT_FOUND"))
	{
		return false;
	}
	return true;
}
