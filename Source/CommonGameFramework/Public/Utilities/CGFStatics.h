#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Types/CGFItemTypes.h"
#include "CGFStatics.generated.h"

UCLASS()
class COMMONGAMEFRAMEWORK_API UCGFStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Item helpers
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Item")
	static bool IsItemInstanceValid(const FItemInstance& Instance);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Item")
	static FString GetItemInstanceDebugString(const FItemInstance& Instance);

	// Tag helpers
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Tags")
	static bool ItemMatchesTagFilter(const FGameplayTagContainer& ItemTags,
		const FGameplayTagContainer& AllowedTags,
		const FGameplayTagContainer& DeniedTags);

	// GUID helpers
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Utility")
	static FGuid GenerateNewGuid();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Utility")
	static bool IsGuidValid(const FGuid& Guid);
};
