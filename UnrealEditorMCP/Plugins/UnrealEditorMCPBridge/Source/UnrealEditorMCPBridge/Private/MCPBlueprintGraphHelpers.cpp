#include "MCPBlueprintGraphHelpers.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBPGraphHelpers, Log, All);

namespace MCPBlueprintGraphHelpers
{
	UBlueprint* LoadBlueprint(const FString& AssetPath, FString& OutErrorCode, FString& OutErrorMessage)
	{
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *AssetPath);
		if (!BP)
		{
			OutErrorCode = TEXT("BP_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Blueprint not found: '%s'"), *AssetPath);
		}
		return BP;
	}

	bool IsParentClassActorDerived(const FString& ParentClassName, FString& OutErrorCode, FString& OutErrorMessage)
	{
		UClass* ParentClass = FindFirstObject<UClass>(*ParentClassName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		if (!ParentClass)
		{
			// 尝试常见前缀：A->Actor, U->Object, BP -> Blueprint 派生类
			for (const TCHAR* Prefix : { TEXT("A"), TEXT("U"), TEXT("") })
			{
				FString Prefixed = FString(Prefix) + ParentClassName;
				ParentClass = FindFirstObject<UClass>(*Prefixed, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
				if (ParentClass) break;
			}
		}
		if (!ParentClass || !ParentClass->IsChildOf(UObject::StaticClass()))
		{
			OutErrorCode = TEXT("CLASS_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Parent class '%s' not found"), *ParentClassName);
			return false;
		}
		if (ParentClass->HasMetaData(TEXT("NotBlueprintable")))
		{
			OutErrorCode = TEXT("CLASS_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Parent class '%s' is marked NotBlueprintable"), *ParentClassName);
			return false;
		}
		return true;
	}

	UEdGraph* GetEventGraph(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!BP)
		{
			OutErrorCode = TEXT("BP_NOT_FOUND");
			OutErrorMessage = TEXT("Blueprint is null");
			return nullptr;
		}

		if (BP->UbergraphPages.Num() == 0)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("Blueprint has no EventGraph");
			return nullptr;
		}

		UEdGraph* EventGraph = BP->UbergraphPages[0];
		if (!EventGraph)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("EventGraph is null");
		}
		return EventGraph;
	}

	UK2Node_Event* FindEventNodeByMemberName(UEdGraph* Graph, const FName& MemberName)
	{
		if (!Graph) return nullptr;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
			if (EventNode)
			{
				FName NodeMemberName = EventNode->EventReference.GetMemberName();
				if (NodeMemberName == MemberName)
				{
					return EventNode;
				}
			}
		}
		return nullptr;
	}

	UK2Node_Event* CreateEventNode(UEdGraph* Graph, const FName& MemberName, UClass* ParentClass, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Graph)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("Graph is null");
			return nullptr;
		}

		if (!ParentClass)
		{
			OutErrorCode = TEXT("CLASS_NOT_FOUND");
			OutErrorMessage = TEXT("Parent class is null");
			return nullptr;
		}

		UK2Node_Event* EventNode = NewObject<UK2Node_Event>(Graph);
		if (!EventNode)
		{
			OutErrorCode = TEXT("CREATE_FAILED");
			OutErrorMessage = TEXT("Failed to create Event node");
			return nullptr;
		}

		EventNode->EventReference.SetExternalMember(MemberName, ParentClass);
		EventNode->CreateNewGuid();
		EventNode->NodePosX = 0;
		EventNode->NodePosY = Graph->Nodes.Num() * 200;
		Graph->AddNode(EventNode, false, false);
		EventNode->PostPlacedNewNode();
		EventNode->AllocateDefaultPins();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(CastChecked<UBlueprint>(Graph->GetOuter()));

		return EventNode;
	}

	UEdGraphNode* FindNodeByGuid(UEdGraph* Graph, const FString& NodeGuid)
	{
		if (!Graph) return nullptr;

		FGuid Guid;
		if (!FGuid::Parse(NodeGuid, Guid)) return nullptr;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node->NodeGuid == Guid)
			{
				return Node;
			}
		}
		return nullptr;
	}

	UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
	{
		if (!Node) return nullptr;

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == Direction && Pin->PinName.ToString() == PinName)
			{
				return Pin;
			}
		}
		return nullptr;
	}

	bool TryConnectPins(UEdGraph* Graph, const FString& SourceNodeGuid, const FString& SourcePinName, const FString& TargetNodeGuid, const FString& TargetPinName, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Graph)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("Graph is null");
			return false;
		}

		UEdGraphNode* SourceNode = FindNodeByGuid(Graph, SourceNodeGuid);
		if (!SourceNode)
		{
			OutErrorCode = TEXT("NODE_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Source node not found by guid: '%s'"), *SourceNodeGuid);
			return false;
		}

		UEdGraphNode* TargetNode = FindNodeByGuid(Graph, TargetNodeGuid);
		if (!TargetNode)
		{
			OutErrorCode = TEXT("NODE_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Target node not found by guid: '%s'"), *TargetNodeGuid);
			return false;
		}

		UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
		if (!SourcePin)
		{
			OutErrorCode = TEXT("PIN_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Source pin '%s' not found on node"), *SourcePinName);
			return false;
		}

		UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
		if (!TargetPin)
		{
			OutErrorCode = TEXT("PIN_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Target pin '%s' not found on node"), *TargetPinName);
			return false;
		}

		const UEdGraphSchema* Schema = Graph->GetSchema();
		if (!Schema)
		{
			OutErrorCode = TEXT("SCHEMA_NOT_FOUND");
			OutErrorMessage = TEXT("Graph schema not found");
			return false;
		}

		FBlueprintEditorUtils::MarkBlueprintAsModified(CastChecked<UBlueprint>(Graph->GetOuter()));

		bool bConnected = Schema->TryCreateConnection(SourcePin, TargetPin);
		if (!bConnected)
		{
			OutErrorCode = TEXT("CONNECTION_REJECTED");
			OutErrorMessage = FString::Printf(TEXT("Schema rejected connection: '%s.%s' -> '%s.%s'"), *SourcePinName, *SourcePin->PinType.PinCategory.ToString(), *TargetPinName, *TargetPin->PinType.PinCategory.ToString());
		}
		return bConnected;
	}

	bool CompileBlueprint(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!BP)
		{
			OutErrorCode = TEXT("BP_NOT_FOUND");
			OutErrorMessage = TEXT("Blueprint is null");
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(BP);

		if (BP->Status == BS_Error)
		{
			OutErrorCode = TEXT("COMPILE_FAILED");
			OutErrorMessage = TEXT("Blueprint compilation failed");
			return false;
		}

		return true;
	}

	bool SaveBlueprintAsset(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!BP)
		{
			OutErrorCode = TEXT("BP_NOT_FOUND");
			OutErrorMessage = TEXT("Blueprint is null");
			return false;
		}

		UPackage* Package = BP->GetOutermost();
		if (!Package)
		{
			OutErrorCode = TEXT("SAVE_FAILED");
			OutErrorMessage = TEXT("Failed to get package");
			return false;
		}

		Package->MarkPackageDirty();

		FString PackageName = Package->GetName();
		FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		bool bSaved = UPackage::SavePackage(Package, BP, *FilePath, SaveArgs);

		if (!bSaved)
		{
			OutErrorCode = TEXT("SAVE_FAILED");
			OutErrorMessage = TEXT("Failed to save Blueprint to disk");
			return false;
		}

		return true;
	}

	FString GuidToString(const FGuid& Guid)
	{
		return Guid.ToString(EGuidFormats::DigitsWithHyphens);
	}

	TSharedPtr<FJsonObject> BuildEventGraphInfoJson(UEdGraph* Graph)
	{
		TSharedPtr<FJsonObject> Info = MakeShareable(new FJsonObject());

		if (!Graph)
		{
			Info->SetBoolField(TEXT("exists"), false);
			Info->SetNumberField(TEXT("event_node_count"), 0);
			return Info;
		}

		Info->SetBoolField(TEXT("exists"), true);
		Info->SetStringField(TEXT("graph_name"), Graph->GetName());

		int32 EventCount = 0;
		TArray<TSharedPtr<FJsonValue>> EventsArray;

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
			if (EventNode)
			{
				EventCount++;
				TSharedPtr<FJsonObject> EventObj = MakeShareable(new FJsonObject());
				FName MemberName = EventNode->EventReference.GetMemberName();
				EventObj->SetStringField(TEXT("member_name"), MemberName.ToString());
				EventObj->SetStringField(TEXT("node_guid"), GuidToString(EventNode->NodeGuid));

				TArray<TSharedPtr<FJsonValue>> PinsArray;
				for (UEdGraphPin* Pin : EventNode->Pins)
				{
					TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject());
					PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
					PinObj->SetStringField(TEXT("direction"), (Pin->Direction == EGPD_Output) ? TEXT("output") : TEXT("input"));
					PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
				}
				EventObj->SetArrayField(TEXT("pins"), PinsArray);

				EventsArray.Add(MakeShareable(new FJsonValueObject(EventObj)));
			}
		}

		Info->SetNumberField(TEXT("event_node_count"), EventCount);
		Info->SetArrayField(TEXT("event_nodes"), EventsArray);

		return Info;
	}

	void BuildNodeInfoJson(UEdGraphNode* Node, TSharedPtr<FJsonObject> NodeObj)
	{
		if (!Node || !NodeObj.IsValid()) return;

		NodeObj->SetStringField(TEXT("node_guid"), GuidToString(Node->NodeGuid));
		NodeObj->SetStringField(TEXT("node_class"), Node->GetClass()->GetName());

		TArray<TSharedPtr<FJsonValue>> PinsArray;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject());
			PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
			PinObj->SetStringField(TEXT("direction"), (Pin->Direction == EGPD_Output) ? TEXT("output") : TEXT("input"));
			PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
			PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
		}
		NodeObj->SetArrayField(TEXT("pins"), PinsArray);
	}
}
