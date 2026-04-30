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

// ── Stage 16 Widget Deepening ──────────────────────────────────────────

/** 查询 Widget 的可编辑属性 schema（name/type/editable） */
class FMCPWidgetGetPropertySchemaHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_get_property_schema"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 查询 Widget 当前 slot 的可编辑属性 schema */
class FMCPWidgetGetSlotSchemaHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_get_slot_schema"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 按名称/类名/路径搜索 Widget 树中的节点 */
class FMCPWidgetFindHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_find"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 设置或替换 Widget Blueprint 的 root widget */
class FMCPWidgetSetRootHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_set_root"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 将 widget 移动到新的 parent 下（reparent） */
class FMCPWidgetReparentHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_reparent"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 调整同一 parent 下 children 的顺序 */
class FMCPWidgetReorderChildHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_reorder_child"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 重命名 Widget 树中的节点 */
class FMCPWidgetRenameHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_rename"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 设置 Widget Slot 布局属性 */
class FMCPWidgetSetSlotPropertyHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_set_slot_property"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 复制 Widget 或子树 */
class FMCPWidgetDuplicateHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_duplicate"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 用容器面板包裹现有 Widget */
class FMCPWidgetWrapWithPanelHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("widget_wrap_with_panel"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
