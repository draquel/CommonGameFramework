#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Interfaces/CGFWaterSurfaceInterface.h"
#include "CGFWaterSubsystem.generated.h"

/** The water inside these world bounds changed (an invalid box: everywhere). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCGFOnWaterChanged, const FBox&, Bounds);

/**
 * Where the framework asks about the world's water: the game layer registers one provider
 * (ICGFWaterSurfaceInterface), and framework actors sample the surface through this subsystem. Without a provider
 * there is no water and every sample fails, so a framework plugin used alone behaves as it did.
 */
UCLASS()
class COMMONGAMEFRAMEWORK_API UCGFWaterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * The world's water subsystem.
	 * @param WorldContextObject Any object in the world
	 * @return The subsystem, or null without a world
	 */
	static UCGFWaterSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * Register the world's water (the game layer, once its world is known). Replaces any earlier provider and
	 * broadcasts OnWaterChanged for everywhere.
	 * @param InProvider An object implementing ICGFWaterSurfaceInterface (null clears)
	 */
	UFUNCTION(BlueprintCallable, Category = "CommonGameFramework|Water")
	void SetProvider(const TScriptInterface<ICGFWaterSurfaceInterface>& InProvider);

	/**
	 * Remove a provider, if it is the registered one (a provider going away clears only itself).
	 * @param InProvider The provider to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "CommonGameFramework|Water")
	void ClearProvider(const UObject* InProvider);

	/** @return True while a provider is registered and alive */
	UFUNCTION(BlueprintPure, Category = "CommonGameFramework|Water")
	bool HasProvider() const;

	/**
	 * The water over a world position, from the provider.
	 *
	 * @param Location World position
	 * @param ReachAbove How far above a surface the position may be and still count as over water
	 * @param OutSample The sample (reset when there is no answer)
	 * @return True when a provider answered; OutSample.bWater says whether water is there
	 */
	UFUNCTION(BlueprintCallable, Category = "CommonGameFramework|Water")
	bool SampleWaterSurface(const FVector& Location, float ReachAbove, FCGFWaterSurfaceSample& OutSample) const;

	/**
	 * Tell the listeners that the water inside some bounds changed (a body's level moved, a body appeared): what rests
	 * at a surface there settles again. The owner of the water calls it; nothing polls.
	 * @param Bounds World bounds of the change (an invalid box: everywhere)
	 */
	UFUNCTION(BlueprintCallable, Category = "CommonGameFramework|Water")
	void NotifyWaterChanged(const FBox& Bounds);

	/** Fires on SetProvider, ClearProvider and NotifyWaterChanged, on the machine that called them. */
	UPROPERTY(BlueprintAssignable, Category = "CommonGameFramework|Water")
	FCGFOnWaterChanged OnWaterChanged;

private:
	/** The registered provider (the game layer's); null: no water. */
	UPROPERTY()
	TScriptInterface<ICGFWaterSurfaceInterface> Provider;
};
