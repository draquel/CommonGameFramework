#include "Utilities/CGFCombatStatics.h"
#include "Tags/CGFGameplayTags.h"
#include "GameFramework/Actor.h"

TScriptInterface<ICGFDamageableInterface> UCGFCombatStatics::FindDamageable(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	// Implements<> works for both native and Blueprint implementers; the native
	// interface pointer inside TScriptInterface stays null for Blueprint ones,
	// which is why callers must go through ICGFDamageableInterface::Execute_*.
	if (Actor->Implements<UCGFDamageableInterface>())
	{
		return TScriptInterface<ICGFDamageableInterface>(Actor);
	}

	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (IsValid(Component) && Component->Implements<UCGFDamageableInterface>())
		{
			return TScriptInterface<ICGFDamageableInterface>(Component);
		}
	}

	return nullptr;
}

FGameplayTag UCGFCombatStatics::GetFactionTag(AActor* Actor)
{
	const TScriptInterface<ICGFDamageableInterface> Damageable = FindDamageable(Actor);
	if (UObject* Object = Damageable.GetObject())
	{
		return ICGFDamageableInterface::Execute_GetFactionTag(Object);
	}
	return FGameplayTag();
}

bool UCGFCombatStatics::IsActorDead(AActor* Actor)
{
	const TScriptInterface<ICGFDamageableInterface> Damageable = FindDamageable(Actor);
	if (UObject* Object = Damageable.GetObject())
	{
		return ICGFDamageableInterface::Execute_IsDead(Object);
	}
	return false;
}

bool UCGFCombatStatics::AreHostileFactions(FGameplayTag FactionA, FGameplayTag FactionB)
{
	if (!FactionA.IsValid() || !FactionB.IsValid())
	{
		return false;
	}

	// Breakable props (Faction.Object) are fair game for everyone, including neutrals; two objects
	// are never hostile to each other.
	const bool bAObject = FactionA == CGFGameplayTags::Faction_Object;
	const bool bBObject = FactionB == CGFGameplayTags::Faction_Object;
	if (bAObject || bBObject)
	{
		return !(bAObject && bBObject);
	}

	if (FactionA == CGFGameplayTags::Faction_Neutral || FactionB == CGFGameplayTags::Faction_Neutral)
	{
		return false;
	}

	return FactionA != FactionB;
}

bool UCGFCombatStatics::AreHostile(AActor* ActorA, AActor* ActorB)
{
	return AreHostileFactions(GetFactionTag(ActorA), GetFactionTag(ActorB));
}
