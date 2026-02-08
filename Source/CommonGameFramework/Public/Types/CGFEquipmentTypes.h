#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CGFEquipmentTypes.generated.h"

// ---------------------------------------------------------------------------
// FEquipmentSlotDefinition — configures one equipment slot
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FEquipmentSlotDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer AcceptedItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText SlotDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName AttachSocket;
};
