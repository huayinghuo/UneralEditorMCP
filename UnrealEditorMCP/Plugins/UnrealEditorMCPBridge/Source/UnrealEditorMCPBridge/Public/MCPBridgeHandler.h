#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Handler 操作风险等级
enum class EMCPActionCategory : uint8
{
	Read,      // 只读查询，不修改引擎状态
	Write,     // 写入操作，修改关卡/资产数据
	Dangerous  // 高危操作（Python 执行、事务控制等），需单独授权
};

// MCP Bridge Handler 接口
class IMCPBridgeHandler
{
public:
	virtual ~IMCPBridgeHandler() = default;

	// 返回该 Handler 对应的 action 名称
	virtual FString GetActionName() const = 0;

	// 返回操作风险等级
	virtual EMCPActionCategory GetCategory() const = 0;

	// 执行请求，填充结果或错误
	virtual bool Execute(
		TSharedPtr<FJsonObject> Payload,
		TSharedPtr<FJsonObject>& OutResult,
		FString& OutErrorCode,
		FString& OutErrorMessage) = 0;
};
