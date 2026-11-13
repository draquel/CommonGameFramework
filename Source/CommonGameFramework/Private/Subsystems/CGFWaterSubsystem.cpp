#include "Subsystems/CGFWaterSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UCGFWaterSubsystem* UCGFWaterSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UCGFWaterSubsystem>() : nullptr;
}

void UCGFWaterSubsystem::SetProvider(const TScriptInterface<ICGFWaterSurfaceInterface>& InProvider)
{
	if (Provider.GetObject() == InProvider.GetObject())
	{
		return;
	}
	Provider = InProvider;
	// Whatever rested by the old answer settles by the new one.
	OnWaterChanged.Broadcast(FBox(ForceInit));
}

void UCGFWaterSubsystem::ClearProvider(const UObject* InProvider)
{
	if (InProvider && Provider.GetObject() == InProvider)
	{
		Provider = nullptr;
		OnWaterChanged.Broadcast(FBox(ForceInit));
	}
}

bool UCGFWaterSubsystem::HasProvider() const
{
	return IsValid(Provider.GetObject());
}

bool UCGFWaterSubsystem::SampleWaterSurface(const FVector& Location, float ReachAbove, FCGFWaterSurfaceSample& OutSample) const
{
	OutSample = FCGFWaterSurfaceSample();
	UObject* Object = Provider.GetObject();
	if (!IsValid(Object))
	{
		return false;
	}
	// Execute_ reaches a C++ implementation and a Blueprint one alike.
	return ICGFWaterSurfaceInterface::Execute_SampleWaterSurface(Object, Location, ReachAbove, OutSample);
}

void UCGFWaterSubsystem::NotifyWaterChanged(const FBox& Bounds)
{
	OnWaterChanged.Broadcast(Bounds);
}
