#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

class FMCPListBlueprintsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("list_blueprints"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPGetBlueprintInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_blueprint_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPCreateBlueprintHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("create_blueprint"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 添加 Blueprint 成员变量
class FMCPAddBlueprintVariableHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_variable"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 添加 Blueprint 函数/图表
class FMCPAddBlueprintFunctionHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_function"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/**
 * 向 Blueprint 的 SimpleConstructionScript 添加组件
 * @param asset_path   Blueprint 资源路径（必填）
 * @param component_class 组件类名，如 StaticMeshComponent、SceneComponent 等（必填）
 * @param component_name  组件名称，可选，默认使用类名
 */
class FMCPBlueprintAddComponentHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_component"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
