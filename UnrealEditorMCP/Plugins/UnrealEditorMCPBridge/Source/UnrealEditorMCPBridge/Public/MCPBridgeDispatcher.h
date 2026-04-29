#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

// MCP Bridge 请求调度器（表驱动 + token 校验）
class FMCPBridgeDispatcher
{
public:
	FMCPBridgeDispatcher() = default;

	void RegisterHandler(TSharedPtr<IMCPBridgeHandler> Handler);

	// 设置 token（空字符串 = 不校验，向后兼容 MVP 模式）
	void SetToken(const FString& InToken) { Token = InToken; }
	FString GetToken() const { return Token; }

	// 分发请求，token 不为空时对 Write/Dangerous 操作进行校验
	bool Dispatch(
		const FString& Action,
		TSharedPtr<FJsonObject> Payload,
		TSharedPtr<FJsonObject>& OutResult,
		FString& OutErrorCode,
		FString& OutErrorMessage);

	// 获取指定 action 的类别（用于 Python 端展示）
	EMCPActionCategory GetActionCategory(const FString& Action) const;

	// 返回所有注册的 action 及其类别、参数信息
	TArray<TSharedPtr<FJsonValue>> GetAllActionsInfo() const;

	// 获取 token 校验结果（nullptr 表示不需要 token 或未配置）
	bool GetTokenFromPayload(TSharedPtr<FJsonObject> Payload, FString& OutToken) const;

private:
	TMap<FString, TSharedPtr<IMCPBridgeHandler>> Handlers;
	FString Token;  // 配置的共享令牌（空 = 不校验）
};
