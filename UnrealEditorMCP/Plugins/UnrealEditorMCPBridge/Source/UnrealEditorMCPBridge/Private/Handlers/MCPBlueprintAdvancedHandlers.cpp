#include "Handlers/MCPBlueprintAdvancedHandlers.h"
#include "MCPBlueprintGraphHelpers.h"
#include "MCPBridgeHelpers.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPBPAdvanced, Log, All);

// ====== 阶段 17 新增 Helper 实现 ======

namespace MCPBlueprintGraphHelpers
{
	/** 校验 K2Node 类名合法性：查找类 → 检查为 UEdGraphNode 子类 → 排除抽象类 → 排除需专用命令的节点类型 */
	bool ValidateNodeClass(const FString& NodeClassName, UClass*& OutClass, FString& OutErrorCode, FString& OutErrorMessage)
	{
		// 先尝试原始类名查找，再补 K2Node_ 前缀
		OutClass = FindFirstObject<UClass>(*NodeClassName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		if (!OutClass)
		{
			FString Prefixed = TEXT("K2Node_") + NodeClassName;
			OutClass = FindFirstObject<UClass>(*Prefixed, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
		}
		// 类名不存在
		if (!OutClass)
		{
			OutErrorCode = TEXT("INVALID_NODE_CLASS");
			OutErrorMessage = FString::Printf(TEXT("Node class not found: '%s'"), *NodeClassName);
			return false;
		}
		// 非 UEdGraphNode 子类
		if (!OutClass->IsChildOf(UEdGraphNode::StaticClass()))
		{
			OutErrorCode = TEXT("INVALID_NODE_CLASS");
			OutErrorMessage = FString::Printf(TEXT("Class '%s' is not a UEdGraphNode subclass"), *NodeClassName);
			OutClass = nullptr;
			return false;
		}
		// 抽象类无法实例化
		if (OutClass->HasAnyClassFlags(CLASS_Abstract))
		{
			OutErrorCode = TEXT("INVALID_NODE_CLASS");
			OutErrorMessage = FString::Printf(TEXT("Node class '%s' is abstract and cannot be instantiated"), *NodeClassName);
			return false;
		}
		// 排除需要专用命令的节点类型（Event/CallFunction/Variable）
		if (OutClass == UK2Node_Event::StaticClass() ||
			OutClass == UK2Node_CallFunction::StaticClass() ||
			OutClass == UK2Node_VariableGet::StaticClass() ||
			OutClass == UK2Node_VariableSet::StaticClass())
		{
			OutErrorCode = TEXT("SPECIALIZED_NODE_CLASS");
			OutErrorMessage = FString::Printf(TEXT("'%s' requires a specialized command (event/call_function/variable_node)"), *NodeClassName);
			return false;
		}
		return true;
	}

	/** 通过类名动态创建 UEdGraphNode：NewObject → CreateNewGuid → AddNode（必须先入图以获取 Schema 上下文）→ PostPlacedNewNode → AllocateDefaultPins */
	UEdGraphNode* CreateNodeByClass(UEdGraph* Graph, UClass* NodeClass, int32 PosX, int32 PosY, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Graph)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("Graph is null");
			return nullptr;
		}
		if (!NodeClass)
		{
			OutErrorCode = TEXT("INVALID_NODE_CLASS");
			OutErrorMessage = TEXT("Node class is null");
			return nullptr;
		}

		UEdGraphNode* Node = NewObject<UEdGraphNode>(Graph, NodeClass, NAME_None, RF_Transactional);
		if (!Node)
		{
			OutErrorCode = TEXT("CREATE_FAILED");
			OutErrorMessage = FString::Printf(TEXT("Failed to create node of class '%s'"), *NodeClass->GetName());
			return nullptr;
		}

		Node->CreateNewGuid();
		Node->NodePosX = PosX;
		Node->NodePosY = PosY;
		Graph->AddNode(Node, false, false);
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();

		UBlueprint* BP = Cast<UBlueprint>(Graph->GetOuter());
		if (BP)
		{
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		}

		return Node;
	}

	/** 创建 VariableGet 节点：NewObject → SetSelfMember → CreateNewGuid → AddNode → PostPlacedNewNode → AllocateDefaultPins */
	UK2Node_VariableGet* CreateVariableGetNode(UEdGraph* Graph, const FName& VarName, UBlueprint* BP, int32 PosX, int32 PosY, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Graph || !BP)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("Graph or BP is null");
			return nullptr;
		}

		UK2Node_VariableGet* GetNode = NewObject<UK2Node_VariableGet>(Graph);
		GetNode->VariableReference.SetSelfMember(VarName);
		GetNode->CreateNewGuid();
		GetNode->NodePosX = PosX;
		GetNode->NodePosY = PosY;
		Graph->AddNode(GetNode, false, false);
		GetNode->PostPlacedNewNode();
		GetNode->AllocateDefaultPins();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		return GetNode;
	}

	/** 创建 VariableSet 节点：与 Get 节点相同的创建流程，VariableReference.SetSelfMember 后 AddNode → PostPlacedNewNode → AllocateDefaultPins */
	UK2Node_VariableSet* CreateVariableSetNode(UEdGraph* Graph, const FName& VarName, UBlueprint* BP, int32 PosX, int32 PosY, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Graph || !BP)
		{
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = TEXT("Graph or BP is null");
			return nullptr;
		}

		UK2Node_VariableSet* SetNode = NewObject<UK2Node_VariableSet>(Graph);
		SetNode->VariableReference.SetSelfMember(VarName);
		SetNode->CreateNewGuid();
		SetNode->NodePosX = PosX;
		SetNode->NodePosY = PosY;
		Graph->AddNode(SetNode, false, false);
		SetNode->PostPlacedNewNode();
		SetNode->AllocateDefaultPins();

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
		return SetNode;
	}

	/** 设置引脚默认值：优先使用 Schema->TrySetDefaultValue（类型安全），失败则直接赋值 DefaultValue */
	bool SetPinDefaultValue(UEdGraphPin* Pin, const FString& DefaultValue, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Pin)
		{
			OutErrorCode = TEXT("PIN_NOT_FOUND");
			OutErrorMessage = TEXT("Pin is null");
			return false;
		}
		if (Pin->Direction != EGPD_Input)
		{
			OutErrorCode = TEXT("PIN_NOT_FOUND");
			OutErrorMessage = TEXT("Cannot set default on output pin");
			return false;
		}

		const UEdGraphSchema* Schema = Pin->GetSchema();
		if (Schema)
		{
			Schema->TrySetDefaultValue(*Pin, DefaultValue);
		}
		else
		{
			Pin->DefaultValue = DefaultValue;
		}

		return true;
	}

	/** 构建函数签名 JSON：遍历 FProperty 提取参数名/方向/类型/是否返回参数 */
	TSharedPtr<FJsonObject> BuildFunctionSignatureJson(UFunction* Function)
	{
		TSharedPtr<FJsonObject> SigObj = MakeShareable(new FJsonObject());
		if (!Function)
		{
			SigObj->SetBoolField(TEXT("found"), false);
			return SigObj;
		}

		SigObj->SetBoolField(TEXT("found"), true);
		SigObj->SetStringField(TEXT("function_name"), Function->GetName());
		SigObj->SetStringField(TEXT("function_path"), Function->GetPathName());

		TArray<TSharedPtr<FJsonValue>> PinsArray;
		for (TFieldIterator<FProperty> PropIt(Function); PropIt; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject());
			PinObj->SetStringField(TEXT("name"), Prop->GetName());
			PinObj->SetStringField(TEXT("direction"), TEXT("input"));
			PinObj->SetStringField(TEXT("type"), Prop->GetCPPType());
			PinObj->SetBoolField(TEXT("is_exec"), false);

			if (Prop->IsA<FObjectProperty>())
			{
				PinObj->SetStringField(TEXT("type"), TEXT("object"));
			}

			if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				PinObj->SetStringField(TEXT("direction"), TEXT("output"));
			}

			PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
		}
		SigObj->SetArrayField(TEXT("params"), PinsArray);
		SigObj->SetNumberField(TEXT("param_count"), PinsArray.Num());

		return SigObj;
	}

	/** 根据 graph_name 解析目标图：指定名时在 UbergraphPages 和 FunctionGraphs 中查找；未指定时返回默认 EventGraph */
	UEdGraph* ResolveGraph(UBlueprint* BP, const FString& GraphName, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!BP)
		{
			OutErrorCode = TEXT("BP_NOT_FOUND");
			OutErrorMessage = TEXT("Blueprint is null");
			return nullptr;
		}

		if (!GraphName.IsEmpty())
		{
			for (UEdGraph* Graph : BP->UbergraphPages)
			{
				if (Graph && Graph->GetName() == GraphName) return Graph;
			}
			for (UEdGraph* Graph : BP->FunctionGraphs)
			{
				if (Graph && Graph->GetName() == GraphName) return Graph;
			}
			OutErrorCode = TEXT("GRAPH_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Graph not found: '%s'"), *GraphName);
			return nullptr;
		}

		return GetEventGraph(BP, OutErrorCode, OutErrorMessage);
	}

	/** 断开节点指定方向引脚的全部连接：通过 BreakAllPinLinks(true) 清除与该引脚链接的所有对端引脚关系 */
	bool DisconnectPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction, FString& OutErrorCode, FString& OutErrorMessage)
	{
		if (!Node)
		{
			OutErrorCode = TEXT("NODE_NOT_FOUND");
			OutErrorMessage = TEXT("Node is null");
			return false;
		}

		UEdGraphPin* Pin = FindPin(Node, PinName, Direction);
		if (!Pin)
		{
			OutErrorCode = TEXT("PIN_NOT_FOUND");
			OutErrorMessage = FString::Printf(TEXT("Pin '%s' not found on node"), *PinName);
			return false;
		}

		UEdGraph* Graph = Node->GetGraph();
		if (Graph)
		{
			UBlueprint* BP = Cast<UBlueprint>(Graph->GetOuter());
			if (BP) BP->Modify();
		}

		Pin->BreakAllPinLinks(true);
		return true;
	}
}

// ====== 1. blueprint_add_node_by_class ======
/** 通用 K2 节点工厂：根据 node_class 动态创建任意非专用 UEdGraphNode（Branch/Sequence/Comparison/转换等），排除 Event/CallFunction/Variable 节点 */
bool FMCPAddNodeByClassHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, NodeClass;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("node_class"), NodeClass))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and node_class are required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	FString GraphName;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("graph_name"), GraphName);

	UEdGraph* TargetGraph = MCPBlueprintGraphHelpers::ResolveGraph(BP, GraphName, OutErrorCode, OutErrorMessage);
	if (!TargetGraph) return false;

	UClass* NodeClassPtr = nullptr;
	if (!MCPBlueprintGraphHelpers::ValidateNodeClass(NodeClass, NodeClassPtr, OutErrorCode, OutErrorMessage)) return false;

	int32 PosX = 400, PosY = TargetGraph->Nodes.Num() * 200;
	if (Payload.IsValid())
	{
		Payload->TryGetNumberField(TEXT("pos_x"), PosX);
		Payload->TryGetNumberField(TEXT("pos_y"), PosY);
	}

	BP->Modify();
	UEdGraphNode* Node = MCPBlueprintGraphHelpers::CreateNodeByClass(TargetGraph, NodeClassPtr, PosX, PosY, OutErrorCode, OutErrorMessage);
	if (!Node) return false;

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	OutResult->SetStringField(TEXT("node_guid"), MCPBlueprintGraphHelpers::GuidToString(Node->NodeGuid));
	OutResult->SetStringField(TEXT("node_class"), NodeClassPtr->GetName());
	return true;
}

// ====== 2. blueprint_add_variable_node ======
/** 在图中创建 Get/Set Variable 节点：校验变量存在 → 根据 node_type 创建 UK2Node_VariableGet 或 UK2Node_VariableSet */
bool FMCPAddVariableNodeHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, VariableName, NodeType;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("variable_name"), VariableName) || !Payload->TryGetStringField(TEXT("node_type"), NodeType))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, variable_name, and node_type are required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	if (NodeType != TEXT("get") && NodeType != TEXT("set"))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("UNSUPPORTED_TYPE"), TEXT("node_type must be 'get' or 'set'"), OutErrorCode, OutErrorMessage);
		return false;
	}

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

	FString GraphName;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("graph_name"), GraphName);
	UEdGraph* TargetGraph = MCPBlueprintGraphHelpers::ResolveGraph(BP, GraphName, OutErrorCode, OutErrorMessage);
	if (!TargetGraph) return false;

	int32 PosX = 400, PosY = TargetGraph->Nodes.Num() * 200;
	if (Payload.IsValid())
	{
		Payload->TryGetNumberField(TEXT("pos_x"), PosX);
		Payload->TryGetNumberField(TEXT("pos_y"), PosY);
	}

	BP->Modify();

	FString NodeGuid;
	if (NodeType == TEXT("get"))
	{
		UK2Node_VariableGet* GetNode = MCPBlueprintGraphHelpers::CreateVariableGetNode(TargetGraph, FName(*VariableName), BP, PosX, PosY, OutErrorCode, OutErrorMessage);
		if (!GetNode) return false;
		NodeGuid = MCPBlueprintGraphHelpers::GuidToString(GetNode->NodeGuid);
	}
	else
	{
		UK2Node_VariableSet* SetNode = MCPBlueprintGraphHelpers::CreateVariableSetNode(TargetGraph, FName(*VariableName), BP, PosX, PosY, OutErrorCode, OutErrorMessage);
		if (!SetNode) return false;
		NodeGuid = MCPBlueprintGraphHelpers::GuidToString(SetNode->NodeGuid);
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	OutResult->SetStringField(TEXT("node_guid"), NodeGuid);
	OutResult->SetStringField(TEXT("variable_name"), VariableName);
	OutResult->SetStringField(TEXT("node_type"), NodeType);
	return true;
}

// ====== 3. blueprint_set_pin_default ======
/** 设置图节点输入引脚默认值：支持函数参数、分支条件、比较值等任意输入引脚 */
bool FMCPSetPinDefaultHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, NodeGuid, PinName, DefaultValue;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("node_guid"), NodeGuid) || !Payload->TryGetStringField(TEXT("pin_name"), PinName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, node_guid, and pin_name are required"), OutErrorCode, OutErrorMessage);
		return false;
	}
	Payload->TryGetStringField(TEXT("default_value"), DefaultValue);

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	FString GraphName;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("graph_name"), GraphName);
	UEdGraph* TargetGraph = MCPBlueprintGraphHelpers::ResolveGraph(BP, GraphName, OutErrorCode, OutErrorMessage);
	if (!TargetGraph) return false;

	UEdGraphNode* Node = MCPBlueprintGraphHelpers::FindNodeByGuid(TargetGraph, NodeGuid);
	if (!Node)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NODE_NOT_FOUND"), FString::Printf(TEXT("Node not found: '%s'"), *NodeGuid), OutErrorCode, OutErrorMessage);
		return false;
	}

	UEdGraphPin* Pin = MCPBlueprintGraphHelpers::FindPin(Node, PinName, EGPD_Input);
	if (!Pin)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PIN_NOT_FOUND"), FString::Printf(TEXT("Input pin '%s' not found"), *PinName), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();
	if (!MCPBlueprintGraphHelpers::SetPinDefaultValue(Pin, DefaultValue, OutErrorCode, OutErrorMessage)) return false;

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("node_guid"), NodeGuid);
	OutResult->SetStringField(TEXT("pin_name"), PinName);
	OutResult->SetStringField(TEXT("default_value"), Pin->DefaultValue);
	return true;
}

// ====== 4. blueprint_get_function_signature ======
/** 获取蓝图可调用函数的完整参数签名（参数名/方向/类型），用于 AI 在创建 CallFunction 节点前预知接口 */
bool FMCPGetFunctionSignatureHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString FunctionName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("function_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UFunction* Function = FindFirstObject<UFunction>(*FunctionName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!Function)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("FUNCTION_NOT_FOUND"), FString::Printf(TEXT("Function not found: '%s'"), *FunctionName), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult = MCPBlueprintGraphHelpers::BuildFunctionSignatureJson(Function);
	return true;
}

// ====== 5. blueprint_remove_node ======
/** 从图中删除指定 guid 的节点：Graph->RemoveNode 后标记结构修改 */
bool FMCPRemoveNodeHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, NodeGuid;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("node_guid"), NodeGuid))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and node_guid are required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	FString GraphName;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("graph_name"), GraphName);
	UEdGraph* TargetGraph = MCPBlueprintGraphHelpers::ResolveGraph(BP, GraphName, OutErrorCode, OutErrorMessage);
	if (!TargetGraph) return false;

	UEdGraphNode* Node = MCPBlueprintGraphHelpers::FindNodeByGuid(TargetGraph, NodeGuid);
	if (!Node)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NODE_NOT_FOUND"), FString::Printf(TEXT("Node not found: '%s'"), *NodeGuid), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();
	TargetGraph->RemoveNode(Node);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	OutResult->SetStringField(TEXT("node_guid"), NodeGuid);
	return true;
}

// ====== 6. blueprint_disconnect_pins ======
/** 断开节点指定引脚的全部连接：同时尝试 Output 和 Input 方向，通过 BreakAllPinLinks 清除所有链接 */
bool FMCPDisconnectPinsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, NodeGuid, PinName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("node_guid"), NodeGuid) || !Payload->TryGetStringField(TEXT("pin_name"), PinName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path, node_guid, and pin_name are required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	FString GraphName;
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("graph_name"), GraphName);
	UEdGraph* TargetGraph = MCPBlueprintGraphHelpers::ResolveGraph(BP, GraphName, OutErrorCode, OutErrorMessage);
	if (!TargetGraph) return false;

	UEdGraphNode* Node = MCPBlueprintGraphHelpers::FindNodeByGuid(TargetGraph, NodeGuid);
	if (!Node)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NODE_NOT_FOUND"), FString::Printf(TEXT("Node not found: '%s'"), *NodeGuid), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();

	MCPBlueprintGraphHelpers::DisconnectPin(Node, PinName, EGPD_Output, OutErrorCode, OutErrorMessage);
	MCPBlueprintGraphHelpers::DisconnectPin(Node, PinName, EGPD_Input, OutErrorCode, OutErrorMessage);

	FBlueprintEditorUtils::MarkBlueprintAsModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("node_guid"), NodeGuid);
	OutResult->SetStringField(TEXT("pin_name"), PinName);
	return true;
}

// ====== 7. blueprint_remove_variable ======
/** 从 Blueprint 删除成员变量：遍历 NewVariables 查找匹配名 → RemoveAt → 标记结构修改 */
bool FMCPRemoveVariableHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, VariableName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and variable_name are required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UBlueprint* BP = MCPBlueprintGraphHelpers::LoadBlueprint(AssetPath, OutErrorCode, OutErrorMessage);
	if (!BP) return false;

	int32 VarIndex = INDEX_NONE;
	for (int32 i = 0; i < BP->NewVariables.Num(); i++)
	{
		if (BP->NewVariables[i].VarName.ToString() == VariableName)
		{
			VarIndex = i;
			break;
		}
	}
	if (VarIndex == INDEX_NONE)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("VARIABLE_NOT_FOUND"), FString::Printf(TEXT("Variable '%s' not found"), *VariableName), OutErrorCode, OutErrorMessage);
		return false;
	}

	BP->Modify();
	BP->NewVariables.RemoveAt(VarIndex);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("asset_path"), AssetPath);
	OutResult->SetStringField(TEXT("variable_name"), VariableName);
	return true;
}
