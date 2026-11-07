#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Types/CGFCommonEnums.h"
#include "CGFItemTypes.generated.h"

class UItemInstanceFragment;
class UActorComponent;

// ---------------------------------------------------------------------------
// UItemInstanceFragment — per-instance mutable data base class
// ---------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class COMMONGAMEFRAMEWORK_API UItemInstanceFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<FJsonObject> SerializeFragment() const;
	virtual void DeserializeFragment(const TSharedPtr<FJsonObject>& Data);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Fragment")
	FText GetFragmentDisplayText() const;

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual void OnAttachedToInstance(const struct FItemInstance& OwningInstance);
};

// ---------------------------------------------------------------------------
// FItemInstance — runtime item representation
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FItemInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly)
	FPrimaryAssetId ItemDefinitionId;

	UPROPERTY(BlueprintReadOnly)
	int32 StackCount = 1;

	UPROPERTY(BlueprintReadOnly, Instanced)
	TArray<TObjectPtr<UItemInstanceFragment>> InstanceFragments;

	bool IsValid() const;

	/** Identity: the same instance (the same InstanceId), whatever its count or fragments. Game code finds an instance with it. */
	bool operator==(const FItemInstance& Other) const;
	bool operator!=(const FItemInstance& Other) const;

	/**
	 * The reflection system's comparison (the WithIdentical trait): every field NetSerialize sends, and the fragment
	 * revision. Replication compares a property with the state it last sent through this, so a count changed in place (a
	 * partial removal, a stack added onto, a split) is a change and reaches the clients; with identity alone it never did
	 * (G135). The fragments are compared as objects and never dereferenced: replication's copy of the last sent state
	 * shares the live item's fragment objects without keeping them alive, so their contents look the same on both sides.
	 * A change to them shows as the revision EditFragment bumps, which that copy holds by value (G136).
	 * @param Other The item to compare with; null is no match.
	 * @param PortFlags The caller's port flags; the comparison is the same for every flag.
	 * @return True when the id, the definition, the count, the fragment objects and the fragment revision are all the same.
	 */
	bool Identical(const FItemInstance* Other, uint32 PortFlags) const;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	/**
	 * Find a fragment by class, to read it. A change goes through EditFragment.
	 * @return The first instance fragment of class T, or null.
	 */
	template<typename T>
	const T* FindFragment() const
	{
		for (const UItemInstanceFragment* Fragment : InstanceFragments)
		{
			if (const T* Typed = Cast<T>(Fragment))
			{
				return Typed;
			}
		}
		return nullptr;
	}

	/**
	 * Find a fragment by class to change its contents in place, and mark the item changed for replication: the fragment
	 * revision rises, so Identical no longer matches the state last sent and the item is sent again (G136). Call it on
	 * the item where it is held (an inventory slot's own item, an equipment slot's), each time a fragment changes, and do
	 * not keep the pointer past the change. A copy of an item shares its fragment objects with the original: a change
	 * through a copy lands on both and marks only the copy, so write the copy back (UInventoryComponent::TryReplaceItem
	 * with its own id) or the change never replicates. A held item in a fast array also needs its slot marked dirty,
	 * which the owning component's operations do.
	 * @return The first instance fragment of class T, or null (nothing is marked then).
	 */
	template<typename T>
	T* EditFragment()
	{
		for (UItemInstanceFragment* Fragment : InstanceFragments)
		{
			if (T* Typed = Cast<T>(Fragment))
			{
				++FragmentRevision;
				return Typed;
			}
		}
		return nullptr;
	}

private:
	/**
	 * How many times a fragment was changed through EditFragment (wrapping). Not a property, not sent and not saved: the
	 * value only has to differ from the one in replication's copy of the last sent state, into which it is copied by
	 * value with the rest of the item. A client's copy gets new fragment objects at every receive, so it needs none.
	 */
	uint32 FragmentRevision = 0;
};

template<>
struct TStructOpsTypeTraits<FItemInstance> : public TStructOpsTypeTraitsBase2<FItemInstance>
{
	enum
	{
		WithNetSerializer = true,
		WithIdentical = true, // not WithIdenticalViaEquality: operator== is identity, which hides a count change (G135)
	};
};

// ---------------------------------------------------------------------------
// Delegates — Inventory
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemAdded, const FItemInstance&, Item, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemRemoved, const FItemInstance&, Item, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemMoved, const FItemInstance&, Item, int32, OldSlot, int32, NewSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemUpdated, const FItemInstance&, Item, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

// ---------------------------------------------------------------------------
// Delegates — Equipment
// ---------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, const FItemInstance&, Item, FGameplayTag, SlotTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequipped, const FItemInstance&, Item, FGameplayTag, SlotTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

// ---------------------------------------------------------------------------
// Delegates — Storage
// ---------------------------------------------------------------------------

/** An asynchronous storage save or delete finished (ICGFItemStorageInterface): the inventory's id and whether it landed. */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnStorageComplete, const FString&, OwnerId, bool, bSuccess);
/** An asynchronous storage load finished: the inventory's id, whether the read succeeded, and the items read. */
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnStorageLoadComplete, const FString&, OwnerId, bool, bSuccess, const TArray<FItemInstance>&, Items);

// ---------------------------------------------------------------------------
// FInventorySlot — wraps FItemInstance for replication
// ---------------------------------------------------------------------------

struct FInventorySlotArray;

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInventorySlot : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FItemInstance Item;

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer SlotTags;

	UPROPERTY(BlueprintReadOnly)
	bool bIsOccupied = false;

	void PreReplicatedRemove(const FInventorySlotArray& ArraySerializer);
	void PostReplicatedAdd(const FInventorySlotArray& ArraySerializer);
	void PostReplicatedChange(const FInventorySlotArray& ArraySerializer);
};

// ---------------------------------------------------------------------------
// FInventorySlotArray — FFastArraySerializer for slot replication
// ---------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInventorySlotArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventorySlot> Items;

	// Not a UPROPERTY — UInventoryComponent is defined in a downstream plugin.
	// Set by the owning component during initialization.
	UActorComponent* OwningComponent = nullptr;

	// Replication callbacks — set by owning component to fire delegates on clients.
	// These avoid a compile-time dependency from CommonGameFramework → ItemInventoryPlugin.
	TFunction<void(const FInventorySlot&)> OnSlotAddedCallback;
	TFunction<void(const FInventorySlot&)> OnSlotRemovedCallback;
	TFunction<void(const FInventorySlot&)> OnSlotChangedCallback;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventorySlot, FInventorySlotArray>(Items, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FInventorySlotArray> : public TStructOpsTypeTraitsBase2<FInventorySlotArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
