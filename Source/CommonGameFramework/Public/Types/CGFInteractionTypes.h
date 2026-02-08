#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/CGFCommonEnums.h"
#include "CGFInteractionTypes.generated.h"

// ---------------------------------------------------------------------------
// FInteractionOption — describes one way to interact with something
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInteractionOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag InteractionType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bRequiresHold = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bRequiresHold"))
	float HoldDuration = 0.0f;
};

// ---------------------------------------------------------------------------
// FInteractionContext — passed to interaction handlers
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInteractionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> Interactor;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> InteractableActor;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag InteractionType;

	UPROPERTY(BlueprintReadOnly)
	FVector InteractionLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float Distance = 0.0f;
};

// ---------------------------------------------------------------------------
// Delegates — Interaction
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFound, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableLost, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionStarted, const FInteractionContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionCompleted, const FInteractionContext&, Context, EInteractionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionFailed, const FInteractionContext&, Context, EInteractionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChanneledInteractionProgress, float, Progress);
