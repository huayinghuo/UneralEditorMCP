// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "UMGASCharacterBase.generated.h"

class UAbilitySystemComponent;

/** GAS 角色基类：玩家和怪物共用，提供 ASC 访问接口的统一入口 */
UCLASS(Abstract)
class UNREALEDITORMCP_API AUMGASCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AUMGASCharacterBase();

	/** 获取此角色的 ASC：玩家委托到 PlayerState，怪物返回自身持有的 ASC */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	/** 子类需重写此函数以初始化 ASC ActorInfo（SetOwnerActor + SetAvatarActor） */
	virtual void InitAbilityActorInfo();

protected:
	virtual void BeginPlay() override;
};
