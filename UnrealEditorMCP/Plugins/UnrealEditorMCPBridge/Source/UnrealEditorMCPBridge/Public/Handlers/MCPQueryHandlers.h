#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

class FMCPBridgeServer;

// 连通性检测
class FMCPPingHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("ping"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 编辑器信息
class FMCPGetEditorInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_editor_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 项目信息
class FMCPGetProjectInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_project_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 世界状态
class FMCPGetWorldStateHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_world_state"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 列出资产
class FMCPListAssetsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("list_assets"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 获取资产信息
class FMCPGetAssetInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_asset_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 获取 MCP 配置契约 —— 返回所有 action 及其分类，作为 C++/Python 统一来源
class FMCPGetMCPConfigHandler : public IMCPBridgeHandler
{
public:
	explicit FMCPGetMCPConfigHandler(FMCPBridgeServer* InServer) : Server(InServer) {}
	virtual FString GetActionName() const override { return TEXT("get_mcp_config"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
private:
	FMCPBridgeServer* Server;
};

/**
 * 桥接运行时状态诊断查询 —— 返回传输层实时状态
 * 供 MCP 客户端在不查看 UE 日志的情况下判断服务是否就绪、是否被占用、最近故障原因
 */
class FMCPGetBridgeRuntimeStatusHandler : public IMCPBridgeHandler
{
public:
	explicit FMCPGetBridgeRuntimeStatusHandler(FMCPBridgeServer* InServer) : Server(InServer) {}
	virtual FString GetActionName() const override { return TEXT("get_bridge_runtime_status"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
private:
	FMCPBridgeServer* Server;
};
