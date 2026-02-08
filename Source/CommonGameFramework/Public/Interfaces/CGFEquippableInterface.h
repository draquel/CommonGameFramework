#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "CGFEquippableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFEquippableInterface : public UInterface
{
	GENERATED_BODY()
};

class COMMONGAMEFRAMEWORK_API ICGFEquippableInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
	FGameplayTag GetEquipmentSlot() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
	TArray<TSubclassOf<UGameplayAbility>> GetGrantedAbilities() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
	TArray<TSubclassOf<UGameplayEffect>> GetAppliedEffects() const;
};
