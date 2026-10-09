#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "Types/CGFCombatTypes.h"
#include "CGFDamageableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by anything that can be damaged and belongs to a faction.
 *
 * The usual implementer is a combat component on a pawn (same pattern as
 * IInteractable on UInteractableComponent); use UCGFCombatStatics::FindDamageable
 * to locate it from an actor. Damage application itself is not part of the
 * contract — it lives with the implementer's attribute model. This interface
 * answers the questions every attacker needs before and after a hit.
 *
 * All three functions have a generated default (empty faction, not dead, not
 * immune); implementers are expected to override all of them.
 */
class COMMONGAMEFRAMEWORK_API ICGFDamageableInterface
{
	GENERATED_BODY()

public:
	/** Faction.* tag this damageable belongs to. Empty = not a combatant (never hostile). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	FGameplayTag GetFactionTag() const;

	/** True once the damageable has died. Dead targets reject further damage. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool IsDead() const;

	/**
	 * Lets the target veto a specific hit (e.g. immune to Damage.Type.Fire).
	 * Default false = damage allowed. Checked after the dead/invulnerable/faction checks.
	 * @param Context The hit being evaluated.
	 * @return True to reject this hit with ECGFDamageResult::Rejected_ByTarget.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool IsImmuneToDamage(const FCGFDamageContext& Context) const;
};
