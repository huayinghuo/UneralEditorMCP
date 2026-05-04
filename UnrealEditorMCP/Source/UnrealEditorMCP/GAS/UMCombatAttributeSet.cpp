// Fill out your copyright notice in the Description page of Project Settings.


#include "UMCombatAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"


UUMCombatAttributeSet::UUMCombatAttributeSet()
{
}

void UUMCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		ClampAttribute(Attribute, NewValue, GetMaxHealthAttribute());
	}
	else if (Attribute == GetManaAttribute())
	{
		ClampAttribute(Attribute, NewValue, GetMaxManaAttribute());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		ClampAttribute(Attribute, NewValue, GetMaxStaminaAttribute());
	}
	// AttackPower、AbilityPower、Armor、MoveSpeed 没有上界约束
	// Damage 作为瞬态属性，不在此 Clamp
}

void UUMCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 仅服务器端处理 Damage → Health 的关系
	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		float DownDamage = GetDamage();
		if (DownDamage > 0.f)
		{
			// 扣减 Health
			float NewHealth = GetHealth() - DownDamage;
			SetHealth(NewHealth);
		}
			// 清零 Damage（GE 执行完成后重置，准备下一次伤害计算）
			SetDamage(0.f);
	}
}

void UUMCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 所有持久属性都需要复制
	// Damage 不需要复制，不在此注册
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, AbilityPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UUMCombatAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UUMCombatAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue,
                                           const FGameplayAttribute& MaxAttribute) const
{
	// 从当前 ASC 的 AttributeSet 中读取 MaxXxx 的 CurrentValue
	float MaxValue = MaxAttribute.GetNumericValue(this);
	if(MaxValue > 0.f)
	{
		NewValue = FMath::Clamp(NewValue, 0.f, MaxValue);
	}
	else
	{
		// 如果 MaxValue 无效（<= 0），至少保证 NewValue 不为负数
		NewValue = FMath::Max(NewValue, 0.f);
	}
}


// ========== OnRep 实现 ==========
void UUMCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	// GAMEPLAYATTRIBUTE_REPNOTIFY 宏自动处理：
	// 1. 比较新旧值
	// 2. 通知 ASC 属性已变化
	// 3. 触发 UI 绑定的回调
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, Health, OldHealth);
}

void UUMCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, MaxHealth, OldMaxHealth);
}

void UUMCombatAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, Mana, OldMana);
}

void UUMCombatAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, MaxMana, OldMaxMana);
}

void UUMCombatAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, Stamina, OldStamina);
}

void UUMCombatAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, MaxStamina, OldMaxStamina);
}

void UUMCombatAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, AttackPower, OldAttackPower);
}

void UUMCombatAttributeSet::OnRep_AbilityPower(const FGameplayAttributeData& OldAbilityPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, AbilityPower, OldAbilityPower);
}

void UUMCombatAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, Armor, OldArmor);
}

void UUMCombatAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UUMCombatAttributeSet, MoveSpeed, OldMoveSpeed);
}
