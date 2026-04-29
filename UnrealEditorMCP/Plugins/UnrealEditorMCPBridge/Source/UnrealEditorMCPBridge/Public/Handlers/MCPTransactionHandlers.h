#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

// 供外部调用：断连等异常情况自动结束当前活跃事务
void MCPTransaction_AutoEndIfActive();

class FMCPBeginTransactionHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("begin_transaction"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPEndTransactionHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("end_transaction"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPUndoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("undo"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPRedoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("redo"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
