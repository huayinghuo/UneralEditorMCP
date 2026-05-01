#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UBlueprint;
class UEdGraphNode;
class UEdGraphPin;

/** 蓝图声明式 Spec 节点定义：用于 apply_spec 输入和 export_spec 输出，支持 event/call_function/node_by_class/variable_get/variable_set 五种类型 */
struct FBPNodeSpec
{
	FString Id;               // 用户定义标识符，边通过此 ID 引用节点
	FString Type;             // 节点类型："event" / "call_function" / "node_by_class" / "variable_get" / "variable_set"
	FString EventName;        // 事件节点的事件名（如 ReceiveBeginPlay）
	FString FunctionName;     // 函数调用节点的函数名（如 PrintString）
	FString NodeClass;        // 通用 K2Node 类名（如 K2Node_IfThenElse）
	FString VarName;          // 变量名（用于 variable_get/set）
	TMap<FString, FString> Params; // 引脚默认值映射（如 InString → "hello"）
};

/** 蓝图声明式 Spec 边定义：连接两个节点的指定引脚 */
struct FBPEdgeSpec
{
	FString FromNode;  // 源节点 spec ID
	FString FromPin;   // 源引脚名
	FString ToNode;    // 目标节点 spec ID
	FString ToPin;     // 目标引脚名
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
	/** 构建最小 spec JSON（用于测试和例子） */
	TSharedPtr<FJsonObject> BuildSpecJson(const FString& BlueprintName, const FString& ParentClass,
		const TArray<FBPNodeSpec>& Nodes, const TArray<FBPEdgeSpec>& Edges);

	/** 从 JSON payload 解析 spec：提取 blueprint.name/class、nodes[]、edges[] */
	bool ParseSpecFromJson(TSharedPtr<FJsonObject> Payload, FString& OutName, FString& OutParentClass,
		TArray<FBPNodeSpec>& OutNodes, TArray<FBPEdgeSpec>& OutEdges,
		FString& OutErrorCode, FString& OutErrorMessage);

	/** 将已有 BP 的 EventGraph 导出为 spec JSON（含所有节点 + 引脚 + 边） */
	TSharedPtr<FJsonObject> ExportSpecFromBP(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);
}
