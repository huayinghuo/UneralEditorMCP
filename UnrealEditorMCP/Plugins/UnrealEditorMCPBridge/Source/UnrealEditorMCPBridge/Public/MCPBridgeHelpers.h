#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class AActor;
class UActorComponent;
class UWorld;

// MCP Bridge 公共辅助函数
namespace MCPBridgeHelpers
{
	UWorld* GetEditorWorld();
	AActor* FindActorByNameOrLabel(UWorld* World, const FString& Name);
	UActorComponent* FindComponentByNameOrClass(AActor* Actor, const FString& Name);
	TSharedPtr<FJsonObject> BuildActorJson(AActor* Actor, bool bIncludeTransform = false);
	bool DestroyActorInEditor(AActor* Actor);

	// 通过 UE 反射读取属性值（ExportText），OutFound 指示属性是否存在
	FString GetActorPropertyAsString(AActor* Actor, const FString& PropertyName, bool& OutFound);
	FString GetComponentPropertyAsString(UActorComponent* Component, const FString& PropertyName, bool& OutFound);

	// 通过 UE 反射设置属性值（ImportText），返回实际写入是否成功
	bool SetActorPropertyFromString(AActor* Actor, const FString& PropertyName, const FString& ValueStr);
	bool SetComponentPropertyFromString(UActorComponent* Component, const FString& PropertyName, const FString& ValueStr);

	AActor* SpawnActorInEditor(UWorld* World, const FString& ClassName, const FVector& Location, const FRotator& Rotation, const FVector& Scale);
	FVector ParseVectorOptional(TSharedPtr<FJsonObject> Payload, const FString& Key, const FVector& Default);

	// rotation 统一约定：{pitch, yaw, roll} → FRotator(Pitch=Y, Yaw=Z, Roll=X)
	FRotator ParseRotatorFromPayload(TSharedPtr<FJsonObject> Payload, const FString& Key, const FRotator& Default);

	void BuildErrorResponse(const FString& Code, const FString& Message, FString& OutErrorCode, FString& OutErrorMessage);
}
