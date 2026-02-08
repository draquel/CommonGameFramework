# CommonGameFramework — Shared Types Reference

## Overview

This document is the definitive reference for every type, interface, and gameplay tag defined in CommonGameFramework. All other plugins depend on these definitions. Changes here ripple across the entire plugin suite — modify conservatively.

---

## Core Structs

### FItemInstance

The runtime representation of a single item (or item stack). This is the most referenced type in the system.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FItemInstance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid InstanceId;                    // Globally unique, server-generated

    UPROPERTY(BlueprintReadOnly)
    FPrimaryAssetId ItemDefinitionId;    // Points to UItemDefinition asset

    UPROPERTY(BlueprintReadOnly)
    int32 StackCount = 1;               // >= 1 for valid instances

    UPROPERTY(BlueprintReadOnly, Instanced)
    TArray<TObjectPtr<UItemInstanceFragment>> InstanceFragments;  // Mutable per-instance data

    bool IsValid() const;
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};
```

**Lifecycle rules:**
- Created only on the server via `UItemDatabaseSubsystem::CreateItemInstance()`
- Identified across the network by `InstanceId` (never by array index)
- `StackCount = 0` means the instance should be removed — no code should hold a reference to it after this point
- `ItemDefinitionId` is loaded on-demand; game code should go through `UItemDatabaseSubsystem` to resolve it, never load it directly
- `InstanceFragments` are owned UObjects instanced per-item; they're the one place UObjects are used for per-item data

**Serialization:**
- Network: Custom `NetSerialize` via `TStructOpsTypeTraits` (WithNetSerializer)
- Storage: JSON-serializable via `FJsonObjectConverter`; each fragment serializes its own data
- Equality: Two instances are equal if `InstanceId` matches (WithIdenticalViaEquality)

### UItemInstanceFragment (Base Class)

```cpp
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class COMMONGAMEFRAMEWORK_API UItemInstanceFragment : public UObject
{
    GENERATED_BODY()
public:
    // Override to provide JSON-serializable data for persistence
    virtual TSharedPtr<FJsonObject> SerializeFragment() const;
    virtual void DeserializeFragment(const TSharedPtr<FJsonObject>& Data);

    // Override to provide display data for UI
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Fragment")
    FText GetFragmentDisplayText() const;

    // Network serialization support
    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

    // Called when this fragment is added to an item instance
    virtual void OnAttachedToInstance(const FItemInstance& OwningInstance);
};
```

**Rules:**
- Always `EditInlineNew` + `DefaultToInstanced` so they can be edited inline on item definitions and created as instanced subobjects
- Subclasses define their own `UPROPERTY` fields for mutable state
- Must implement `SerializeFragment`/`DeserializeFragment` for persistence to work
- Must implement `NetSerialize` for replication to work
- Lives in CommonGameFramework (not ItemInventoryPlugin) because both Equipment and Item plugins need to work with instance fragments

### FInventorySlot

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInventorySlot : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FItemInstance Item;

    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = -1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer SlotTags;    // Optional filtering (e.g., hotbar, ammo)

    UPROPERTY(BlueprintReadOnly)
    bool bIsOccupied = false;

    // FFastArraySerializerItem callbacks
    void PreReplicatedRemove(const struct FInventorySlotArray& ArraySerializer);
    void PostReplicatedAdd(const struct FInventorySlotArray& ArraySerializer);
    void PostReplicatedChange(const struct FInventorySlotArray& ArraySerializer);
};
```

**Rules:**
- Inherits `FFastArraySerializerItem` for per-element replication
- `SlotIndex` is stable — never changes after initialization
- Removing an item sets `bIsOccupied = false` and clears `Item`; slot remains in the array
- `SlotTags` are set at configuration time (editor or BeginPlay), not at runtime

### FInteractionOption

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInteractionOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag InteractionType;      // Interaction.Type.Pickup, etc.

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayText;                 // "Pick Up", "Open", "Talk"

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Priority = 0;               // Higher = shown first in UI

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bRequiresHold = false;        // True = channeled interaction

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bRequiresHold"))
    float HoldDuration = 0.0f;        // Seconds to hold
};
```

### FInteractionContext

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInteractionContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> Interactor;           // Who is interacting

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<AActor> InteractableActor;    // What is being interacted with

    UPROPERTY(BlueprintReadOnly)
    FGameplayTag InteractionType;                // Which interaction

    UPROPERTY(BlueprintReadOnly)
    FVector InteractionLocation;                 // World location of interaction

    UPROPERTY(BlueprintReadOnly)
    float Distance;                              // Distance between actors
};
```

### FEquipmentSlotDefinition

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FEquipmentSlotDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag SlotTag;              // Equipment.Slot.Head, etc.

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer AcceptedItemTags;  // Tag filter for this slot

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText SlotDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AttachSocket;                // Skeletal mesh socket for visual attachment
};
```

### FLootContext

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FLootContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 1;                   // Player/enemy level for level-gated drops

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ContextTags; // Situational tags: biome, difficulty, etc.

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PartySize = 1;              // For party-scaled drops

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LuckModifier = 1.0f;        // Multiplier on rare drop chances

    UPROPERTY(BlueprintReadOnly)
    FRandomStream RandomStream;        // Seeded RNG for deterministic testing
};
```

---

## Enums

### EItemRarity
```
Common, Uncommon, Rare, Epic, Legendary
```
Maps to `Item.Rarity.*` gameplay tags. The enum exists for C++ switch statements and comparisons; the tag is used for data-driven queries.

### EInventoryOperationResult
```
Success, Pending, InvalidItem, InsufficientSpace, WeightExceeded, FilterRejected,
NotAuthorized, ItemNotFound, SlotOccupied, StackFull, InvalidSlot, Failed
```

### EInteractionResult
```
Success, Failed, NotAllowed, OutOfRange, Cancelled, InProgress
```

### EEquipmentResult
```
Success, Failed, InvalidItem, IncompatibleSlot, SlotOccupied,
NoInventorySpace (for unequip-to-inventory), NotAuthorized
```

---

## Interfaces

### IInteractable

Implemented by: `UInteractableComponent`, `AWorldItem`, any custom interactable actor.

```cpp
UINTERFACE(MinimalAPI, BlueprintType)
class UCGFInteractableInterface : public UInterface { GENERATED_BODY() };

class COMMONGAMEFRAMEWORK_API ICGFInteractableInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool CanInteract(AActor* Interactor, const FInteractionContext& Context) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    TArray<FInteractionOption> GetInteractionOptions(AActor* Interactor) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    EInteractionResult Interact(AActor* Interactor, FGameplayTag InteractionType);
};
```

### IInventoryOwner

Implemented by: Any actor with `UInventoryComponent` (players, NPCs, containers).

```cpp
UINTERFACE(MinimalAPI, BlueprintType)
class UCGFInventoryInterface : public UInterface { GENERATED_BODY() };

class COMMONGAMEFRAMEWORK_API ICGFInventoryInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
    UInventoryComponent* GetInventoryComponent() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
    TArray<UInventoryComponent*> GetInventoryComponents() const;
};
```

**Note**: `UInventoryComponent*` is forward-declared here. The actual class lives in ItemInventoryPlugin. This is acceptable because the interface only returns a pointer — CommonGameFramework never calls methods on it. Consumers that call methods on the returned pointer already depend on ItemInventoryPlugin.

### IEquippable

Queried against item data to determine equipment behavior. Typically implemented by `UItemFragment_Equipment`.

```cpp
UINTERFACE(MinimalAPI, BlueprintType)
class UCGFEquippableInterface : public UInterface { GENERATED_BODY() };

class COMMONGAMEFRAMEWORK_API ICGFEquippableInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
    FGameplayTag GetEquipmentSlot() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
    TArray<TSubclassOf<UGameplayAbility>> GetGrantedAbilities() const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equipment")
    TArray<TSubclassOf<UGameplayEffect>> GetAppliedEffects() const;
};
```

### IItemStorage

Persistence abstraction. Implemented by: `ULocalItemStorage`, `URemoteItemStorage`, any custom backend.

```cpp
UINTERFACE(MinimalAPI, BlueprintType)
class UCGFItemStorageInterface : public UInterface { GENERATED_BODY() };

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

    // Async variants use delegate callbacks
    virtual void SaveInventoryAsync(const FString& OwnerId, const TArray<FItemInstance>& Items,
        const FOnStorageComplete& Callback) = 0;

    virtual void LoadInventoryAsync(const FString& OwnerId,
        const FOnStorageLoadComplete& Callback) = 0;
};
```

### IInteractionSource

Implemented by: Any actor that performs interactions (player characters, AI pawns).

```cpp
UINTERFACE(MinimalAPI, BlueprintType)
class UCGFInteractionSourceInterface : public UInterface { GENERATED_BODY() };

class COMMONGAMEFRAMEWORK_API ICGFInteractionSourceInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    UInteractionComponent* GetInteractionComponent() const;
};
```

---

## Gameplay Tags

All tags below are declared as native gameplay tags in `CGFGameplayTags.h` using `UE_DEFINE_GAMEPLAY_TAG_COMMENT`.

### Item Tags
```
Item.Category.Weapon          — Weapons (swords, bows, staves)
Item.Category.Armor           — Armor pieces
Item.Category.Consumable      — Potions, food, scrolls
Item.Category.Material        — Crafting materials, resources
Item.Category.Quest           — Quest items (often non-droppable)
Item.Category.Misc            — Everything else

Item.Rarity.Common
Item.Rarity.Uncommon
Item.Rarity.Rare
Item.Rarity.Epic
Item.Rarity.Legendary
```

### Inventory Tags
```
Inventory.Type.Player         — Player's personal inventory
Inventory.Type.Container      — World containers (chests, barrels)
Inventory.Type.NPC            — NPC inventories (vendors, enemies)
Inventory.Type.Loot           — Temporary loot drop inventories
```

### Equipment Tags
```
Equipment.Slot.Head
Equipment.Slot.Chest
Equipment.Slot.Legs
Equipment.Slot.Feet
Equipment.Slot.Hands
Equipment.Slot.MainHand
Equipment.Slot.OffHand
Equipment.Slot.Accessory1
Equipment.Slot.Accessory2
```

### Interaction Tags
```
Interaction.Type.Pickup       — Pick up an item
Interaction.Type.Drop         — Drop an item
Interaction.Type.Use          — Use/activate (context-dependent)
Interaction.Type.Open         — Open a container, door, etc.
Interaction.Type.Equip        — Equip an item directly from world
Interaction.Type.Inspect      — Examine/read without taking
```

### Extending Tags

Plugins and game code can add child tags under any of these roots. For example:
- `Item.Category.Weapon.Melee.Sword` (game-specific)
- `Interaction.Type.Hack` (game-specific)
- `Equipment.Slot.Mount` (game-specific)

New root-level tag namespaces should only be added to CommonGameFramework when they represent a concept shared across multiple plugins.

---

## Utility Functions (CGFStatics)

```cpp
UCLASS()
class COMMONGAMEFRAMEWORK_API UCGFStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Item helpers
    UFUNCTION(BlueprintCallable, Category = "CGF|Item")
    static bool IsItemInstanceValid(const FItemInstance& Instance);

    UFUNCTION(BlueprintCallable, Category = "CGF|Item")
    static FString GetItemInstanceDebugString(const FItemInstance& Instance);

    // Tag helpers
    UFUNCTION(BlueprintCallable, Category = "CGF|Tags")
    static bool ItemMatchesTagFilter(const FGameplayTagContainer& ItemTags,
        const FGameplayTagContainer& AllowedTags,
        const FGameplayTagContainer& DeniedTags);

    // GUID helpers
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Utility")
    static FGuid GenerateNewGuid();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CGF|Utility")
    static bool IsGuidValid(const FGuid& Guid);
};
```

---

## Delegate Signatures

Declared in `CGFItemTypes.h` for cross-plugin use:

```cpp
// Inventory events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemAdded, const FItemInstance&, Item, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemRemoved, const FItemInstance&, Item, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemMoved, const FItemInstance&, Item, int32, OldSlot, int32, NewSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemUpdated, const FItemInstance&, Item, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

// Equipment events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, const FItemInstance&, Item, FGameplayTag, SlotTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequipped, const FItemInstance&, Item, FGameplayTag, SlotTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

// Interaction events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFound, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableLost, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionStarted, const FInteractionContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionCompleted, const FInteractionContext&, Context, EInteractionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionFailed, const FInteractionContext&, Context, EInteractionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChanneledInteractionProgress, float, Progress);

// Storage callbacks
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnStorageComplete, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnStorageLoadComplete, bool, bSuccess, const TArray<FItemInstance>&, Items);
```
