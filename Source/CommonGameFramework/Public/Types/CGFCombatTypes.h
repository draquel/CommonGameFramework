#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CGFCombatTypes.generated.h"

// ---------------------------------------------------------------------------
// ECGFDamageResult — outcome of a damage application request
// ---------------------------------------------------------------------------

/**
 * Result of asking a damageable target to take damage. Every rejection has its
 * own value so callers (abilities, traps, debug commands) can log or react
 * without re-deriving the reason.
 */
UENUM(BlueprintType)
enum class ECGFDamageResult : uint8
{
	/** Damage was applied to the target's attribute pipeline. */
	Applied,
	/** No damageable object was found on the target actor. */
	Rejected_NoTarget,
	/** Called without network authority — damage is server-authoritative. */
	Rejected_NotAuthority,
	/** Target is already dead. */
	Rejected_Dead,
	/** Target currently carries State.Invulnerable. */
	Rejected_Invulnerable,
	/** Instigator and target are not hostile and the context did not ignore factions. */
	Rejected_Friendly,
	/** Target has no ability system component to apply the effect to. */
	Rejected_NoAbilitySystem,
	/** The target's ICGFDamageableInterface::IsImmuneToDamage vetoed this context. */
	Rejected_ByTarget,
};

// ---------------------------------------------------------------------------
// FCGFDamageContext — everything a damage source knows about one hit
// ---------------------------------------------------------------------------

/**
 * Describes a single damage event from the source's point of view. Built by
 * the attacker (ability, trap, projectile, debug command) and handed to the
 * target's damageable implementation, which turns it into attribute changes.
 *
 * Carries no logic: the receiving plugin decides how BaseDamage, DamageType
 * and the actors involved map onto its attribute/mitigation model.
 */
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FCGFDamageContext
{
	GENERATED_BODY()

	/** Actor responsible for the hit (usually a pawn). Null for environmental hazards. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TWeakObjectPtr<AActor> InstigatorActor;

	/** Actor that physically caused the hit (weapon, trap, projectile). May equal InstigatorActor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TWeakObjectPtr<AActor> CauserActor;

	/** Damage.Type.* tag. Receivers treat an empty tag as Damage.Type.Physical. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (Categories = "Damage.Type"))
	FGameplayTag DamageType;

	/** Damage before the receiver's own modifiers (attack power, defense, resistances). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseDamage = 0.f;

	/** World-space impact point, if known. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FVector HitLocation = FVector::ZeroVector;

	/** World-space impact normal, if known. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FVector HitNormal = FVector::ZeroVector;

	/** InstanceId of the FItemInstance that produced this hit (equipped weapon, thrown item). Invalid if none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FGuid SourceItemInstanceId;

	/** When true the receiver skips the faction/hostility check (hazards hurt everyone). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	bool bIgnoreFaction = false;

	/** Free-form tags for later systems (e.g. Damage.Source.Trap, Damage.Critical). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	FGameplayTagContainer ContextTags;

	/** True when BaseDamage is positive and finite. */
	bool HasDamage() const { return FMath::IsFinite(BaseDamage) && BaseDamage > 0.f; }
};
