// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UMGASCharacterBase.h"
#include "UMGASPlayerCharacter.generated.h"

/** 玩家操控的角色：ASC 来自 PlayerState，在 PossessedBy / OnRep_PlayerState 中绑定 ActorInfo */
UCLASS()
class UNREALEDITORMCP_API AUMGASPlayerCharacter : public AUMGASCharacterBase
{
	GENERATED_BODY()

public:
	AUMGASPlayerCharacter();

	/** 服务器端：控制器获取到此 Pawn 时初始化 ASC ActorInfo */
	virtual void PossessedBy(AController* NewController) override;

	/** 客户端侧：PlayerState 复制到达时初始化 ASC ActorInfo */
	virtual void OnRep_PlayerState() override;

	virtual void InitAbilityActorInfo() override;
};
