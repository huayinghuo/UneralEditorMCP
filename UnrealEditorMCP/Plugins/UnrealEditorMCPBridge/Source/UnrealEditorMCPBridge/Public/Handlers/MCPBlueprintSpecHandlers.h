#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

/** 从声明式 spec JSON 创建/重建 Blueprint 图：解析 spec → 创建 BP → 填充节点/边 → 保存（不自动编译） */
class FMCPApplyBlueprintSpecHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_apply_spec"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 将已有 BP 的 EventGraph 导出为 spec JSON 格式（节点 + 引脚 + 边 + 默认值），支持 round-trip */ 
class FMCPExportBlueprintSpecHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_export_spec"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
