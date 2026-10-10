#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Interfaces/CGFDamageableInterface.h"
#include "CGFCombatStatics.generated.h"

/**
 * Pure helpers over the combat contracts: locate a damageable on an actor and
 * answer the faction/hostility rule. No gameplay state, no damage application —
 * that belongs to the plugin that owns the attribute model.
 *
 * The hostility rule is deliberately one function (AreHostileFactions) so a
 * faction-relationship table can replace it later without touching callers.
 */
UCLASS()
class COMMONGAMEFRAMEWORK_API UCGFCombatStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Find the object implementing ICGFDamageableInterface for an actor: the actor
	 * itself first, then its components (C++ or Blueprint implementers).
	 * @param Actor Actor to search. Null-safe.
	 * @return Interface handle; its object is null when nothing on the actor is damageable.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Combat")
	static TScriptInterface<ICGFDamageableInterface> FindDamageable(AActor* Actor);

	/** Faction.* tag of the actor's damageable, or an empty tag if it has none. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Combat")
	static FGameplayTag GetFactionTag(AActor* Actor);

	/** True if the actor has a damageable that reports itself dead. Non-combatants return false. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Combat")
	static bool IsActorDead(AActor* Actor);

	/**
	 * Hostility rule between two faction tags (v1):
	 *  - either tag empty  → not hostile (non-combatants are never valid targets)
	 *  - either is Faction.Neutral → not hostile
	 *  - same tag → not hostile
	 *  - otherwise → hostile
	 * Exact-match only; nested faction tags are treated as distinct factions. Faction.Object
	 * (breakable props) is hostile to every other faction, Neutral included; Object vs Object is not.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Combat")
	static bool AreHostileFactions(FGameplayTag FactionA, FGameplayTag FactionB);

	/** AreHostileFactions over the two actors' damageables. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Combat")
	static bool AreHostile(AActor* ActorA, AActor* ActorB);
};
