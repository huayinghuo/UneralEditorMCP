#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UBlueprint;
class UEdGraph;
class UK2Node_Event;
class UK2Node_CallFunction;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_VariableGet;
class UK2Node_VariableSet;

namespace MCPBlueprintGraphHelpers
{
	/** 加载 Blueprint 资产，失败时设置 BP_NOT_FOUND 错误 */
	UBlueprint* LoadBlueprint(const FString& AssetPath, FString& OutErrorCode, FString& OutErrorMessage);

	/** 校验父类是否为 Actor 子类（支持自动补 A 前缀） */
	bool IsParentClassActorDerived(const FString& ParentClassName, FString& OutErrorCode, FString& OutErrorMessage);

	/** 获取 Blueprint 的默认 EventGraph（UbergraphPages[0]） */
	UEdGraph* GetEventGraph(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);

	/** 在图中查找指定 member_name 的事件节点 */
	UK2Node_Event* FindEventNodeByMemberName(UEdGraph* Graph, const FName& MemberName);

	/** 创建事件节点：NewObject → SetExternalMember → AllocateDefaultPins → AddNode */
	UK2Node_Event* CreateEventNode(UEdGraph* Graph, const FName& MemberName, UClass* ParentClass, FString& OutErrorCode, FString& OutErrorMessage);

	/** 根据 guid 字符串查找图节点 */
	UEdGraphNode* FindNodeByGuid(UEdGraph* Graph, const FString& NodeGuid);

	/** 在节点上按名称和方向查找引脚 */
	UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction);

	/** 连接两个引脚：校验节点/引脚存在 → Schema->TryCreateConnection */
	bool TryConnectPins(UEdGraph* Graph, const FString& SourceNodeGuid, const FString& SourcePinName, const FString& TargetNodeGuid, const FString& TargetPinName, FString& OutErrorCode, FString& OutErrorMessage);

	/** 编译 Blueprint，失败时检查 BS_Error 状态 */
	bool CompileBlueprint(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);

	/** 保存 Blueprint 到磁盘：GetOutermost → MarkPackageDirty → SavePackage */
	bool SaveBlueprintAsset(UBlueprint* BP, FString& OutErrorCode, FString& OutErrorMessage);

	/** FGuid 转标准连字符格式字符串 */
	FString GuidToString(const FGuid& Guid);

	/** 构建 EventGraph 信息 JSON（事件节点清单 + 引脚） */
	TSharedPtr<FJsonObject> BuildEventGraphInfoJson(UEdGraph* Graph);

	/** 将节点信息（guid/class/pins）写入 JSON 对象 */
	void BuildNodeInfoJson(UEdGraphNode* Node, TSharedPtr<FJsonObject> NodeObj);

	// ---- 阶段 17 新增 helper ----

	/** 校验 K2Node 类名合法性：查找 → 非抽象 → 非专用 → 可实例化 */
	bool ValidateNodeClass(const FString& NodeClassName, UClass*& OutClass, FString& OutErrorCode, FString& OutErrorMessage);

	/** 通过类创建 UEdGraphNode：AddNode（先入图获取 Schema 上下文）→ PostPlacedNewNode → AllocateDefaultPins */
	UEdGraphNode* CreateNodeByClass(UEdGraph* Graph, UClass* NodeClass, int32 PosX, int32 PosY, FString& OutErrorCode, FString& OutErrorMessage);

	/** 创建 VariableGet 节点：SetSelfMember → AddNode → PostPlacedNewNode → AllocateDefaultPins */
	UK2Node_VariableGet* CreateVariableGetNode(UEdGraph* Graph, const FName& VarName, UBlueprint* BP, int32 PosX, int32 PosY, FString& OutErrorCode, FString& OutErrorMessage);

	/** 创建 VariableSet 节点：流程同 Get，通过 VariableReference 绑定变量名 */
	UK2Node_VariableSet* CreateVariableSetNode(UEdGraph* Graph, const FName& VarName, UBlueprint* BP, int32 PosX, int32 PosY, FString& OutErrorCode, FString& OutErrorMessage);

	/** 设置引脚默认值：优先尝试 Schema->TrySetDefaultValue，失败则直接赋值 */
	bool SetPinDefaultValue(UEdGraphPin* Pin, const FString& DefaultValue, FString& OutErrorCode, FString& OutErrorMessage);

	/** 构建函数签名 JSON：遍历 FProperty 提取参数名/方向/类型 */
	TSharedPtr<FJsonObject> BuildFunctionSignatureJson(UFunction* Function);

	/** 根据 graph_name 解析目标图：在 UbergraphPages 和 FunctionGraphs 中查找 */
	UEdGraph* ResolveGraph(UBlueprint* BP, const FString& GraphName, FString& OutErrorCode, FString& OutErrorMessage);

	/** 断开节点指定方向引脚的全部连接 */
	bool DisconnectPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction, FString& OutErrorCode, FString& OutErrorMessage);
}
