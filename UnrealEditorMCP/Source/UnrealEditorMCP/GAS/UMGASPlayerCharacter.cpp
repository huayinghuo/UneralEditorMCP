// Copyright Epic Games, Inc. All Rights Reserved.

#include "UMGASPlayerCharacter.h"
#include "UMGASPlayerState.h"
#include "AbilitySystemComponent.h"
#include "UMCombatAttributeSet.h"

AUMGASPlayerCharacter::AUMGASPlayerCharacter()
{
}

void AUMGASPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 服务器侧：控制器取得此 Pawn 时立即初始化 ASC ActorInfo
	// ASC 在 PlayerState 上，this（Pawn）作为 AvatarActor
	InitAbilityActorInfo();
}

void AUMGASPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 客户端侧：PlayerState 复制到达后初始化 ASC ActorInfo
	// 客户端此时才拿到复制的 ASC 引用
	InitAbilityActorInfo();
}

void AUMGASPlayerCharacter::InitAbilityActorInfo()
{
	// 从 PlayerState 获取 ASC
	AUMGASPlayerState* UMPlayerState = GetPlayerState<AUMGASPlayerState>();
	if (!UMPlayerState)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UMPlayerState->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// 核心：将 ASC 的 OwnerActor 设为 PlayerState，AvatarActor 设为此 Pawn
	// Server 端调用时为 ASC 绑定复制关系；Client 端调用时建立本地代理结构
	ASC->InitAbilityActorInfo(UMPlayerState, this);

	if(!ASC->GetAttributeSet(UUMCombatAttributeSet::StaticClass()))
	{
		// 确保 ASC 拥有 CombatAttributeSet：如果没有，创建一个新的实例并添加到 ASC 中
		UUMCombatAttributeSet* CombatAttributeSet = NewObject<UUMCombatAttributeSet>(this);
		if (CombatAttributeSet)
		{
			ASC->AddAttributeSetSubobject(CombatAttributeSet);
		}
	}

	// 应用初始属性 GE
	if (HasAuthority())
	{
		// 加载 GE 蓝图类
		UClass* InitGEClass = LoadClass<UGameplayEffect>(nullptr,
			TEXT("/Game/GAS/GameplayEffects/GE_Hero_AttributeInit.GE_Hero_AttributeInit_C"));

		if (InitGEClass && ASC)
		{
			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			// Level = 1 表示 1 级效果（后续技能升级才会用到不同 Level）
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(InitGEClass, 1.0f, ContextHandle);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}
