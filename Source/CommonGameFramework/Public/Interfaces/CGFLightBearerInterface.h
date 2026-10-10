#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CGFLightBearerInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFLightBearerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by anything that can carry a light source (feature 7: light as a mechanic).
 *
 * The usual implementer is a character whose equipment holds a torch; consumers are things that
 * need light to be used (a dungeon sconce is lit from a carried flame) and perception that notices
 * a lit bearer. Which item provides the light, and how fuel is stored, is the implementer's
 * business — the equipment plugin reads a LightSource item fragment and burns the item's
 * durability as fuel.
 *
 * Both functions have a generated default (no light, nothing to consume), so an actor that
 * implements the interface without overriding is simply dark.
 */
class COMMONGAMEFRAMEWORK_API ICGFLightBearerInterface
{
	GENERATED_BODY()

public:
	/**
	 * Strength of the light carried right now: 0 = none (or unlit / out of fuel), about 1 for a
	 * torch (point-light lumens / 1000). Readable on every machine.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light")
	float GetCarriedLightLevel() const;

	/**
	 * Authority: spend fuel from the carried light (lighting a sconce from a torch).
	 * @param Seconds Fuel seconds to burn.
	 * @return False when nothing lit is carried, the light needs no fuel, or not on authority.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Light")
	bool ConsumeCarriedLightFuel(float Seconds);
};
