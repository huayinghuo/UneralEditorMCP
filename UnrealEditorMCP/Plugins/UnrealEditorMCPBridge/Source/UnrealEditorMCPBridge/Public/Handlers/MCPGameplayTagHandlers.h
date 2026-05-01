#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

/** 创建或查找 GameplayTag（不存在则创建） */
class FMCPCreateGameplayTagHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("create_gameplay_tag"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 列出项目中所有 GameplayTag */
class FMCPListGameplayTagsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("list_gameplay_tags"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

/** 按名称搜索 GameplayTag */
class FMCPSearchGameplayTagsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("search_gameplay_tags"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
