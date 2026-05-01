#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

// ---- InputAction (5) ----
class FMCPSearchInputActionsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("search_input_actions"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPCreateInputActionHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("create_input_action"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPGetInputActionInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_input_action_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPDeleteInputActionHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("delete_input_action"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// ---- InputMappingContext (8) ----
class FMCPSearchIMCHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("search_input_mapping_contexts"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPCreateIMCHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("create_input_mapping_context"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPGetIMCInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_input_mapping_context_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPDeleteIMCHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("delete_input_mapping_context"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPAddMappingHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("add_input_mapping"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPRemoveMappingHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("remove_input_mapping"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPSetMappingActionHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("set_input_mapping_action"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPSetMappingKeyHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("set_input_mapping_key"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// ---- BP Enhanced Input nodes (3) ----
class FMCPBPEnhancedActionHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_enhanced_input_node"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
class FMCPBPIMCNodeHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("blueprint_add_imc_node"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
