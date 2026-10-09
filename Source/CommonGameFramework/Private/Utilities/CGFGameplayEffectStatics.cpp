#include "Utilities/CGFGameplayEffectStatics.h"
#include "Tags/CGFGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "CommonGameFramework.h"

DEFINE_LOG_CATEGORY_STATIC(LogCGFEffects, Log, All);

FCGFAttributeModifier UCGFGameplayEffectStatics::MakeAttributeModifier(TSubclassOf<UAttributeSet> AttributeSet, FName AttributeName, float Magnitude, bool& bOutValid)
{
	FCGFAttributeModifier Modifier;
	Modifier.Magnitude = Magnitude;
	bOutValid = false;
	if (AttributeSet)
	{
		if (FProperty* Property = FindFProperty<FProperty>(AttributeSet, AttributeName))
		{
			Modifier.Attribute = FGameplayAttribute(Property);
			bOutValid = Modifier.Attribute.IsValid();
		}
	}
	if (!bOutValid)
	{
		UE_LOG(LogCGFEffects, Warning, TEXT("MakeAttributeModifier: %s has no attribute '%s'."), *GetNameSafe(AttributeSet), *AttributeName.ToString());
	}
	return Modifier;
}

FGameplayTag UCGFGameplayEffectStatics::MakeStatSetByCallerTag(const FGameplayAttribute& Attribute)
{
	if (!Attribute.IsValid())
	{
		return FGameplayTag();
	}
	const FString TagName = FString::Printf(TEXT("%s.%s"), *CGFGameplayTags::SetByCaller_Stat.GetTag().ToString(), *Attribute.GetName());
	return FGameplayTag::RequestGameplayTag(FName(*TagName), /*ErrorIfNotFound*/ false);
}

bool UCGFGameplayEffectStatics::ApplyInstantAttributeModifiers(UAbilitySystemComponent* ASC,
	const TArray<FCGFAttributeModifier>& Modifiers, UObject* SourceObject)
{
	if (!ASC)
	{
		return false;
	}

	// Transient definition, outered to the ASC so it lives exactly as long as the component.
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(ASC, NAME_None, RF_Transient);
	Effect->DurationPolicy = EGameplayEffectDurationType::Instant;

	for (const FCGFAttributeModifier& Modifier : Modifiers)
	{
		if (!Modifier.IsValid())
		{
			continue;
		}
		FGameplayModifierInfo Info;
		Info.Attribute = Modifier.Attribute;
		Info.ModifierOp = EGameplayModOp::Additive;
		Info.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Modifier.Magnitude));
		Effect->Modifiers.Add(Info);
	}

	if (Effect->Modifiers.Num() == 0)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (SourceObject)
	{
		Context.AddSourceObject(SourceObject);
	}
	ASC->ApplyGameplayEffectToSelf(Effect, 1.f, Context);
	return true;
}

TArray<FGameplayAttribute> UCGFGameplayEffectStatics::GetStatEffectAttributes(TSubclassOf<UGameplayEffect> EffectClass)
{
	TArray<FGameplayAttribute> Result;
	const UGameplayEffect* Effect = EffectClass ? EffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	if (!Effect)
	{
		return Result;
	}
	for (const FGameplayModifierInfo& Info : Effect->Modifiers)
	{
		if (Info.ModifierMagnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::SetByCaller
			&& Info.ModifierMagnitude.GetSetByCallerFloat().DataTag.MatchesTag(CGFGameplayTags::SetByCaller_Stat))
		{
			Result.Add(Info.Attribute);
		}
	}
	return Result;
}

FActiveGameplayEffectHandle UCGFGameplayEffectStatics::ApplyStatModifierEffect(UAbilitySystemComponent* ASC,
	TSubclassOf<UGameplayEffect> EffectClass, const TArray<FCGFAttributeModifier>& Modifiers, UObject* SourceObject)
{
	if (!ASC || !EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	const UGameplayEffect* Effect = EffectClass->GetDefaultObject<UGameplayEffect>();
	if (!Effect || Effect->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		UE_LOG(LogCGFEffects, Warning, TEXT("ApplyStatModifierEffect: %s must be a duration/infinite effect."), *GetNameSafe(EffectClass));
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	if (SourceObject)
	{
		Context.AddSourceObject(SourceObject);
	}
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}
	FGameplayEffectSpec& Spec = *SpecHandle.Data;

	// Every SetByCaller modifier on the class starts at zero so unrequested stats are no-ops
	// instead of "magnitude not found" warnings.
	TMap<FGameplayTag, FGameplayAttribute> CoveredByTag;
	for (const FGameplayModifierInfo& Info : Effect->Modifiers)
	{
		if (Info.ModifierMagnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::SetByCaller)
		{
			const FGameplayTag Tag = Info.ModifierMagnitude.GetSetByCallerFloat().DataTag;
			if (Tag.IsValid())
			{
				Spec.SetSetByCallerMagnitude(Tag, 0.f);
				CoveredByTag.Add(Tag, Info.Attribute);
			}
		}
	}

	int32 Applied = 0;
	for (const FCGFAttributeModifier& Modifier : Modifiers)
	{
		if (!Modifier.IsValid())
		{
			continue;
		}
		const FGameplayTag Tag = MakeStatSetByCallerTag(Modifier.Attribute);
		if (!Tag.IsValid() || !CoveredByTag.Contains(Tag))
		{
			UE_LOG(LogCGFEffects, Warning, TEXT("ApplyStatModifierEffect: %s has no SetByCaller modifier for attribute %s (expected tag %s.%s); skipped."),
				*GetNameSafe(EffectClass), *Modifier.Attribute.GetName(), *CGFGameplayTags::SetByCaller_Stat.GetTag().ToString(), *Modifier.Attribute.GetName());
			continue;
		}
		// Accumulate so two entries for the same attribute add up.
		const float Current = Spec.GetSetByCallerMagnitude(Tag, false, 0.f);
		Spec.SetSetByCallerMagnitude(Tag, Current + Modifier.Magnitude);
		++Applied;
	}

	if (Applied == 0)
	{
		return FActiveGameplayEffectHandle();
	}
	return ASC->ApplyGameplayEffectSpecToSelf(Spec);
}
