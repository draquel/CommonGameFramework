#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CGFInteractionSourceInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFInteractionSourceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by actors that perform interactions (players, AI).
 * Returns UActorComponent* — consumers in InteractionSystem cast to UInteractionComponent*.
 */
class COMMONGAMEFRAMEWORK_API ICGFInteractionSourceInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	UActorComponent* GetInteractionComponent() const;
};
