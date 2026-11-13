#pragma once

#include "CoreMinimal.h"
#include "CGFWaterTypes.generated.h"

/**
 * The water over a world position, as a framework actor needs it to rest at a surface
 * (ICGFWaterSurfaceInterface::SampleWaterSurface, UCGFWaterSubsystem::SampleWaterSurface).
 *
 * The framework knows no world: what water is, where its surface lies and which way is up come from the game layer's
 * provider. A sample is taken per position and never kept as the world's level: a world may have several bodies of
 * water, and a body's level may change.
 */
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FCGFWaterSurfaceSample
{
	GENERATED_BODY()

	/** Water is there: the position is in it, or its surface lies within the reach asked below the position. */
	UPROPERTY(BlueprintReadWrite, Category = "Water")
	bool bWater = false;

	/** The position's depth under the surface: positive in the water, negative above it. */
	UPROPERTY(BlueprintReadWrite, Category = "Water")
	double Depth = 0.0;

	/** The point of the surface straight over (or under) the position, along Up. */
	UPROPERTY(BlueprintReadWrite, Category = "Water")
	FVector SurfacePoint = FVector::ZeroVector;

	/** The surface's up there (unit): world +Z on a flat world, the direction from the centre on a planet. */
	UPROPERTY(BlueprintReadWrite, Category = "Water")
	FVector Up = FVector::UpVector;
};
