#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

/** 通用 K2 节点工厂：根据类名动态创建任意非专用 UEdGraphNode（Branch/Sequence/Comparison 等） */
class FMCPAddNodeByClassHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_node_by_class"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 在图中创建 Get/Set Variable 节点：用于在蓝图中读写成员变量 */
class FMCPAddVariableNodeHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_variable_node"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 设置图节点输入引脚默认值：支持函数参数、分支条件、比较值等任意输入引脚 */
class FMCPSetPinDefaultHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_set_pin_default"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 获取蓝图可调用函数的完整参数签名（参数名/方向/类型） */
class FMCPGetFunctionSignatureHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_get_function_signature"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 从图中删除指定 guid 的节点 */
class FMCPRemoveNodeHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_remove_node"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 断开节点指定引脚的全部连接 */
class FMCPDisconnectPinsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_disconnect_pins"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 从 Blueprint 中删除成员变量 */
class FMCPRemoveVariableHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_remove_variable"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
