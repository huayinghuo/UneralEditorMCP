#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

class FMCPApplyBlueprintSpecHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_apply_spec"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPExportBlueprintSpecHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_export_spec"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
