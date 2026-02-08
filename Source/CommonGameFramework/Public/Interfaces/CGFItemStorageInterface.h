#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/CGFItemTypes.h"
#include "CGFItemStorageInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UCGFItemStorageInterface : public UInterface
{
	GENERATED_BODY()
};

class COMMONGAMEFRAMEWORK_API ICGFItemStorageInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage")
	bool SaveInventory(const FString& OwnerId, const TArray<FItemInstance>& Items);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage")
	TArray<FItemInstance> LoadInventory(const FString& OwnerId);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage")
	bool DeleteInventory(const FString& OwnerId);

	virtual void SaveInventoryAsync(const FString& OwnerId, const TArray<FItemInstance>& Items,
		const FOnStorageComplete& Callback) {}

	virtual void LoadInventoryAsync(const FString& OwnerId,
		const FOnStorageLoadComplete& Callback) {}
};
