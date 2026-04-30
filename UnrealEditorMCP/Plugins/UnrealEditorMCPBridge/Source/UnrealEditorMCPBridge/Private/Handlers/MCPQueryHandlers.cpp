#include "Handlers/MCPQueryHandlers.h"
#include "MCPBridgeHelpers.h"
#include "MCPBridgeServer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/EngineVersion.h"
#include "Editor.h"
#include "Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

// ====== Ping ======
bool FMCPPingHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("message"), TEXT("pong"));
	return true;
}

// ====== GetEditorInfo ======
bool FMCPGetEditorInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
	OutResult->SetBoolField(TEXT("is_editor"), true);
	return true;
}

// ====== GetProjectInfo ======
bool FMCPGetProjectInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("project_name"), FApp::GetProjectName());
	OutResult->SetStringField(TEXT("project_dir"), FPaths::ProjectDir());
	return true;
}

// ====== GetWorldState ======
bool FMCPGetWorldStateHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());

	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (!World)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_WORLD"), TEXT("No editor world available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 统计 Actor 数量
	int32 ActorCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		ActorCount++;
	}

	// 统计选中的 Actor 数量
	int32 SelectedCount = 0;
	if (GEditor)
	{
		USelection* Selection = GEditor->GetSelectedActors();
		if (Selection)
		{
			SelectedCount = Selection->Num();
		}
	}

	OutResult->SetStringField(TEXT("world_name"), World->GetName());
	OutResult->SetNumberField(TEXT("actor_count"), ActorCount);
	OutResult->SetNumberField(TEXT("selected_count"), SelectedCount);
	OutResult->SetBoolField(TEXT("has_world_partition"), World->IsPartitionedWorld());

	return true;
}

// ====== ListAssets（新增：AssetRegistry 资产列表）======
bool FMCPListAssetsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath = TEXT("/Game");
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("path"), AssetPath);
	}

	bool bRecursive = true;
	if (Payload.IsValid())
	{
		Payload->TryGetBoolField(TEXT("recursive"), bRecursive);
	}

	FString ClassFilter;
	if (Payload.IsValid())
	{
		Payload->TryGetStringField(TEXT("class_name"), ClassFilter);
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(*AssetPath);
	Filter.bRecursivePaths = bRecursive;
	if (!ClassFilter.IsEmpty())
	{
		Filter.ClassPaths.Add(FTopLevelAssetPath(*ClassFilter));
	}

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	OutResult = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> AssetsArray;

	for (const FAssetData& Asset : AssetList)
	{
		TSharedPtr<FJsonObject> AssetObj = MakeShareable(new FJsonObject());
		AssetObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		AssetObj->SetStringField(TEXT("class"), Asset.AssetClassPath.GetAssetName().ToString());
		AssetObj->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		AssetObj->SetStringField(TEXT("package"), Asset.PackageName.ToString());
		AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
	}

	OutResult->SetArrayField(TEXT("assets"), AssetsArray);
	OutResult->SetNumberField(TEXT("count"), AssetsArray.Num());
	OutResult->SetStringField(TEXT("path"), AssetPath);

	return true;
}

// ====== GetAssetInfo（新增：单个资产详情）======
bool FMCPGetAssetInfoHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString AssetPath;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("asset_path"), AssetPath))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.asset_path is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	if (!AssetData.IsValid())
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("ASSET_NOT_FOUND"), FString::Printf(TEXT("Asset '%s' not found"), *AssetPath), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
	OutResult->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());
	OutResult->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
	OutResult->SetStringField(TEXT("package"), AssetData.PackageName.ToString());

	TSharedPtr<FJsonObject> TagsObj = MakeShareable(new FJsonObject());
	for (const TPair<FName, FAssetTagValueRef>& Tag : AssetData.TagsAndValues)
	{
		TagsObj->SetStringField(Tag.Key.ToString(), Tag.Value.AsString());
	}
	OutResult->SetObjectField(TEXT("tags"), TagsObj);

	return true;
}

// ====== GetMCPConfig（统一 action registry）======
bool FMCPGetMCPConfigHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());

	if (Server)
	{
		OutResult->SetArrayField(TEXT("actions"), Server->GetAllActionsInfo());
		OutResult->SetNumberField(TEXT("count"), Server->GetAllActionsInfo().Num());

		// 模式信息：默认开发模式（token 未启用）
		bool bTokenEnabled = !Server->GetToken().IsEmpty();
		OutResult->SetStringField(TEXT("mode"), bTokenEnabled ? TEXT("restricted") : TEXT("development"));
		OutResult->SetBoolField(TEXT("token_enabled"), bTokenEnabled);

		OutResult->SetStringField(TEXT("version"), TEXT("1.0.0"));
		OutResult->SetStringField(TEXT("bridge_protocol"), TEXT("TCP/JSON Lines"));
		OutResult->SetStringField(TEXT("ue_version"), FEngineVersion::Current().ToString());
	}

	return true;
}

// ====== GetBridgeRuntimeStatus（阶段 12A 诊断查询）======
bool FMCPGetBridgeRuntimeStatusHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	if (!Server)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("INTERNAL_ERROR"), TEXT("Server reference is null"), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult = MakeShareable(new FJsonObject());

	// 状态字符串映射
	const TCHAR* StatusStr = TEXT("Unknown");
	switch (Server->GetStatus())
	{
	case EMCPBridgeServerStatus::Unstarted:  StatusStr = TEXT("Unstarted");  break;
	case EMCPBridgeServerStatus::Listening:  StatusStr = TEXT("Listening");  break;
	case EMCPBridgeServerStatus::Connected:  StatusStr = TEXT("Connected");  break;
	case EMCPBridgeServerStatus::Error:      StatusStr = TEXT("Error");      break;
	case EMCPBridgeServerStatus::Stopped:    StatusStr = TEXT("Stopped");    break;
	}

	OutResult->SetStringField(TEXT("server_status"), StatusStr);
	OutResult->SetNumberField(TEXT("port"), Server->GetServerPort());
	OutResult->SetBoolField(TEXT("token_enabled"), !Server->GetToken().IsEmpty());
	OutResult->SetBoolField(TEXT("client_connected"), Server->IsClientConnected());
	OutResult->SetStringField(TEXT("last_error_code"), Server->GetLastErrorCode());
	OutResult->SetStringField(TEXT("last_error_message"), Server->GetLastErrorMessage());
	OutResult->SetStringField(TEXT("transport_mode"), TEXT("tcp-jsonlines"));
	OutResult->SetStringField(TEXT("bind_address"), TEXT("127.0.0.1"));

	return true;
}
