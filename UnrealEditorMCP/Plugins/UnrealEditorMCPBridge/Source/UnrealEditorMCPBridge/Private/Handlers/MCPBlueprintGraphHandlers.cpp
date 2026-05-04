#include "Handlers/MCPBlueprintGraphHandlers.h"
#include "MCPBlueprintGraphHelpers.h"
#include "MCPBridgeHelpers.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBPGraph, Log, All);

// ====== 2. blueprint_get_event_graph_info ======
bool FMCPGetBlueprintEventGraphInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	UEdGraph* EventGraph = BP->UbergraphPages.Num() > 0 ? BP->UbergraphPages[0] : nullptr;

	OutResult = MCPBlueprintGraphHelpers::BuildEventGraphInfoJson(EventGraph);
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("blueprint_name"), BP->GetName());
	OutResult->SetBoolField(TEXT("has_event_graph"), EventGraph != nullptr);

	return true;
}

// ====== 3. blueprint_add_event_node ======
bool FMCPBlueprintAddEventNodeHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString EventName = TEXT("ReceiveBeginPlay");
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("event_name"), EventName);
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	UEdGraph* EventGraph = MCPBlueprintGraphHelpers::GetEventGraph(BP, OutErrorCode, OutErrorMessage);
	if (!EventGraph) return false;

	UK2Node_Event* ExistingNode = MCPBlueprintGraphHelpers::FindEventNodeByMemberName(EventGraph, FName(*EventName));

	FString NodeGuid;
	bool bCreated = false;

	if (ExistingNode)
	{
		NodeGuid = MCPBlueprintGraphHelpers::GuidToString(ExistingNode->NodeGuid);
	}
	else
	{
		if (!BP->ParentClass)
		{
			MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), TEXT("Blueprint has no parent class"), OutErrorCode, OutErrorMessage);
			return false;
		}

		BP->Modify();
		ExistingNode = MCPBlueprintGraphHelpers::CreateEventNode(EventGraph, FName(*EventName), BP->ParentClass, OutErrorCode, OutErrorMessage);
		if (!ExistingNode) return false;

		NodeGuid = MCPBlueprintGraphHelpers::GuidToString(ExistingNode->NodeGuid);
		bCreated = true;
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("graph_name"), EventGraph->GetName());
	OutResult->SetStringField(TEXT("event_name"), EventName);
	OutResult->SetStringField(TEXT("node_guid"), NodeGuid);
	OutResult->SetBoolField(TEXT("created"), bCreated);
	MCPBlueprintGraphHelpers::BuildNodeInfoJson(ExistingNode, OutResult);
	return true;
}

// ====== 4. blueprint_add_call_function_node ======
bool FMCPBlueprintAddCallFunctionNodeHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, FunctionName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path and function_name are required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString GraphName;
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("graph_name"), GraphName);
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	UEdGraph* TargetGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		for (UEdGraph* Graph : BP->UbergraphPages)
		{
			if (Graph && Graph->GetName() == GraphName)
			{
				TargetGraph = Graph;
				break;
			}
		}
		if (!TargetGraph)
		{
			for (UEdGraph* Graph : BP->FunctionGraphs)
			{
				if (Graph && Graph->GetName() == GraphName)
				{
					TargetGraph = Graph;
					break;
				}
			}
		}
	}
	else
	{
		TargetGraph = MCPBlueprintGraphHelpers::GetEventGraph(BP, OutErrorCode, OutErrorMessage);
	}

	if (!TargetGraph)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("GRAPH_NOT_FOUND"), FString::Printf(TEXT("Graph not found: '%s'"), *GraphName), OutErrorCode, OutErrorMessage);
		return false;
	}

	UFunction* Function = FindFirstObject<UFunction>(*FunctionName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!Function)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: '%s'"), *FunctionName), OutErrorCode, OutErrorMessage);
		return false;
	}

	if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("FUNCTION_NOT_CALLABLE"), FString::Printf(TEXT("Function '%s' is not BlueprintCallable"), *FunctionName), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();

	UK2Node_CallFunction* CallNode = NewObject<UK2Node_CallFunction>(TargetGraph);
	if (!CallNode)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("CREATE_FAILED"), TEXT("Failed to create CallFunction node"), OutErrorCode, OutErrorMessage);
		return false;
	}

	CallNode->SetFromFunction(Function);
	CallNode->CreateNewGuid();
	CallNode->NodePosX = 400;
	CallNode->NodePosY = TargetGraph->Nodes.Num() * 200;
	TargetGraph->AddNode(CallNode, false, false);
	CallNode->PostPlacedNewNode();
	CallNode->AllocateDefaultPins();

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	OutResult->SetStringField(TEXT("function_name"), FunctionName);
	OutResult->SetStringField(TEXT("node_guid"), MCPBlueprintGraphHelpers::GuidToString(CallNode->NodeGuid));
	MCPBlueprintGraphHelpers::BuildNodeInfoJson(CallNode, OutResult);
	return true;
}

// ====== 5. blueprint_connect_pins ======
bool FMCPBlueprintConnectPinsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, SourceNodeGuid, SourcePinName, TargetNodeGuid, TargetPinName;
	if (!Payload.IsValid() ||
		!Payload->TryGetStringField(TEXT("asset_path"), AssetPath) ||
		!Payload->TryGetStringField(TEXT("source_node_guid"), SourceNodeGuid) ||
		!Payload->TryGetStringField(TEXT("source_pin_name"), SourcePinName) ||
		!Payload->TryGetStringField(TEXT("target_node_guid"), TargetNodeGuid) ||
		!Payload->TryGetStringField(TEXT("target_pin_name"), TargetPinName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, source_node_guid, source_pin_name, target_node_guid, target_pin_name are required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString GraphName;
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("graph_name"), GraphName);
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	UEdGraph* TargetGraph = nullptr;
	if (!GraphName.IsEmpty())
	{
		for (UEdGraph* Graph : BP->UbergraphPages)
		{
			if (Graph && Graph->GetName() == GraphName)
			{
				TargetGraph = Graph;
				break;
			}
		}
		for (UEdGraph* Graph : BP->FunctionGraphs)
		{
			if (Graph && Graph->GetName() == GraphName)
			{
				TargetGraph = Graph;
				break;
			}
		}
	}
	else
	{
		TargetGraph = MCPBlueprintGraphHelpers::GetEventGraph(BP, OutErrorCode, OutErrorMessage);
	}

	if (!TargetGraph)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("GRAPH_NOT_FOUND"), FString::Printf(TEXT("Graph not found: '%s'"), *GraphName), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();
	if (!MCPBlueprintGraphHelpers::TryConnectPins(TargetGraph, SourceNodeGuid, SourcePinName, TargetNodeGuid, TargetPinName, OutErrorCode, OutErrorMessage))
	{
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	OutResult->SetStringField(TEXT("source_node_guid"), SourceNodeGuid);
	OutResult->SetStringField(TEXT("target_node_guid"), TargetNodeGuid);
	OutResult->SetBoolField(TEXT("connected"), true);
	return true;
}

// ====== 6. blueprint_compile_save ======
bool FMCPCompileSaveBlueprintHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	bool bSave = false;
	if (Payload.IsValid())
	{
		Payload->TryGetBoolField(TEXT("save"), bSave);
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	bool bCompiled = MCPBlueprintGraphHelpers::CompileBlueprint(BP, OutErrorCode, OutErrorMessage);

	bool bSaved = false;
	if (bCompiled && bSave)
	{
		FString SaveErrorCode, SaveErrorMessage;
		bSaved = MCPBlueprintGraphHelpers::SaveBlueprintAsset(BP, SaveErrorCode, SaveErrorMessage);
		if (!bSaved)
		{
			OutErrorCode = SaveErrorCode;
			OutErrorMessage = SaveErrorMessage;
		}
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetBoolField(TEXT("compiled"), bCompiled);
	OutResult->SetBoolField(TEXT("saved"), bSaved);

	if (!bCompiled)
	{
		return false;
	}

	return true;
}
