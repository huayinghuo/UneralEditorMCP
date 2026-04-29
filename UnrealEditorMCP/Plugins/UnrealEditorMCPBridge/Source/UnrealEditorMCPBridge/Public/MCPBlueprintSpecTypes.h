#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UBlueprint;
class UEdGraphNode;
class UEdGraphPin;

struct FBPNodeSpec
{
	FString Id;
	FString Type;
	FString EventName;
	FString FunctionName;
	TMap<FString, FString> Params;
};

struct FBPEdgeSpec
{
	FString FromNode;
	FString FromPin;
	FString ToNode;
	FString ToPin;
};

struct FBPNodeExportInfo
{
	FString Id;
	FString NodeGuid;
	FString Type;
	FString EventName;
	FString FunctionName;
	TArray<TSharedPtr<FJsonValue>> Pins;
};

struct FBPEdgeExportInfo
{
	FString FromNodeGuid;
	FString FromPin;
	FString ToNodeGuid;
	FString ToPin;
};

namespace MCPBlueprintSpecHelpers
{
	TSharedPtr<FJsonObject> BuildSpecJson(const FString& BlueprintName, const FString& ParentClass,
		const TArray<FBPNodeSpec>& Nodes, const TArray<FBPEdgeSpec>& Edges);

	bool ParseSpecFromJson(TSharedPtr<FJsonObject> Payload, FString& OutName, FString& OutParentClass,
		TArray<FBPNodeSpec>& OutNodes, TArray<FBPEdgeSpec>& OutEdges,
		FString& OutErrorCode, FString& OutErrorMessage);

	TSharedPtr<FJsonObject> ExportSpecFromBP(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);
}
