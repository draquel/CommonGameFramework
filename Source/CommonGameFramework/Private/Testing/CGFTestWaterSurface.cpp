#include "Testing/CGFTestWaterSurface.h"

bool UCGFTestWaterSurface::SampleWaterSurface_Implementation(const FVector& Location, float ReachAbove, FCGFWaterSurfaceSample& OutSample) const
{
	OutSample = FCGFWaterSurfaceSample();
	if (bRadial)
	{
		const FVector FromCenter = Location - Center;
		const double Distance = FromCenter.Size();
		OutSample.Up = Distance > UE_SMALL_NUMBER ? FromCenter / Distance : FVector::UpVector;
		OutSample.Depth = Level - Distance;
	}
	else
	{
		OutSample.Up = FVector::UpVector;
		OutSample.Depth = Level - Location.Z;
	}
	OutSample.SurfacePoint = Location + OutSample.Up * OutSample.Depth;

	// In the water, or within the reach over it; and not in a place the water does not enter. Over the surface the
	// place that counts is the water just under it.
	const FVector Wet = OutSample.Depth > 0.0 ? Location : OutSample.SurfacePoint - OutSample.Up;
	OutSample.bWater = OutSample.Depth > -static_cast<double>(ReachAbove);
	for (const FBox& Dry : DryBounds)
	{
		if (Dry.IsInsideOrOn(Wet))
		{
			OutSample.bWater = false;
			break;
		}
	}
	return true;
}
