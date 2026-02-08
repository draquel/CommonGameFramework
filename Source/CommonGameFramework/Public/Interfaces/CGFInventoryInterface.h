#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CGFInventoryInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFInventoryInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by actors that own inventory components.
 * Returns UActorComponent* — consumers in ItemSystem cast to UInventoryComponent*.
 */
class COMMONGAMEFRAMEWORK_API ICGFInventoryInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	UActorComponent* GetInventoryComponent() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	TArray<UActorComponent*> GetInventoryComponents() const;
};
