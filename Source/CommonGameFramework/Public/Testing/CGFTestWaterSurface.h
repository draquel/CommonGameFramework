#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Interfaces/CGFWaterSurfaceInterface.h"
#include "CGFTestWaterSurface.generated.h"

/**
 * A water provider for tests of what rests at a water surface: one surface, a plane at a height or a sphere about a
 * centre, with places under it that hold no water. A test registers it with UCGFWaterSubsystem::SetProvider (the
 * framework plugins' own tests, and the game module's replication test, whose map has no voxel world).
 *
 * Not for a game: a world's water is the game layer's to provide. It is a class and so cannot be compiled for
 * automation builds only, as the other helpers in this folder are.
 */
UCLASS()
class COMMONGAMEFRAMEWORK_API UCGFTestWaterSurface : public UObject, public ICGFWaterSurfaceInterface
{
	GENERATED_BODY()

public:
	/** The surface is a sphere about Center (a planet's sea) instead of a plane across world Z. */
	UPROPERTY()
	bool bRadial = false;

	/** The sphere's centre (bRadial). */
	UPROPERTY()
	FVector Center = FVector::ZeroVector;

	/** The surface's level: its world Z, or the sphere's radius. */
	UPROPERTY()
	double Level = 0.0;

	/** Places under the level that hold no water (a dry cave, the inside of a hull). */
	UPROPERTY()
	TArray<FBox> DryBounds;

	// ICGFWaterSurfaceInterface
	virtual bool SampleWaterSurface_Implementation(const FVector& Location, float ReachAbove, FCGFWaterSurfaceSample& OutSample) const override;
};
