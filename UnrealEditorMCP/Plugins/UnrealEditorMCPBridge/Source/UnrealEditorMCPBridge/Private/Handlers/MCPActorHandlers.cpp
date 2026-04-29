#include "Handlers/MCPActorHandlers.h"
#include "MCPBridgeHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Selection.h"

// ====== GetSelectedActors（增强：含 Transform）======
bool FMCPGetSelectedActorsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> ActorsArray;

	if (GEditor)
	{
		USelection* SelectedActors = GEditor->GetSelectedActors();
		if (SelectedActors)
		{
			for (int32 i = 0; i < SelectedActors->Num(); ++i)
			{
				AActor* Actor = Cast<AActor>(SelectedActors->GetSelectedObject(i));
				if (Actor)
				{
					ActorsArray.Add(MakeShareable(new FJsonValueObject(
						MCPBridgeHelpers::BuildActorJson(Actor, true))));
				}
			}
		}
	}

	OutResult->SetArrayField(TEXT("actors"), ActorsArray);
	OutResult->SetNumberField(TEXT("count"), ActorsArray.Num());
	return true;
}

// ====== ListLevelActors（增强：含 Transform）======
bool FMCPListLevelActorsHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());
	TArray<TSharedPtr<FJsonValue>> ActorsArray;

	UWorld* World = MCPBridgeHelpers::GetEditorWorld();
	if (World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			ActorsArray.Add(MakeShareable(new FJsonValueObject(
				MCPBridgeHelpers::BuildActorJson(*It, true))));
		}
	}

	OutResult->SetArrayField(TEXT("actors"), ActorsArray);
	OutResult->SetNumberField(TEXT("count"), ActorsArray.Num());
	return true;
}

// ====== GetActorProperty（新增：通过 UE 反射读属性）======
bool FMCPGetActorPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
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

	OutResult = MakeShareable(new FJsonObject());

	bool bFound = false;
	FString PropertyValue = MCPBridgeHelpers::GetActorPropertyAsString(Actor, PropertyName, bFound);
	OutResult->SetStringField(TEXT("actor_name"), Actor->GetName());
	OutResult->SetStringField(TEXT("property_name"), PropertyName);
	OutResult->SetStringField(TEXT("value"), PropertyValue);
	OutResult->SetBoolField(TEXT("found"), bFound);

	return true;
}

// ====== GetComponentProperty（新增：通过反射读 Component 属性）======
bool FMCPGetComponentPropertyHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
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

	OutResult = MakeShareable(new FJsonObject());

	bool bFound = false;
	FString PropertyValue = MCPBridgeHelpers::GetComponentPropertyAsString(Component, PropertyName, bFound);
	OutResult->SetStringField(TEXT("actor_name"), Actor->GetName());
	OutResult->SetStringField(TEXT("component_name"), Component->GetName());
	OutResult->SetStringField(TEXT("component_class"), Component->GetClass()->GetName());
	OutResult->SetStringField(TEXT("property_name"), PropertyName);
	OutResult->SetStringField(TEXT("value"), PropertyValue);
	OutResult->SetBoolField(TEXT("found"), bFound);

	return true;
}
