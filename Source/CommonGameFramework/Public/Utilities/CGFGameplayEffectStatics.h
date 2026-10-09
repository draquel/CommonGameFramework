#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "Types/CGFCombatTypes.h"
#include "CGFGameplayEffectStatics.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UAttributeSet;

/**
 * Applies data-driven FCGFAttributeModifier lists without gameplay-effect assets.
 *
 * Two paths, chosen by duration:
 *  - Instant changes (consumables) build a transient instant UGameplayEffect on the fly.
 *    Instant effects are executed on the server and never replicated as active effects,
 *    so a transient definition is safe.
 *  - Lasting changes (equipment stats) must NOT use a transient definition: active effects
 *    replicate to the owning client by definition reference, and a runtime object cannot be
 *    resolved there. Instead the caller supplies a source-defined effect class whose modifiers
 *    are SetByCaller keyed by the convention tag SetByCaller.Stat.<AttributeName>; this helper
 *    fills those magnitudes (zero for anything not requested) and applies the spec.
 *
 * Both are server-only operations; callers check authority.
 */
UCLASS()
class COMMONGAMEFRAMEWORK_API UCGFGameplayEffectStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Build a modifier from an attribute set class and a property name (for scripts and data tools
	 * that cannot author FGameplayAttribute directly).
	 * @param AttributeSet  Attribute set class that declares the property.
	 * @param AttributeName Property name, e.g. "Defense".
	 * @param Magnitude     Additive amount.
	 * @param bOutValid     False when the property does not exist on the class.
	 */
	UFUNCTION(BlueprintCallable, Category = "CGF|GameplayEffects")
	static FCGFAttributeModifier MakeAttributeModifier(TSubclassOf<UAttributeSet> AttributeSet, FName AttributeName, float Magnitude, bool& bOutValid);

	/** SetByCaller.Stat.<AttributeName> for an attribute, or an empty tag if that tag is not registered. */
	UFUNCTION(BlueprintPure, Category = "CGF|GameplayEffects")
	static FGameplayTag MakeStatSetByCallerTag(const FGameplayAttribute& Attribute);

	/**
	 * Apply additive changes once, through a transient instant effect.
	 * @param ASC          Target ability system (server).
	 * @param Modifiers    Attribute / magnitude pairs; invalid entries are skipped.
	 * @param SourceObject Recorded in the effect context (the item definition, the actor...). May be null.
	 * @return True if an effect with at least one valid modifier was applied.
	 */
	UFUNCTION(BlueprintCallable, Category = "CGF|GameplayEffects")
	static bool ApplyInstantAttributeModifiers(UAbilitySystemComponent* ASC, const TArray<FCGFAttributeModifier>& Modifiers,
		UObject* SourceObject);

	/**
	 * Apply lasting additive changes through a source-defined effect class with SetByCaller.Stat.* modifiers.
	 * Every SetByCaller modifier on the class defaults to 0; the requested ones get their magnitude.
	 * Modifiers whose attribute the class does not cover are logged and skipped.
	 * @param ASC          Target ability system (server).
	 * @param EffectClass  Infinite/duration effect whose modifiers are SetByCaller keyed by SetByCaller.Stat.<AttributeName>.
	 * @param Modifiers    Attribute / magnitude pairs.
	 * @param SourceObject Recorded in the effect context. May be null.
	 * @return Handle of the applied effect (invalid if nothing could be applied). Remove it with RemoveActiveGameplayEffect.
	 */
	UFUNCTION(BlueprintCallable, Category = "CGF|GameplayEffects")
	static FActiveGameplayEffectHandle ApplyStatModifierEffect(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayEffect> EffectClass,
		const TArray<FCGFAttributeModifier>& Modifiers, UObject* SourceObject);

	/** Attributes a stat-modifier effect class covers (its SetByCaller.Stat.* modifiers), for validation and tooling. */
	UFUNCTION(BlueprintPure, Category = "CGF|GameplayEffects")
	static TArray<FGameplayAttribute> GetStatEffectAttributes(TSubclassOf<UGameplayEffect> EffectClass);
};
