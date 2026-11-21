#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/CGFWaterTypes.h"
#include "CGFWaterSurfaceInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFWaterSurfaceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by what knows the world's water (the game layer: a voxel world's water levels, a level's water volumes),
 * and registered with UCGFWaterSubsystem. Framework actors that rest at a water surface (a dropped item) ask the
 * subsystem, so no framework plugin depends on a world plugin.
 *
 * The answer is the same on every machine for the same position: water comes from the world, not from replication.
 */
class COMMONGAMEFRAMEWORK_API ICGFWaterSurfaceInterface
{
	GENERATED_BODY()

public:
	/**
	 * The water over a world position.
	 *
	 * @param Location World position
	 * @param ReachAbove How far above a surface the position may be and still count as over water (a drop height)
	 * @param OutSample The surface over the position, its up and the position's depth; bWater says whether water is
	 *        really there (not rock, not a dry cave below the level)
	 * @return True when the provider knows the place (a surface exists over it), whatever bWater says
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CommonGameFramework|Water")
	bool SampleWaterSurface(const FVector& Location, float ReachAbove, FCGFWaterSurfaceSample& OutSample) const;
};
