#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

// 执行 Python 片段 —— Dangerous 级别（可执行任意代码）
class FMCPExecutePythonHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("execute_python_snippet"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Dangerous; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
