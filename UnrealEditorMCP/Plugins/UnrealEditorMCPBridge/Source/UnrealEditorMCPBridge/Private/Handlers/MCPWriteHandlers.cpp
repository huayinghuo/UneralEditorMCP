#include "Handlers/MCPWriteHandlers.h"
#include "MCPBridgeHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "FileHelpers.h"

// ====== SpawnActor ======
bool FMCPSpawnActorHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ClassName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("class_name"), ClassName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.class_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (!World)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_WORLD"), TEXT("No editor world available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 解析可选的变换参数，rotation 使用统一 {pitch, yaw, roll} 语义
	FVector Location = MCPBridgeHelpers::ParseVectorOptional(Payload, TEXT("location"), FVector::ZeroVector);
	FRotator Rotation = MCPBridgeHelpers::ParseRotatorFromPayload(Payload, TEXT("rotation"), FRotator::ZeroRotator);
	FVector Scale = MCPBridgeHelpers::ParseVectorOptional(Payload, TEXT("scale"), FVector(1.0f, 1.0f, 1.0f));

	AActor* Actor = MCPBridgeHelpers::SpawnActorInEditor(World, ClassName, Location, Rotation, Scale);
	if (!Actor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SPAWN_FAILED"), FString::Printf(TEXT("Failed to spawn actor of class '%s'"), *ClassName), OutErrorCode, OutErrorMessage);
		return false;
	}

	// 返回生成后的 Actor 信息（含 Transform）
	OutResult = MCPBridgeHelpers::BuildActorJson(Actor, true);
	OutResult->SetStringField(TEXT("status"), TEXT("spawned"));
	return true;
}

// ====== SetActorTransform ======
bool FMCPLevelSetActorTransformHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ActorName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.actor_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (!World)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_WORLD"), TEXT("No editor world available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	AActor* Actor = MCPBridgeHelpers::FindActorByNameOrLabel(World, ActorName);
	if (!Actor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("ACTOR_NOT_FOUND"), FString::Printf(TEXT("Actor '%s' not found"), *ActorName), OutErrorCode, OutErrorMessage);
		return false;
	}

	// rotation 使用统一 {pitch, yaw, roll} 语义，未提供的保持原值
	FVector Location = MCPBridgeHelpers::ParseVectorOptional(Payload, TEXT("location"), Actor->GetActorLocation());
	FRotator Rotation = MCPBridgeHelpers::ParseRotatorFromPayload(Payload, TEXT("rotation"), Actor->GetActorRotation());
	FVector Scale = MCPBridgeHelpers::ParseVectorOptional(Payload, TEXT("scale"), Actor->GetActorScale3D());

	Actor->Modify();
	Actor->SetActorLocation(Location);
	Actor->SetActorRotation(Rotation);
	Actor->SetActorScale3D(Scale);

	// 返回更新后的变换信息
	OutResult = MCPBridgeHelpers::BuildActorJson(Actor, true);
	OutResult->SetStringField(TEXT("status"), TEXT("transformed"));
	return true;
}

// ====== SetActorProperty ======
bool FMCPActorSetPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ActorName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.actor_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString PropertyName;
	if (!Payload->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.property_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString Value;
	if (!Payload->TryGetStringField(TEXT("value"), Value))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.value is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (!World)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_WORLD"), TEXT("No editor world available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	AActor* Actor = MCPBridgeHelpers::FindActorByNameOrLabel(World, ActorName);
	if (!Actor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("ACTOR_NOT_FOUND"), FString::Printf(TEXT("Actor '%s' not found"), *ActorName), OutErrorCode, OutErrorMessage);
		return false;
	}

	bool bSuccess = MCPBridgeHelpers::SetActorPropertyFromString(Actor, PropertyName, Value);
	if (!bSuccess)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_SET_FAILED"), FString::Printf(TEXT("Failed to set property '%s' on actor '%s'"), *PropertyName, *ActorName), OutErrorCode, OutErrorMessage);
		return false;
	}

	// Read-back 验证：读取新值确认写入成功
	bool bFound = false;
	FString ReadBackValue = MCPBridgeHelpers::GetActorPropertyAsString(Actor, PropertyName, bFound);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("actor_name"), Actor->GetName());
	OutResult->SetStringField(TEXT("property_name"), PropertyName);
	OutResult->SetStringField(TEXT("value"), ReadBackValue);
	OutResult->SetBoolField(TEXT("found"), bFound);
	OutResult->SetBoolField(TEXT("set"), true);
	return true;
}

// ====== SaveCurrentLevel ======
bool FMCPSaveCurrentLevelHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (!World)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_WORLD"), TEXT("No editor world available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	bool bSaved = FEditorFileUtils::SaveCurrentLevel();
	if (!bSaved)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("SAVE_FAILED"), TEXT("Failed to save current level. Check asset path and permissions."), OutErrorCode, OutErrorMessage);
		return false;
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("world_name"), World->GetName());
	OutResult->SetStringField(TEXT("path"), World->GetPathName());
	OutResult->SetBoolField(TEXT("saved"), true);
	return true;
}

// ====== DeleteActor（新增）======
bool FMCPDeleteActorHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ActorName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.actor_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (!World)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_WORLD"), TEXT("No editor world available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	AActor* Actor = MCPBridgeHelpers::FindActorByNameOrLabel(World, ActorName);
	if (!Actor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("ACTOR_NOT_FOUND"), FString::Printf(TEXT("Actor '%s' not found"), *ActorName), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString DeletedName = Actor->GetName();
	bool bDestroyed = MCPBridgeHelpers::DestroyActorInEditor(Actor);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("actor_name"), DeletedName);
	OutResult->SetBoolField(TEXT("deleted"), bDestroyed);

	if (!bDestroyed)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("DELETE_FAILED"), FString::Printf(TEXT("Failed to delete actor '%s'"), *DeletedName), OutErrorCode, OutErrorMessage);
		return false;
	}

	return true;
}

// ====== SetComponentProperty（新增）======
bool FMCPSetComponentPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ActorName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.actor_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString ComponentName;
	if (!Payload->TryGetStringField(TEXT("component_name"), ComponentName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.component_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString PropertyName;
	if (!Payload->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.property_name is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	FString Value;
	if (!Payload->TryGetStringField(TEXT("value"), Value))
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("payload.value is required"), OutErrorCode, OutErrorMessage);
		return false;
	}

	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (!World)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("NO_WORLD"), TEXT("No editor world available"), OutErrorCode, OutErrorMessage);
		return false;
	}

	AActor* Actor = MCPBridgeHelpers::FindActorByNameOrLabel(World, ActorName);
	if (!Actor)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("ACTOR_NOT_FOUND"), FString::Printf(TEXT("Actor '%s' not found"), *ActorName), OutErrorCode, OutErrorMessage);
		return false;
	}

	UActorComponent* Component = MCPBridgeHelpers::FindComponentByNameOrClass(Actor, ComponentName);
	if (!Component)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("COMPONENT_NOT_FOUND"), FString::Printf(TEXT("Component '%s' not found on actor '%s'"), *ComponentName, *ActorName), OutErrorCode, OutErrorMessage);
		return false;
	}

	bool bSuccess = MCPBridgeHelpers::SetComponentPropertyFromString(Component, PropertyName, Value);
	if (!bSuccess)
	{
		MCPBridgeHelpers::BuildErrorResponse(TEXT("PROPERTY_SET_FAILED"), FString::Printf(TEXT("Failed to set property '%s'"), *PropertyName), OutErrorCode, OutErrorMessage);
		return false;
	}

	// Read-back 验证
	bool bFound = false;
	FString ReadBackValue = MCPBridgeHelpers::GetComponentPropertyAsString(Component, PropertyName, bFound);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("actor_name"), Actor->GetName());
	OutResult->SetStringField(TEXT("component_name"), Component->GetName());
	OutResult->SetStringField(TEXT("component_class"), Component->GetClass()->GetName());
	OutResult->SetStringField(TEXT("property_name"), PropertyName);
	OutResult->SetStringField(TEXT("value"), ReadBackValue);
	OutResult->SetBoolField(TEXT("found"), bFound);
	OutResult->SetBoolField(TEXT("set"), true);
	return true;
}
