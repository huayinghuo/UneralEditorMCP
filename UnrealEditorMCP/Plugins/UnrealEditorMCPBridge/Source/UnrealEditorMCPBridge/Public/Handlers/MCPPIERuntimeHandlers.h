#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

/** 启动 Play In Editor（PIE）会话 */
class FMCPPIEStartHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("pie_start"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 停止 PIE 会话 */
class FMCPPIEStopHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("pie_stop"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 查询 PIE 会话是否正在运行 */
class FMCPPIEIsRunningHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("pie_is_running"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 获取 PIE 中 Actor 的运行时状态（位置/旋转/速度） */
class FMCPGetActorStateHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_actor_state"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 设置关卡/世界的默认 Pawn 类 */
class FMCPSetLevelDefaultPawnHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("set_level_default_pawn"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
