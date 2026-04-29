#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

class FMCPListWidgetsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("list_widgets"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPGetWidgetInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_widget_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPCreateWidgetBlueprintHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("create_widget_blueprint"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 添加子控件到 Widget Blueprint 树中
// payload: asset_path, parent_widget（可选，默认 root）, widget_class（必填，如 "TextBlock"/"Button"）
class FMCPWidgetAddChildHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_add_child"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 从 Widget Blueprint 树中移除控件
// payload: asset_path, widget_name
class FMCPWidgetRemoveChildHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_remove_child"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 设置 Widget Blueprint 中控件的属性
// payload: asset_path, widget_name, property_name, value
class FMCPWidgetSetPropertyHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_set_property"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
