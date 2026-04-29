#include "Handlers/MCPMaterialHandlers.h"
#include "MCPBridgeHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/Texture.h"
#include "MaterialEditingLibrary.h"
#include "Engine/Texture.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPMaterial, Log, All);

// ====== ListMaterials ======
bool FMCPListMaterialsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString SearchPath = TEXT("/Game");
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("path"), SearchPath);
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(*SearchPath);
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
	Filter.ClassPaths.Add(UMaterialInstance::StaticClass()->GetClassPathName());

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	OutResult = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> MatsArray;

	for (const FAssetData& Asset : AssetList)
	{
		TSharedPtr<FJsonObject> MatObj = MakeShareable(new FJsonObject());
		MatObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		MatObj->SetStringField(TEXT("class"), Asset.AssetClassPath.GetAssetName().ToString());
		MatObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		MatObj->SetStringField(TEXT("package"), Asset.PackageName.ToString());
		MatsArray.Add(MakeShareable(new FJsonValueObject(MatObj)));
	}

	OutResult->SetArrayField(TEXT("materials"), MatsArray);
	OutResult->SetNumberField(TEXT("count"), MatsArray.Num());
	OutResult->SetStringField(TEXT("path"), SearchPath);
	return true;
}

// ====== GetMaterialInfo ======
bool FMCPGetMaterialInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *AssetPath);
	if (!Material)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MATERIAL_NOT_FOUND"), FString::Printf(TEXT("Material not found: '%s'"), *AssetPath), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("name"), Material->GetName());
	OutResult->SetStringField(TEXT("path"), Material->GetPathName());
	OutResult->SetStringField(TEXT("class"), Material->GetClass()->GetName());
	OutResult->SetBoolField(TEXT("is_instance"), Material->IsA(UMaterialInstance::StaticClass()));

	// Material Instance 返回父材质与标量/向量/纹理参数
	if (UMaterialInstance* MI = Cast<UMaterialInstance>(Material))
	{
		if (MI->Parent)
		{
			OutResult->SetStringField(TEXT("parent"), MI->Parent->GetPathName());
		}

		// 标量参数
		TArray<TSharedPtr<FJsonValue>> ScalarArray;
		TArray<FMaterialParameterInfo> ScalarInfo;
		TArray<FGuid> ScalarGuids;
		MI->GetAllScalarParameterInfo(ScalarInfo, ScalarGuids);
		for (const FMaterialParameterInfo& Info : ScalarInfo)
		{
			float Value;
			if (MI->GetScalarParameterValue(Info, Value))
			{
				TSharedPtr<FJsonObject> ParamObj = MakeShareable(new FJsonObject());
				ParamObj->SetStringField(TEXT("name"), Info.Name.ToString());
				ParamObj->SetNumberField(TEXT("value"), Value);
				ScalarArray.Add(MakeShareable(new FJsonValueObject(ParamObj)));
			}
		}
		OutResult->SetArrayField(TEXT("scalar_params"), ScalarArray);

		// 向量参数
		TArray<TSharedPtr<FJsonValue>> VectorArray;
		TArray<FMaterialParameterInfo> VectorInfo;
		TArray<FGuid> VectorGuids;
		MI->GetAllVectorParameterInfo(VectorInfo, VectorGuids);
		for (const FMaterialParameterInfo& Info : VectorInfo)
		{
			FLinearColor Value;
			if (MI->GetVectorParameterValue(Info, Value))
			{
				TSharedPtr<FJsonObject> ParamObj = MakeShareable(new FJsonObject());
				ParamObj->SetStringField(TEXT("name"), Info.Name.ToString());
				ParamObj->SetStringField(TEXT("value"),
					FString::Printf(TEXT("R=%.3f G=%.3f B=%.3f A=%.3f"), Value.R, Value.G, Value.B, Value.A));
				VectorArray.Add(MakeShareable(new FJsonValueObject(ParamObj)));
			}
		}
		OutResult->SetArrayField(TEXT("vector_params"), VectorArray);

		// 纹理参数
		TArray<TSharedPtr<FJsonValue>> TexArray;
		TArray<FMaterialParameterInfo> TexInfo;
		TArray<FGuid> TexGuids;
		MI->GetAllTextureParameterInfo(TexInfo, TexGuids);
		for (const FMaterialParameterInfo& Info : TexInfo)
		{
			UTexture* Texture = nullptr;
			if (MI->GetTextureParameterValue(Info, Texture) && Texture)
			{
				TSharedPtr<FJsonObject> ParamObj = MakeShareable(new FJsonObject());
				ParamObj->SetStringField(TEXT("name"), Info.Name.ToString());
				ParamObj->SetStringField(TEXT("texture"), Texture->GetPathName());
				TexArray.Add(MakeShareable(new FJsonValueObject(ParamObj)));
			}
		}
		OutResult->SetArrayField(TEXT("texture_params"), TexArray);
	}

	// Blend Mode / Shading Model（仅 UMaterial）
	if (UMaterial* M = Cast<UMaterial>(Material))
	{
		OutResult->SetStringField(TEXT("blend_mode"), UEnum::GetValueAsString(M->BlendMode));
		OutResult->SetBoolField(TEXT("two_sided"), M->TwoSided);
		OutResult->SetStringField(TEXT("material_domain"), UEnum::GetValueAsString(M->MaterialDomain));
	}

	return true;
}

// ====== SetScalarParam ======
bool FMCPSetMaterialScalarParamHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, ParamName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("param_name"), ParamName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and param_name required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UMaterialInstanceConstant* MIC = LoadObject<UMaterialInstanceConstant>(nullptr, *AssetPath);
	if (!MIC)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MATERIAL_NOT_FOUND"), FString::Printf(TEXT("'%s' not found or not a MaterialInstanceConstant"), *AssetPath), OutErrorCode, OutErrorMessage); return false; }

	double Value = 0.0;
	if (Payload->TryGetNumberField(TEXT("value"), Value))
	{
		MIC->Modify();
		UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(MIC, *ParamName, static_cast<float>(Value));
	}

	float ReadBack;
	bool bFound = MIC->GetScalarParameterValue(FName(*ParamName), ReadBack);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("material"), MIC->GetName());
	OutResult->SetStringField(TEXT("param_name"), ParamName);
	OutResult->SetNumberField(TEXT("value"), ReadBack);
	OutResult->SetBoolField(TEXT("found"), bFound);

	if (!bFound)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PARAM_NOT_FOUND"), FString::Printf(TEXT("Scalar parameter '%s' not found"), *ParamName), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}

// ====== SetVectorParam ======
bool FMCPSetMaterialVectorParamHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, ParamName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("param_name"), ParamName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and param_name required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UMaterialInstanceConstant* MIC = LoadObject<UMaterialInstanceConstant>(nullptr, *AssetPath);
	if (!MIC)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MATERIAL_NOT_FOUND"), FString::Printf(TEXT("'%s' not found or not a MaterialInstanceConstant"), *AssetPath), OutErrorCode, OutErrorMessage); return false; }

	FLinearColor Color(1,1,1,1);
	Payload->TryGetNumberField(TEXT("r"), Color.R);
	Payload->TryGetNumberField(TEXT("g"), Color.G);
	Payload->TryGetNumberField(TEXT("b"), Color.B);
	Payload->TryGetNumberField(TEXT("a"), Color.A);
	MIC->Modify();
	UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(MIC, *ParamName, Color);

	FLinearColor ReadBack;
	bool bFound = MIC->GetVectorParameterValue(FName(*ParamName), ReadBack);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("material"), MIC->GetName());
	OutResult->SetStringField(TEXT("param_name"), ParamName);
	OutResult->SetStringField(TEXT("value"), FString::Printf(TEXT("R=%.3f G=%.3f B=%.3f A=%.3f"), ReadBack.R, ReadBack.G, ReadBack.B, ReadBack.A));
	OutResult->SetBoolField(TEXT("found"), bFound);

	if (!bFound)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PARAM_NOT_FOUND"), FString::Printf(TEXT("Vector parameter '%s' not found"), *ParamName), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}

// ====== SetTextureParam ======
bool FMCPSetMaterialTextureParamHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath, ParamName, TexturePath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath) || !Payload->TryGetStringField(TEXT("param_name"), ParamName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("asset_path and param_name required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UMaterialInstanceConstant* MIC = LoadObject<UMaterialInstanceConstant>(nullptr, *AssetPath);
	if (!MIC)
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MATERIAL_NOT_FOUND"), FString::Printf(TEXT("'%s' not found or not a MaterialInstanceConstant"), *AssetPath), OutErrorCode, OutErrorMessage); return false; }

	if (Payload->TryGetStringField(TEXT("texture_path"), TexturePath))
	{
		UTexture* Tex = LoadObject<UTexture>(nullptr, *TexturePath);
		if (Tex)
		{
			MIC->Modify();
			UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(MIC, *ParamName, Tex);
		}
	}

	UTexture* ReadBackTexture = nullptr;
	bool bFound = MIC->GetTextureParameterValue(FName(*ParamName), ReadBackTexture);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("material"), MIC->GetName());
	OutResult->SetStringField(TEXT("param_name"), ParamName);
	if (ReadBackTexture)
		OutResult->SetStringField(TEXT("texture"), ReadBackTexture->GetPathName());
	OutResult->SetBoolField(TEXT("found"), bFound);

	if (!bFound)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PARAM_NOT_FOUND"), FString::Printf(TEXT("Texture parameter '%s' not found"), *ParamName), OutErrorCode, OutErrorMessage);
		return false;
	}
	return true;
}
