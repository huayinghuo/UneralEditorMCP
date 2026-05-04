#include "Handlers/MCPGameplayTagHandlers.h"
#include "MCPBridgeHelpers.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsSettings.h"
#include "GameplayTagContainer.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPGT, Log, All);

// ====== 1. create_gameplay_tag ======
bool FMCPCreateGameplayTagHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString TagName, Desc; if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("tag_name"), TagName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("tag_name required"), OutErrorCode, OutErrorMessage); return false; }
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("description"), Desc);

	UGameplayTagsManager& Mgr = UGameplayTagsManager::Get();
	FGameplayTag ExistingTag = Mgr.RequestGameplayTag(FName(*TagName), false);
	bool bExisted = ExistingTag.IsValid();

	if (!bExisted)
	{
		// 直接写入 Settings CDO 的 GameplayTagList → SaveConfig 持久化 → EditorRefreshGameplayTagTree 重建树
		UGameplayTagsSettings* Settings = GetMutableDefault<UGameplayTagsSettings>();
		FGameplayTagTableRow NewRow(FName(*TagName), Desc);
		Settings->GameplayTagList.AddUnique(NewRow);
		// 持久化：先尝试 UpdateDefaultConfigFile，再 fallback 直接写文件
		FString ConfigPath = Settings->GetDefaultConfigFilename();
		Settings->TryUpdateDefaultConfigFile(ConfigPath);
		// 重建 tag 树，使新 tag 立即生效（内部从 CDO 的 GameplayTagList 读）
#if WITH_EDITOR
		Mgr.EditorRefreshGameplayTagTree();
#endif
	}
	FGameplayTag Tag = Mgr.RequestGameplayTag(FName(*TagName), true);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("tag_name"), TagName); OutResult->SetBoolField(TEXT("valid"), Tag.IsValid());
	OutResult->SetBoolField(TEXT("created"), !bExisted); OutResult->SetStringField(TEXT("tag"), Tag.ToString());
	return true;
}

// ====== 2. list_gameplay_tags ======
bool FMCPListGameplayTagsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString Prefix; if (Payload.IsValid()) Payload->TryGetStringField(TEXT("prefix"), Prefix);
	int32 Max = 200; if (Payload.IsValid()) Payload->TryGetNumberField(TEXT("max_results"), Max);
	UGameplayTagsManager& Mgr = UGameplayTagsManager::Get();
	TArray<TSharedPtr<FJsonValue>> Results;

	FGameplayTagContainer AllTags;
	Mgr.RequestAllGameplayTags(AllTags, true);
	FString PrefixLower = Prefix.ToLower();
	for (const FGameplayTag& Tag : AllTags)
	{
		FString TagStr = Tag.ToString();
		if (!PrefixLower.IsEmpty() && !TagStr.ToLower().StartsWith(PrefixLower)) continue;
		if (Results.Num() >= Max) break;
		TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject()); O->SetStringField(TEXT("name"), TagStr);
		Results.Add(MakeShareable(new FJsonValueObject(O)));
	}
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetNumberField(TEXT("count"), Results.Num()); OutResult->SetArrayField(TEXT("tags"), Results);
	return true;
}

// ====== 3. search_gameplay_tags ======
bool FMCPSearchGameplayTagsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ST; int32 Max = 50; if (Payload.IsValid()) { Payload->TryGetStringField(TEXT("search_term"), ST); Payload->TryGetNumberField(TEXT("max_results"), Max); }
	UGameplayTagsManager& Mgr = UGameplayTagsManager::Get();
	TArray<TSharedPtr<FJsonValue>> Results;

	FGameplayTagContainer AllTags;
	Mgr.RequestAllGameplayTags(AllTags, true);
	for (const FGameplayTag& Tag : AllTags)
	{
		FString TagStr = Tag.ToString();
		if (!TagStr.Contains(ST, ESearchCase::IgnoreCase)) continue;
		if (Results.Num() >= Max) break;
		TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject()); O->SetStringField(TEXT("name"), TagStr);
		Results.Add(MakeShareable(new FJsonValueObject(O)));
	}
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetNumberField(TEXT("count"), Results.Num()); OutResult->SetArrayField(TEXT("tags"), Results);
	return true;
}
