#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "MCPBridgeHandler.h"

class FMCPListMaterialsHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("list_materials"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

class FMCPGetMaterialInfoHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("get_material_info"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Read; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 设置 Material Instance 标量参数
class FMCPSetMaterialScalarParamHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("material_set_scalar_param"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 设置 Material Instance 向量/颜色参数
class FMCPSetMaterialVectorParamHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("material_set_vector_param"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};

// 设置 Material Instance 纹理参数
class FMCPSetMaterialTextureParamHandler : public IMCPBridgeHandler
{
public:
	virtual FString GetActionName() const override { return TEXT("material_set_texture_param"); }
	virtual EMCPActionCategory GetCategory() const override { return EMCPActionCategory::Write; }
	virtual bool Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage) override;
};
