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

	/**
	 * Transform of the equipped visual relative to AttachSocket: corrects for the skeleton's bone frame (a spine or head
	 * bone points along the chain, while item meshes are authored Z-up with +X forward). Per-item fit goes on the item
	 * (UItemFragment_Equipment::AttachOffset) and is applied on top of this.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FTransform AttachOffset = FTransform::Identity;

	/**
	 * Set on a sheath (the equipment sheathing plan, D77): the hand slot its item is drawn into (Equipment.Slot.MainHand).
	 * A sheath's item hangs at AttachSocket until drawn, then rides the hand with its hand fit; it grants its abilities and
	 * effects only while drawn. Empty = an ordinary slot.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Equipment.Slot"))
	FGameplayTag DrawSlotTag;

	/**
	 * A sheath whose item is in its hand whenever it can be (a shield on the back, drawn onto the off hand): drawn on its
	 * own as soon as the hand is free and nothing held blocks it (UItemFragment_Equipment::BlockedSlotTags), sheathed while
	 * something does. The equipment manager keeps it so; nothing asks for it. Only on a sheath (DrawSlotTag set).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bDrawWhenFree = false;
};
