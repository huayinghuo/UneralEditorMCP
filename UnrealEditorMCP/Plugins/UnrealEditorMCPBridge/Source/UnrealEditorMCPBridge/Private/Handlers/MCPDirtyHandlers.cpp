#include "Handlers/MCPDirtyHandlers.h"
#include "MCPBridgeHelpers.h"
#include "Containers/Ticker.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPDirty, Log, All);

bool FMCPGetDirtyPackagesHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> DirtyArray;

	// 遍历所有 UPackage 检查 IsDirty
	for (TObjectIterator<UPackage> It; It; ++It)
	{
		UPackage* Package = *It;
		if (Package && Package->IsDirty() && !Package->HasAnyFlags(RF_Transient))
		{
			TSharedPtr<FJsonObject> PkgObj = MakeShareable(new FJsonObject());
			PkgObj->SetStringField(TEXT("name"), Package->GetName());
			PkgObj->SetStringField(TEXT("file"), Package->GetLoadedPath().GetLocalFullPath());
			DirtyArray.Add(MakeShareable(new FJsonValueObject(PkgObj)));
		}
	}

	OutResult->SetArrayField(TEXT("dirty_packages"), DirtyArray);
	OutResult->SetNumberField(TEXT("count"), DirtyArray.Num());
	return true;
}
