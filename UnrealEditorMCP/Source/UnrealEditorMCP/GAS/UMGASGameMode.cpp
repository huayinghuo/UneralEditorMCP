// Copyright Epic Games, Inc. All Rights Reserved.

#include "UMGASGameMode.h"
#include "UMGASPlayerCharacter.h"
#include "UMGASPlayerState.h"
#include "UMGASPlayerController.h"

AUMGASGameMode::AUMGASGameMode()
{
	// 在构造函数中绑定默认类型，无需修改 DefaultEngine.ini
	// 也可在项目设置 → Maps & Modes 中覆盖
	DefaultPawnClass = AUMGASPlayerCharacter::StaticClass();
	PlayerStateClass = AUMGASPlayerState::StaticClass();
	PlayerControllerClass = AUMGASPlayerController::StaticClass();
}
