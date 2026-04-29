#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UBlueprint;
class UEdGraph;
class UK2Node_Event;
class UK2Node_CallFunction;
class UEdGraphNode;
class UEdGraphPin;

namespace MCPBlueprintGraphHelpers
{
	UBlueprint* LoadBlueprint(const FString& AssetPath, FString& OutErrorCode, FString& OutErrorMessage);

	bool IsParentClassActorDerived(const FString& ParentClassName, FString& OutErrorCode, FString& OutErrorMessage);

	UEdGraph* GetEventGraph(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);

	UK2Node_Event* FindEventNodeByMemberName(UEdGraph* Graph, const FName& MemberName);

	UK2Node_Event* CreateEventNode(UEdGraph* Graph, const FName& MemberName, UClass* ParentClass, FString& OutErrorCode, FString& OutErrorMessage);

	UEdGraphNode* FindNodeByGuid(UEdGraph* Graph, const FString& NodeGuid);

	UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction);

	bool TryConnectPins(UEdGraph* Graph, const FString& SourceNodeGuid, const FString& SourcePinName, const FString& TargetNodeGuid, const FString& TargetPinName, FString& OutErrorCode, FString& OutErrorMessage);

	bool CompileBlueprint(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);
	bool SaveBlueprintAsset(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);

	FString GuidToString(const FGuid& Guid);
	TSharedPtr<FJsonObject> BuildEventGraphInfoJson(UEdGraph* Graph);
	void BuildNodeInfoJson(UEdGraphNode* Node, TSharedPtr<FJsonObject> NodeObj);
}
