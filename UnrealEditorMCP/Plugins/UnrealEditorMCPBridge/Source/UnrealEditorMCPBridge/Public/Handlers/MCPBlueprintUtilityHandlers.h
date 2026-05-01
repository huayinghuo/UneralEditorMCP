#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

/** 搜索 BlueprintCallable 函数：按关键字 + 可选类过滤，返回匹配函数的签名列表 */
class FMCPBlueprintSearchFunctionsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_search_functions"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 设置 Blueprint 成员变量的 CDO 默认值 */
class FMCPBlueprintSetVariableDefaultHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_set_variable_default"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 设置 Blueprint 组件的默认属性值 */
class FMCPBlueprintSetComponentDefaultHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_set_component_default"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// ---- 阶段 19 CDO 通用属性 ----

/** 读取 Blueprint CDO 任意属性：反射遍历 → ExportText → JSON */
class FMCPBlueprintGetCDOPropertyHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_get_cdo_property"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 设置 Blueprint CDO 任意属性：ImportText 写入 → read-back */
class FMCPBlueprintSetCDOPropertyHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_set_cdo_property"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 向 Blueprint CDO TArray 属性添加元素（struct 类型用 JSON 描述） */
class FMCPBlueprintAddCDOArrayHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_cdo_array"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 从 Blueprint CDO TArray 属性删除元素（按 index） */
class FMCPBlueprintRemoveCDOArrayHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_remove_cdo_array"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
