#include "Handlers/MCPGameplayTagHandlers.h"
#include "MCPBridgeHelpers.h"
#include "GameplayTagsManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPGT, Log, All);

// ====== 1. create_gameplay_tag ======
bool FMCPCreateGameplayTagHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString TagName, Desc; if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("tag_name"), TagName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("tag_name required"), OutErrorCode, OutErrorMessage); return false; }
	if (Payload.IsValid()) Payload->TryGetStringField(TEXT("description"), Desc);

	UGameplayTagsManager& Mgr = UGameplayTagsManager::Get();
	bool bExisted = Mgr.RequestGameplayTag(FName(*TagName), false).IsValid();
	if (!bExisted)
	{
		Mgr.AddNativeGameplayTag(FName(*TagName), Desc);
		// 持久化：写入 DefaultGameplayTags.ini（重启后保留）
		FString ConfigPath = FPaths::ProjectConfigDir() + TEXT("DefaultGameplayTags.ini");
		TArray<FString> ExistingTags;
		GConfig->GetArray(TEXT("/Script/GameplayTags.GameplayTagsSettings"), TEXT("GameplayTagList"), ExistingTags, ConfigPath);
		if (!ExistingTags.Contains(TagName))
		{
			ExistingTags.Add(TagName);
			GConfig->SetArray(TEXT("/Script/GameplayTags.GameplayTagsSettings"), TEXT("GameplayTagList"), ExistingTags, ConfigPath);
			GConfig->Flush(false, ConfigPath);
		}
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
	UGameplayTagsManager& Mgr = UGameplayTagsManager::Get();
	TArray<TSharedPtr<FJsonValue>> Results;
	int32 Count = 0, Max = 200; if (Payload.IsValid()) Payload->TryGetNumberField(TEXT("max_results"), Max);

	// 对已知前缀进行模式匹配查询（GAS 常用前缀）
	TArray<FString> Patterns;
	if (Prefix.IsEmpty()) { Patterns.Add(TEXT("Ability")); Patterns.Add(TEXT("State")); Patterns.Add(TEXT("Status")); Patterns.Add(TEXT("Cooldown")); Patterns.Add(TEXT("Effect")); Patterns.Add(TEXT("Damage")); Patterns.Add(TEXT("Attribute")); Patterns.Add(TEXT("InputTag")); Patterns.Add(TEXT("GameplayCue")); Patterns.Add(TEXT("Event")); }
	else { Patterns.Add(Prefix); }

	for (const FString& P : Patterns)
	{
		for (int i = 0; i < 50; i++)
		{
			FString TN = FString::Printf(TEXT("%s.%d"), *P, i);
			FGameplayTag T = Mgr.RequestGameplayTag(FName(*TN), false);
			if (!T.IsValid()) break;
			if (Count >= Max) break; Count++;
			TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject()); O->SetStringField(TEXT("name"), T.ToString());
			Results.Add(MakeShareable(new FJsonValueObject(O)));
		}
		if (Count >= Max) break;
	}
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetNumberField(TEXT("count"), Count); OutResult->SetArrayField(TEXT("tags"), Results);
	return true;
}

// ====== 3. search_gameplay_tags ======
bool FMCPSearchGameplayTagsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ST; int32 Max = 50; if (Payload.IsValid()) { Payload->TryGetStringField(TEXT("search_term"), ST); Payload->TryGetNumberField(TEXT("max_results"), Max); }
	UGameplayTagsManager& Mgr = UGameplayTagsManager::Get();
	TArray<TSharedPtr<FJsonValue>> Results;

	// 直接查询 tag_name 是否为已注册 tag
	FGameplayTag Direct = Mgr.RequestGameplayTag(FName(*ST), false);
	if (Direct.IsValid())
	{
		TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject()); O->SetStringField(TEXT("name"), Direct.ToString());
		Results.Add(MakeShareable(new FJsonValueObject(O)));
	}

	// 补充通配符查询：如果 search_term 不含点号，则用 "." 前缀匹配
	if (!ST.Contains(TEXT(".")))
	{
		for (int i = 0; i < 20 && Results.Num() < Max; i++)
		{
			FString TN = FString::Printf(TEXT("%s.%d"), *ST, i);
			FGameplayTag T = Mgr.RequestGameplayTag(FName(*TN), false);
			if (!T.IsValid()) break;
			TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject()); O->SetStringField(TEXT("name"), T.ToString());
			Results.Add(MakeShareable(new FJsonValueObject(O)));
		}
	}
	OutResult = MakeShareable(new FJsonObject()); OutResult->SetNumberField(TEXT("count"), Results.Num()); OutResult->SetArrayField(TEXT("tags"), Results);
	return true;
}
