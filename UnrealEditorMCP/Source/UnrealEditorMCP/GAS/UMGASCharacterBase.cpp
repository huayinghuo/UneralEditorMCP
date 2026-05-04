// Copyright Epic Games, Inc. All Rights Reserved.

#include "UMGASCharacterBase.h"

AUMGASCharacterBase::AUMGASCharacterBase()
{
}

UAbilitySystemComponent* AUMGASCharacterBase::GetAbilitySystemComponent() const
{
	// 基类默认返回 nullptr：子类需要根据自身 ASC 来源重写
	return nullptr;
}

void AUMGASCharacterBase::InitAbilityActorInfo()
{
	// 子类重写以绑定 OwnerActor 和 AvatarActor
}

void AUMGASCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}
