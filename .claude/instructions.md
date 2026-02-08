# CommonGameFramework — Plugin Instructions

## Purpose

This is the foundation module that all other game framework plugins depend on. It contains shared types, interfaces, gameplay tag declarations, and utility functions. **This module must have zero dependencies on any other game framework plugin.**

## Documentation

This plugin's `Documentation/` folder contains:
- `ARCHITECTURE.md` — The master architecture document for the entire plugin suite. Covers system architecture, data flow, replication model, cross-plugin integration, performance strategies, and the phased implementation plan.
- `COMMON_TYPES.md` — Detailed reference for all shared types, interfaces, and gameplay tags defined in this module.

**Read `Documentation/ARCHITECTURE.md` before starting any new phase of implementation across any plugin.**

## Module Structure

```
CommonGameFramework/
├── Source/
│   └── CommonGameFramework/
│       ├── Public/
│       │   ├── Interfaces/
│       │   │   ├── CGFInteractableInterface.h       ← IInteractable: anything the player can interact with
│       │   │   ├── CGFInventoryInterface.h           ← IInventoryOwner: any actor that owns inventories
│       │   │   ├── CGFEquippableInterface.h          ← IEquippable: items that can be equipped
│       │   │   ├── CGFItemStorageInterface.h         ← IItemStorage: persistence abstraction
│       │   │   └── CGFInteractionSourceInterface.h   ← IInteractionSource: actors that perform interactions
│       │   ├── Types/
│       │   │   ├── CGFItemTypes.h                    ← FItemInstance, FItemInstanceHandle, FInventorySlot
│       │   │   ├── CGFInteractionTypes.h             ← FInteractionOption, FInteractionContext, EInteractionResult
│       │   │   ├── CGFEquipmentTypes.h               ← FEquipmentSlotDefinition, EEquipmentResult
│       │   │   ├── CGFLootTypes.h                    ← FLootContext, FLootResult
│       │   │   └── CGFCommonEnums.h                  ← EItemRarity, EInventoryOperation, shared enums
│       │   ├── Tags/
│       │   │   └── CGFGameplayTags.h                 ← Native gameplay tag declarations
│       │   ├── Utilities/
│       │   │   └── CGFStatics.h                      ← Static helper functions, math, validation
│       │   └── CommonGameFramework.h                 ← Module API macro
│       └── Private/
│           ├── Tags/
│           │   └── CGFGameplayTags.cpp
│           ├── Utilities/
│           │   └── CGFStatics.cpp
│           └── CommonGameFramework.cpp
└── CommonGameFramework.uplugin
```

## Key Design Decisions

### FItemInstance (The Core Struct)

This is the most important type in the entire system. Every plugin touches it.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FItemInstance
{
    GENERATED_BODY()

    // Globally unique ID for this specific instance — stable across network
    UPROPERTY(BlueprintReadOnly)
    FGuid InstanceId;

    // Reference to the item definition data asset (PrimaryAssetId for async loading)
    UPROPERTY(BlueprintReadOnly)
    FPrimaryAssetId ItemDefinitionId;

    // Current stack count
    UPROPERTY(BlueprintReadOnly)
    int32 StackCount = 1;

    // Instance-specific mutable data (durability, enchantments, etc.)
    // Uses instanced sub-objects for polymorphic per-instance state
    UPROPERTY(BlueprintReadOnly, Instanced)
    TArray<TObjectPtr<UItemInstanceFragment>> InstanceFragments;

    bool IsValid() const;
    bool operator==(const FItemInstance& Other) const;

    // Net serialization
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FItemInstance> : public TStructOpsTypeTraitsBase2<FItemInstance>
{
    enum
    {
        WithNetSerializer = true,
        WithIdenticalViaEquality = true,
    };
};
```

**Critical rules for FItemInstance:**
- Always create with a valid FGuid (FGuid::NewGuid() on the server, never on the client)
- InstanceId is the primary key for all operations — never reference items by array index across the network
- ItemDefinitionId uses FPrimaryAssetId so the actual data asset can be async loaded
- StackCount must always be >= 1 for valid instances (0 means the instance should be removed)
- InstanceFragments are UObject-based (not struct) because they need polymorphism for the fragment pattern — this is the ONE place we use UObjects for item data, and they're instanced subobjects owned by the inventory component

### Instance Fragments vs Definition Fragments

- **Definition Fragments** (`UItemDefinitionFragment`, defined in ItemInventoryPlugin): Live on the item data asset. Static, shared across all instances. Example: "this is a weapon with base damage 50."
- **Instance Fragments** (`UItemInstanceFragment`, defined here in Common): Live on FItemInstance. Per-instance mutable state. Example: "this specific sword has 73/100 durability."

Instance fragment base class lives in CommonGameFramework because both the EquipmentPlugin and ItemInventoryPlugin need to work with them. Definition fragment base class lives in ItemInventoryPlugin because only that plugin defines item data assets.

### Gameplay Tags

Declare all shared tag roots as native gameplay tags in CGFGameplayTags.h:

```
Item.Category.Weapon
Item.Category.Armor
Item.Category.Consumable
Item.Category.Material
Item.Category.Quest
Item.Category.Misc
Item.Rarity.Common
Item.Rarity.Uncommon
Item.Rarity.Rare
Item.Rarity.Epic
Item.Rarity.Legendary
Inventory.Type.Player
Inventory.Type.Container
Inventory.Type.NPC
Inventory.Type.Loot
Equipment.Slot.Head
Equipment.Slot.Chest
Equipment.Slot.Legs
Equipment.Slot.Feet
Equipment.Slot.Hands
Equipment.Slot.MainHand
Equipment.Slot.OffHand
Equipment.Slot.Accessory1
Equipment.Slot.Accessory2
Interaction.Type.Pickup
Interaction.Type.Drop
Interaction.Type.Use
Interaction.Type.Open
Interaction.Type.Equip
Interaction.Type.Inspect
```

These are the starting set. Plugins extend with their own tags but these roots must be defined here so cross-plugin code can reference them.

### Interfaces

**IInteractable** — Implemented by anything in the world that can be interacted with.
- `CanInteract(AActor* Interactor, FInteractionContext Context) → bool`
- `GetInteractionOptions(AActor* Interactor) → TArray<FInteractionOption>`
- `Interact(AActor* Interactor, FGameplayTag InteractionType) → EInteractionResult`

**IInventoryOwner** — Implemented by actors that have inventory components.
- `GetInventoryComponent() → UInventoryComponent*`
- `GetInventoryComponents() → TArray<UInventoryComponent*>` (for actors with multiple inventories)

**IEquippable** — Queried on item definitions to determine equipment behavior.
- `GetEquipmentSlot() → FGameplayTag`
- `GetGrantedAbilities() → TArray<TSubclassOf<UGameplayAbility>>`
- `GetAppliedEffects() → TArray<TSubclassOf<UGameplayEffect>>`

**IItemStorage** — Persistence abstraction.
- `SaveInventory(FString OwnerId, TArray<FItemInstance> Items) → bool`
- `LoadInventory(FString OwnerId) → TArray<FItemInstance>`
- `SaveInventoryAsync(FString OwnerId, TArray<FItemInstance> Items, FOnStorageComplete Callback)`
- `LoadInventoryAsync(FString OwnerId, FOnStorageComplete Callback)`
- `DeleteInventory(FString OwnerId) → bool`

**IInteractionSource** — Implemented by actors that perform interactions (players, AI).
- `GetInteractionComponent() → UInteractionComponent*`

## Build.cs Dependencies

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "GameplayTags",
    "GameplayAbilities",   // For GAS types in interfaces
    "GameplayTasks",
    "NetCore",             // For FFastArraySerializer
});
```

GAS modules are included here because the interfaces reference GAS types. This does NOT mean CommonGameFramework implements GAS logic — it only uses the types for interface declarations.

## Rules for This Module

1. **No gameplay logic.** This module defines types and contracts, not behavior.
2. **No dependencies on other game framework plugins.** If you find yourself needing to #include from ItemInventoryPlugin or EquipmentPlugin, the type belongs here instead.
3. **Every type must be fully serializable** — both for SaveGame and for network replication.
4. **Every interface function must have a Blueprint-callable wrapper** where the UInterface pattern requires it.
5. **Changes here affect all plugins.** Be conservative. Add types only when they genuinely need to be shared.

## Implementation Phase

This module is **Phase 1**. It must be fully implemented and compiling before any other plugin work begins. Implementation order within this module:

1. Module setup (Build.cs, .uplugin, module class)
2. Common enums and basic types (CGFCommonEnums.h, simple structs)
3. FItemInstance and related structs (CGFItemTypes.h)
4. Interaction types (CGFInteractionTypes.h)
5. Equipment types (CGFEquipmentTypes.h)
6. Loot types (CGFLootTypes.h)
7. Gameplay tag declarations (CGFGameplayTags.h/.cpp)
8. All interfaces
9. Utility functions
