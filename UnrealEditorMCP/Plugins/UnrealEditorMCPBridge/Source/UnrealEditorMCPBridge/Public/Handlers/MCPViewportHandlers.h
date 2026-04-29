#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

class FMCPViewportScreenshotHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("viewport_screenshot"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }  // 写磁盘，非纯读
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
