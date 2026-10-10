#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "CGFRestPointInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFRestPointInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by places a character can rest at (feature 8: campsites).
 *
 * The rest point owns what resting means (heal, remember the spot as the respawn point, skip the
 * night) and which crafting station it offers; the character's UI only asks. All mutating calls are
 * authority-only and return false when refused. Generated defaults: nothing happens, no station.
 */
class COMMONGAMEFRAMEWORK_API ICGFRestPointInterface
{
	GENERATED_BODY()

public:
	/** Authority: rest here (restore vitals, remember as the respawn point). @return False when refused. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rest")
	bool Rest(AActor* User);

	/** Authority: sleep through the night to the next morning (and rest). @return False by day or when refused. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rest")
	bool SleepUntilMorning(AActor* User);

	/** True while sleeping is allowed (night / dusk). Readable everywhere. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rest")
	bool CanSleepNow() const;

	/** Crafting.Station.* this rest point offers (empty = no crafting here). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Rest")
	FGameplayTag GetCraftingStationTag() const;
};
