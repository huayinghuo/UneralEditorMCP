#include "Handlers/MCPPIERuntimeHandlers.h"
#include "MCPBridgeHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Editor.h"
#include "UnrealEdGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogMCPPIE, Log, All);

// ====== 1. pie_start ======
bool FMCPPIEStartHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	if (!GEditor) { MCPBridgeHelpers::BuildErrorResponse(TEXT("EDITOR_NOT_FOUND"), TEXT("GEditor is null"), OutErrorCode, OutErrorMessage); return false; }

	bool bSimulate = false;
	if (Payload.IsValid()) Payload->TryGetBoolField(TEXT("simulate"), bSimulate);

	FRequestPlaySessionParams Params;
	GEditor->RequestPlaySession(Params);

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("started"), true);
	OutResult->SetBoolField(TEXT("simulate"), bSimulate);
	return true;
}

// ====== 2. pie_stop ======
bool FMCPPIEStopHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	if (!GEditor) { MCPBridgeHelpers::BuildErrorResponse(TEXT("EDITOR_NOT_FOUND"), TEXT("GEditor is null"), OutErrorCode, OutErrorMessage); return false; }
	if (!GEditor->IsPlaySessionInProgress()) { MCPBridgeHelpers::BuildErrorResponse(TEXT("PIE_NOT_RUNNING"), TEXT("No PIE session"), OutErrorCode, OutErrorMessage); return false; }
	GEditor->RequestEndPlayMap();

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("stopped"), true);
	return true;
}

// ====== 3. pie_is_running ======
bool FMCPPIEIsRunningHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetBoolField(TEXT("running"), GEditor && GEditor->IsPlaySessionInProgress());
	return true;
}

// ====== 4. get_actor_state ======
bool FMCPGetActorStateHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString ActorName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("actor_name"), ActorName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("actor_name is required"), OutErrorCode, OutErrorMessage); return false; }
	if (!GEditor || !GEditor->IsPlaySessionInProgress())
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("PIE_NOT_RUNNING"), TEXT("PIE must be running"), OutErrorCode, OutErrorMessage); return false; }

	UWorld* World = nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{ if (Context.WorldType == EWorldType::PIE && Context.World()) { World = Context.World(); break; } }
	if (!World) { MCPBridgeHelpers::BuildErrorResponse(TEXT("WORLD_NOT_FOUND"), TEXT("Play world not found"), OutErrorCode, OutErrorMessage); return false; }

	AActor* FoundActor = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{ if (It->GetName().Contains(ActorName) || It->GetActorNameOrLabel().Contains(ActorName)) { FoundActor = *It; break; } }
	if (!FoundActor) { MCPBridgeHelpers::BuildErrorResponse(TEXT("ACTOR_NOT_FOUND"), FString::Printf(TEXT("Actor '%s' not found"), *ActorName), OutErrorCode, OutErrorMessage); return false; }

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("actor_name"), FoundActor->GetName());
	OutResult->SetStringField(TEXT("actor_label"), FoundActor->GetActorNameOrLabel());

	FVector Loc = FoundActor->GetActorLocation();
	TSharedPtr<FJsonObject> Pos = MakeShareable(new FJsonObject()); Pos->SetNumberField(TEXT("x"), Loc.X); Pos->SetNumberField(TEXT("y"), Loc.Y); Pos->SetNumberField(TEXT("z"), Loc.Z);
	OutResult->SetObjectField(TEXT("position"), Pos);

	FRotator Rot = FoundActor->GetActorRotation();
	TSharedPtr<FJsonObject> Ro = MakeShareable(new FJsonObject()); Ro->SetNumberField(TEXT("pitch"), Rot.Pitch); Ro->SetNumberField(TEXT("yaw"), Rot.Yaw); Ro->SetNumberField(TEXT("roll"), Rot.Roll);
	OutResult->SetObjectField(TEXT("rotation"), Ro);

	FVector Vel = FoundActor->GetVelocity();
	TSharedPtr<FJsonObject> Ve = MakeShareable(new FJsonObject()); Ve->SetNumberField(TEXT("x"), Vel.X); Ve->SetNumberField(TEXT("y"), Vel.Y); Ve->SetNumberField(TEXT("z"), Vel.Z);
	OutResult->SetObjectField(TEXT("velocity"), Ve);
	return true;
}

// ====== 5. set_level_default_pawn ======
bool FMCPSetLevelDefaultPawnHandler::Execute(TSharedPtr<FJsonObject> Payload, TSharedPtr<FJsonObject>& OutResult, FString& OutErrorCode, FString& OutErrorMessage)
{
	FString PawnClassName;
	if (!Payload.IsValid() || !Payload->TryGetStringField(TEXT("pawn_class"), PawnClassName))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("MISSING_PARAM"), TEXT("pawn_class is required"), OutErrorCode, OutErrorMessage); return false; }

	UClass* PawnClass = FindFirstObject<UClass>(*PawnClassName, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging);
	if (!PawnClass) { FString P = TEXT("A") + PawnClassName; PawnClass = FindFirstObject<UClass>(*P, EFindFirstObjectOptions::None, ELogVerbosity::NoLogging); }
	if (!PawnClass || !PawnClass->IsChildOf(APawn::StaticClass()))
	{ MCPBridgeHelpers::BuildErrorResponse(TEXT("CLASS_NOT_FOUND"), FString::Printf(TEXT("Pawn class '%s' not found"), *PawnClassName), OutErrorCode, OutErrorMessage); return false; }

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) { MCPBridgeHelpers::BuildErrorResponse(TEXT("WORLD_NOT_FOUND"), TEXT("No editor world"), OutErrorCode, OutErrorMessage); return false; }

	AGameModeBase* GameMode = World->GetAuthGameMode();
	AGameModeBase* DefaultGM = GameMode ? GameMode : Cast<AGameModeBase>(World->GetWorldSettings()->DefaultGameMode.GetDefaultObject());
	if (DefaultGM)
	{
		DefaultGM->Modify();
		DefaultGM->DefaultPawnClass = PawnClass;
		DefaultGM->MarkPackageDirty();
	}

	OutResult = MakeShareable(new FJsonObject());
	OutResult->SetStringField(TEXT("pawn_class"), PawnClass->GetName());
	OutResult->SetBoolField(TEXT("set"), true);
	return true;
}
