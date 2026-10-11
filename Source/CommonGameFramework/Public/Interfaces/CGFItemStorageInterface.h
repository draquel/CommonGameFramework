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

	// --- Opaque documents (feature 9: world saves ride the same backend) ---

	/** Store a JSON document under Key (overwrites). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage|Blob")
	bool SaveBlob(const FString& Key, const FString& Json);

	/** Fetch the document stored under Key. @return False when there is none. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage|Blob")
	bool LoadBlob(const FString& Key, FString& OutJson);

	/** Remove the document stored under Key. @return False when there was none. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage|Blob")
	bool DeleteBlob(const FString& Key);

	/** True when a document is stored under Key. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage|Blob")
	bool HasBlob(const FString& Key);

	virtual void SaveInventoryAsync(const FString& OwnerId, const TArray<FItemInstance>& Items,
		const FOnStorageComplete& Callback) {}

	virtual void LoadInventoryAsync(const FString& OwnerId,
		const FOnStorageLoadComplete& Callback) {}
};
