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

/**
 * Persistence for inventories and opaque JSON documents (ItemInventoryPlugin's local and remote backends).
 *
 * The synchronous calls suit a backend that answers at once (files). A backend that waits on a network uses the
 * asynchronous calls: they return at once and report on the game thread through their callback (G93). For one OwnerId,
 * operations land in the order they were requested, and of overlapping saves the last one requested is the one that
 * lands: a save superseded by a later one reports the later one's result.
 */
class COMMONGAMEFRAMEWORK_API ICGFItemStorageInterface
{
	GENERATED_BODY()

public:
	/**
	 * Store an inventory, replacing what OwnerId had.
	 * @param OwnerId The inventory's storage id
	 * @param Items   Its items
	 * @return True when stored (a backend that cannot wait returns true when the save is accepted and logs its outcome)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage")
	bool SaveInventory(const FString& OwnerId, const TArray<FItemInstance>& Items);
	/** The default stores nothing. Declared here because the asynchronous defaults below call it. */
	virtual bool SaveInventory_Implementation(const FString& OwnerId, const TArray<FItemInstance>& Items) { return false; }

	/**
	 * Read an inventory.
	 * @param OwnerId The inventory's storage id
	 * @return Its items; empty when nothing is stored, when the read failed, or when the backend cannot wait
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage")
	TArray<FItemInstance> LoadInventory(const FString& OwnerId);
	/** The default reads nothing. */
	virtual TArray<FItemInstance> LoadInventory_Implementation(const FString& OwnerId) { return TArray<FItemInstance>(); }

	/**
	 * Remove an inventory.
	 * @param OwnerId The inventory's storage id
	 * @return True when removed or absent (a backend that cannot wait returns true when the request is accepted)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage")
	bool DeleteInventory(const FString& OwnerId);
	/** The default removes nothing. */
	virtual bool DeleteInventory_Implementation(const FString& OwnerId) { return false; }

	// --- Asynchronous (G93): return at once, report through the callback on the game thread ---

	/**
	 * Store an inventory without waiting. A backend that answers at once may run the callback before returning.
	 * @param OwnerId  The inventory's storage id
	 * @param Items    Its items (copied before returning)
	 * @param Callback Runs once with (OwnerId, bSuccess): whether this save, or a later one of OwnerId that superseded it, landed
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage|Async")
	void SaveInventoryAsync(const FString& OwnerId, const TArray<FItemInstance>& Items, const FOnStorageComplete& Callback);
	/** The default: the synchronous SaveInventory, then the callback at once. */
	virtual void SaveInventoryAsync_Implementation(const FString& OwnerId, const TArray<FItemInstance>& Items,
		const FOnStorageComplete& Callback)
	{
		const bool bSuccess = SaveInventory_Implementation(OwnerId, Items);
		Callback.ExecuteIfBound(OwnerId, bSuccess);
	}

	/**
	 * Read an inventory without waiting; it is read after every operation of OwnerId requested before it.
	 * @param OwnerId  The inventory's storage id
	 * @param Callback Runs once with (OwnerId, bSuccess, Items): bSuccess false when the read failed (keep what you have);
	 *                 true with no items when nothing is stored
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage|Async")
	void LoadInventoryAsync(const FString& OwnerId, const FOnStorageLoadComplete& Callback);
	/** The default: the synchronous LoadInventory, then the callback at once (an empty result counts as success). */
	virtual void LoadInventoryAsync_Implementation(const FString& OwnerId, const FOnStorageLoadComplete& Callback)
	{
		const TArray<FItemInstance> Items = LoadInventory_Implementation(OwnerId);
		Callback.ExecuteIfBound(OwnerId, true, Items);
	}

	/**
	 * Remove an inventory without waiting; it is removed after every operation of OwnerId requested before it.
	 * @param OwnerId  The inventory's storage id
	 * @param Callback Runs once with (OwnerId, bSuccess)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Storage|Async")
	void DeleteInventoryAsync(const FString& OwnerId, const FOnStorageComplete& Callback);
	/** The default: the synchronous DeleteInventory, then the callback at once. */
	virtual void DeleteInventoryAsync_Implementation(const FString& OwnerId, const FOnStorageComplete& Callback)
	{
		const bool bSuccess = DeleteInventory_Implementation(OwnerId);
		Callback.ExecuteIfBound(OwnerId, bSuccess);
	}

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
};
