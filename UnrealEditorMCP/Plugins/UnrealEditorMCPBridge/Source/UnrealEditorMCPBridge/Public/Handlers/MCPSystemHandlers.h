#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

/** 关闭当前活跃的 UE Editor 模态弹窗（由 MCP 客户端主动调用，非自动执行） */
class FMCPCloseModalWindowHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("close_modal_window"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
