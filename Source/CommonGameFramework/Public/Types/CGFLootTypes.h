#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/CGFItemTypes.h"
#include "CGFLootTypes.generated.h"

// ---------------------------------------------------------------------------
// FLootContext — input parameters for loot generation
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FLootContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer ContextTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PartySize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LuckModifier = 1.0f;

	UPROPERTY(BlueprintReadOnly)
	FRandomStream RandomStream;
};

// ---------------------------------------------------------------------------
// FLootResult — output of loot generation
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FLootResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FItemInstance> Items;

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;
};
