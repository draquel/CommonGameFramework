#pragma once

#include "CoreMinimal.h"
#include "CGFCommonEnums.generated.h"

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

UENUM(BlueprintType)
enum class EInventoryOperationResult : uint8
{
	Success,
	Pending,
	InvalidItem,
	InsufficientSpace,
	WeightExceeded,
	FilterRejected,
	NotAuthorized,
	ItemNotFound,
	SlotOccupied,
	StackFull,
	InvalidSlot,
	Failed
};

UENUM(BlueprintType)
enum class EInteractionResult : uint8
{
	Success,
	Failed,
	NotAllowed,
	OutOfRange,
	Cancelled,
	InProgress
};

UENUM(BlueprintType)
enum class EEquipmentResult : uint8
{
	Success,
	Failed,
	InvalidItem,
	IncompatibleSlot,
	SlotOccupied,
	NoInventorySpace,
	NotAuthorized
};
