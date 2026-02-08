#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "Types/CGFCommonEnums.h"
#include "Types/CGFInteractionTypes.h"
#include "CGFInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class COMMONGAMEFRAMEWORK_API ICGFInteractableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor, const FInteractionContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	TArray<FInteractionOption> GetInteractionOptions(AActor* Interactor) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EInteractionResult Interact(AActor* Interactor, FGameplayTag InteractionType);
};
