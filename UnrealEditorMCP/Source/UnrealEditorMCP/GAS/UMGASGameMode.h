// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "UMGASGameMode.generated.h"

/** GAS 游戏模式：构造函数设置默认 Pawn、PlayerState 和 PlayerController 类型 */
UCLASS()
class UNREALEDITORMCP_API AUMGASGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AUMGASGameMode();
};
