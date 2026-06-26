# Architecture & Implementation Strategy

## Document Purpose

This document is the technical blueprint for building the Game Framework Plugins. It covers the system architecture, data flow between subsystems, the multiplayer replication model, key design patterns, and a phased implementation plan broken down to the individual file level for use with Claude Code.

Read this alongside `CLAUDE.md` (conventions) and each plugin's `.claude/instructions.md` (class details). This document focuses on **how the pieces fit together** and **the order in which to build them**.

---

## 1. System Architecture Overview

### 1.1 Layered Architecture

The system is organized into four layers. Higher layers depend on lower layers but never the reverse.

```
┌──────────────────────────────────────────────────────────────┐
│  GAME LAYER (not part of plugins — game-specific code)       │
│  Game mode, player controller, UI widgets, quest system      │
│  Binds to plugin events, uses plugin APIs, extends via BP    │
├──────────────────────────────────────────────────────────────┤
│  INTEGRATION LAYER                                           │
│  Cross-plugin interactions handled via interfaces & tags     │
│  AWorldItem, Equipment-from-Inventory, Interaction→Pickup    │
├──────────────────────────────────────────────────────────────┤
│  PLUGIN LAYER                                                │
│  ItemInventoryPlugin │ InteractionPlugin │ EquipmentPlugin   │
│  Each plugin is a self-contained system with its own API     │
├──────────────────────────────────────────────────────────────┤
│  FOUNDATION LAYER                                            │
│  CommonGameFramework                                         │
│  Shared types, interfaces, tags, utilities                   │
├──────────────────────────────────────────────────────────────┤
│  ENGINE LAYER                                                │
│  Unreal Engine 5.8: GAS, NetCore, AssetManager, etc.         │
└──────────────────────────────────────────────────────────────┘
```

### 1.2 Module Dependency Map (Compile-Time)

```
                    ┌─────────────────────┐
                    │ CommonGameFramework │
                    │   (Foundation)       │
                    └──────┬──┬──┬────────┘
                           │  │  │
              ┌────────────┘  │  └────────────┐
              │               │               │
              ▼               ▼               ▼
   ┌──────────────┐  ┌───────────────┐  ┌──────────────────┐
   │  ItemSystem   │  │ Interaction   │  │ EquipmentSystem  │
   │  (Runtime)    │  │ System        │  │ (Runtime)        │
   └──────┬───────┘  └───────┬───────┘  └──────┬───────────┘
          │                  │                  │
          │                  │                  ▼
          │                  │          ┌──────────────────┐
          │                  │          │ EquipmentGAS     │
          │                  │          │ Integration      │
          │                  │          └──────────────────┘
          │                  │
          ▼                  ▼
   ┌──────────────┐  ┌───────────────────┐
   │ ItemSystem   │  │ (InteractionSystem│
   │ Editor       │  │ uses ItemSystem   │
   └──────────────┘  │ for AWorldItem)   │
                     └───────────────────┘
```

**Key rule**: Arrows point downward. No module ever depends on something above it or at the same level (except InteractionSystem's private dependency on ItemSystem for AWorldItem, which is an acknowledged coupling).

### 1.3 Runtime Object Ownership

```
APlayerCharacter (Game Layer)
├── UInventoryComponent              ← Owns item instances, replicated
├── UEquipmentManagerComponent       ← References equipped items, replicated
├── UInteractionComponent            ← Detects & dispatches interactions
└── UAbilitySystemComponent (GAS)    ← Receives abilities/effects from equipment

AWorldItem (InteractionPlugin)
├── UStaticMeshComponent             ← Visual representation
├── UInteractableComponent           ← Makes it interactable
└── FItemInstance (data only)        ← The item this actor represents

ALootContainer (Game Layer Actor)
├── UInventoryComponent              ← Holds loot items
├── UInteractableComponent           ← "Open" interaction
└── (Populated via LootGenerationSubsystem at spawn time)

UGameInstance
└── UItemDatabaseSubsystem           ← Caches item definitions, handles async loading

UWorld
├── ULootGenerationSubsystem         ← Rolls loot tables, creates FItemInstances
└── UWorldItemPoolSubsystem          ← Object pool for AWorldItem actors
```

---

## 2. Data Architecture

### 2.1 Item Data Lifecycle

An item's data exists in three forms throughout its lifecycle:

```
DEFINITION (Design-Time)          INSTANCE (Runtime)             ACTOR (World)
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│ UItemDefinition     │     │ FItemInstance        │     │ AWorldItem          │
│ (PrimaryDataAsset)  │     │ (Struct)             │     │ (Actor)             │
│                     │     │                      │     │                      │
│ • DisplayName       │────▶│ • InstanceId (GUID)  │────▶│ • MeshComponent     │
│ • Icon              │     │ • ItemDefinitionId   │     │ • InteractableComp  │
│ • ItemTags          │     │ • StackCount         │     │ • Physics/Collision  │
│ • MaxStackSize      │     │ • InstanceFragments  │     │ • FItemInstance copy │
│ • Weight            │     │   (mutable state)    │     │                      │
│ • DefFragments      │     │                      │     │                      │
│   (static data)     │     │ Lives in:            │     │ Lives in:            │
│ • DefaultInstFrags  │     │ • InventoryComponent │     │ • UWorld             │
│                     │     │ • EquipmentManager   │     │ • Object Pool        │
│ Lives in:           │     │ • LootResult         │     │                      │
│ • AssetRegistry     │     │ • Storage payload    │     │                      │
│ • Content folder    │     │                      │     │                      │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
    One per item type          Many per item type           One per visible item
    Shared, immutable          Lightweight, owned           Heavyweight, pooled
```

**Transitions between forms:**

```
Definition → Instance:   ItemDatabaseSubsystem.CreateItemInstance(AssetId, Count)
                          Copies DefaultInstanceFragments, generates GUID
                          SERVER ONLY — clients never create instances

Instance → Actor:        WorldItemPool.SpawnWorldItem(FItemInstance)
                          Creates/recycles AWorldItem, sets mesh from definition
                          Called on: drop from inventory, loot spawn, world placement

Actor → Instance:        Pickup interaction extracts FItemInstance from AWorldItem
                          AWorldItem returned to pool
                          FItemInstance added to inventory component

Instance → Storage:      IItemStorage.SaveInventory(OwnerId, Items)
                          Serializes FItemInstance array to JSON/binary

Storage → Instance:      IItemStorage.LoadInventory(OwnerId)
                          Deserializes, validates definitions still exist
```

### 2.2 Fragment Architecture

Fragments follow a dual-layer pattern inspired by Lyra's item/equipment model:

```
UItemDefinition
├── TArray<UItemDefinitionFragment*> Fragments        ← STATIC, shared
│   ├── UItemFragment_Weapon       { BaseDamage, FireRate, DamageType }
│   ├── UItemFragment_Equipment    { SlotTag, EquipMesh, GrantedAbilities }
│   └── UItemFragment_WorldDisplay { WorldMesh, WorldMaterial, DropVFX }
│
└── TArray<UItemInstanceFragment*> DefaultInstanceFragments  ← TEMPLATE for instances
    └── UInstanceFragment_Durability { MaxDurability = 100 }

FItemInstance (created from above definition)
└── TArray<UItemInstanceFragment*> InstanceFragments  ← MUTABLE, per-instance
    └── UInstanceFragment_Durability { CurrentDurability = 73 }
```

**How fragment lookup works at runtime:**

```cpp
// To get static data (weapon damage), look at the definition:
UItemDefinition* Def = ItemDatabaseSubsystem->GetDefinition(Instance.ItemDefinitionId);
UItemFragment_Weapon* WeaponFrag = Def->FindFragment<UItemFragment_Weapon>();
float Damage = WeaponFrag->BaseDamage;

// To get instance data (current durability), look at the instance:
UInstanceFragment_Durability* DurFrag = Instance.FindFragment<UInstanceFragment_Durability>();
float CurrentDur = DurFrag->CurrentDurability;
```

Game code should **never** cache fragment pointers across frames for instance fragments — the instance may be moved, stacked, or destroyed. Definition fragments are safe to cache since definitions are immutable.

### 2.3 Inventory Slot Model

```
UInventoryComponent
└── FInventorySlotArray (FFastArraySerializer)
    ├── FInventorySlot [0]  { FItemInstance, SlotIndex=0, SlotTags={} }
    ├── FInventorySlot [1]  { FItemInstance, SlotIndex=1, SlotTags={} }
    ├── FInventorySlot [2]  { <empty>,       SlotIndex=2, SlotTags={} }
    ├── FInventorySlot [3]  { FItemInstance, SlotIndex=3, SlotTags={Equipment.Slot.Hotbar} }
    └── ...
```

**FInventorySlot** extends `FFastArraySerializerItem` for per-element replication:

```cpp
USTRUCT(BlueprintType)
struct FInventorySlot : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FItemInstance Item;

    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = -1;

    // Optional: tag-based slot filtering (e.g., hotbar slots, ammo slots)
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer SlotTags;

    UPROPERTY(BlueprintReadOnly)
    bool bIsOccupied = false;

    // FFastArraySerializerItem callbacks
    void PreReplicatedRemove(const FInventorySlotArray& ArraySerializer);
    void PostReplicatedAdd(const FInventorySlotArray& ArraySerializer);
    void PostReplicatedChange(const FInventorySlotArray& ArraySerializer);
};
```

**FInventorySlotArray** is the `FFastArraySerializer` wrapper:

```cpp
USTRUCT(BlueprintType)
struct FInventorySlotArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FInventorySlot> Items;

    // Back-reference to owning component (set during init)
    UPROPERTY(NotReplicated)
    TObjectPtr<UInventoryComponent> OwningComponent = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
};
```

Slots are **pre-allocated** during `BeginPlay` based on `MaxSlots`. Adding an item fills an existing empty slot rather than appending to the array. Removing an item marks the slot as empty rather than removing the array element. This keeps slot indices stable — critical for UI binding and cross-network references.

---

## 3. Multiplayer Architecture

### 3.1 Authority Model

```
┌─────────────────────────────────────────────────────────────┐
│                     DEDICATED SERVER                         │
│                                                              │
│  All FItemInstances are authoritative here                  │
│  All validation happens here                                │
│  All GUID generation happens here                           │
│  All inventory mutations happen here                        │
│  All equipment changes happen here                          │
│  All loot generation happens here                           │
│                                                              │
│  Replication pushes state to clients via:                    │
│  • FFastArraySerializer (inventory slots)                   │
│  • Standard UPROPERTY replication (equipment slots)          │
│  • Multicast RPCs (one-shot events like VFX)                │
└──────────┬────────────────────────────┬─────────────────────┘
           │                            │
           ▼                            ▼
┌─────────────────────┐     ┌─────────────────────┐
│    CLIENT A          │     │    CLIENT B          │
│                      │     │                      │
│ Read-only replicated │     │ Read-only replicated │
│ inventory state      │     │ inventory state      │
│                      │     │                      │
│ Sends Server RPCs    │     │ Sends Server RPCs    │
│ for all mutations    │     │ for all mutations    │
│                      │     │                      │
│ UI updates on OnRep  │     │ UI updates on OnRep  │
└─────────────────────┘     └─────────────────────┘
```

### 3.2 Inventory Operation Flow (Multiplayer)

Example: Player picks up a world item.

```
CLIENT                                 SERVER
──────                                 ──────
1. InteractionComponent detects        
   AWorldItem as best target           
                                       
2. Player presses Interact key         
                                       
3. InteractionComponent calls          
   TryInteractWithTarget()             
                                       
4. Sends ServerRPC_RequestInteract ──► 5. Server receives RPC
   (TargetActor, InteractionTag)          
                                       6. Validates:
                                          • Target actor exists & is valid
                                          • Target is within range
                                          • Target implements IInteractable
                                          • CanInteract() returns true
                                       
                                       7. Executes interaction:
                                          • Extracts FItemInstance from AWorldItem
                                          • Calls InventoryComponent->Internal_AddItem()
                                          • Returns AWorldItem to pool (destroys/hides)
                                       
                                       8. FFastArraySerializer detects dirty slot
                                       
                                       9. Replication pushes delta ──────────►
                                       
10. Client receives OnRep              
    • PostReplicatedAdd fires          
    • OwningComponent broadcasts       
      OnItemAdded delegate             
                                       
11. UI updates (bound to              
    OnItemAdded)                       
```

### 3.3 Operation Validation Pattern

Every inventory/equipment operation uses a shared validation pattern:

```cpp
// Public API — called by game code or Blueprint
EInventoryOperationResult UInventoryComponent::TryAddItem(const FItemInstance& Item, int32 PreferredSlot)
{
    // Step 1: Run validation (same logic, client or server)
    FInventoryOperationData OpData;
    if (!ValidateAddItem(Item, PreferredSlot, /*out*/ OpData))
    {
        return OpData.Result;  // Failure reason
    }

    // Step 2: Route based on authority
    if (GetOwner()->HasAuthority())
    {
        // We ARE the server — execute directly
        Internal_AddItem(Item, OpData.ResolvedSlot);
        return EInventoryOperationResult::Success;
    }
    else
    {
        // We are a client — request from server
        ServerRPC_RequestAddItem(Item, PreferredSlot);
        // Optionally: predict locally for responsive UI
        return EInventoryOperationResult::Pending;
    }
}

// Server RPC — received on server
void UInventoryComponent::ServerRPC_RequestAddItem_Implementation(
    const FItemInstance& Item, int32 PreferredSlot)
{
    // Re-validate on server (never trust client)
    FInventoryOperationData OpData;
    if (!ValidateAddItem(Item, PreferredSlot, /*out*/ OpData))
    {
        // Optionally notify client of failure via ClientRPC
        ClientRPC_OperationFailed(OpData.Result);
        return;
    }

    Internal_AddItem(Item, OpData.ResolvedSlot);
    // FFastArraySerializer handles replication automatically
}

// Validation — pure logic, no side effects, reusable
bool UInventoryComponent::ValidateAddItem(
    const FItemInstance& Item, int32 PreferredSlot,
    FInventoryOperationData& OutData) const
{
    if (!Item.IsValid())
    {
        OutData.Result = EInventoryOperationResult::InvalidItem;
        return false;
    }

    if (MaxWeight > 0.0f && GetCurrentWeight() + GetItemWeight(Item) > MaxWeight)
    {
        OutData.Result = EInventoryOperationResult::WeightExceeded;
        return false;
    }

    if (!PassesTagFilter(Item))
    {
        OutData.Result = EInventoryOperationResult::FilterRejected;
        return false;
    }

    int32 ResolvedSlot = ResolveSlotForItem(Item, PreferredSlot);
    if (ResolvedSlot == INDEX_NONE)
    {
        OutData.Result = EInventoryOperationResult::InsufficientSpace;
        return false;
    }

    OutData.ResolvedSlot = ResolvedSlot;
    OutData.Result = EInventoryOperationResult::Success;
    return true;
}
```

This pattern applies to **every mutation** across all three plugins. Validation is always a separate, const, side-effect-free function.

### 3.4 Cross-Inventory Atomic Transfers

Moving items between two inventories (player → chest, player → player trade) requires atomicity:

```cpp
EInventoryOperationResult UInventoryComponent::TryMoveItem(
    const FGuid& InstanceId, UInventoryComponent* TargetInventory, int32 TargetSlot)
{
    // Must validate BOTH sides before touching either
    FItemInstance* SourceItem = FindItemByGuid(InstanceId);
    if (!SourceItem) return EInventoryOperationResult::InvalidItem;

    // Validate target can accept
    FInventoryOperationData TargetOpData;
    if (!TargetInventory->ValidateAddItem(*SourceItem, TargetSlot, TargetOpData))
    {
        return TargetOpData.Result;
    }

    // Validate source can release
    FInventoryOperationData SourceOpData;
    if (!ValidateRemoveItem(InstanceId, SourceItem->StackCount, SourceOpData))
    {
        return SourceOpData.Result;
    }

    // BOTH valid — execute atomically (server only reaches here)
    FItemInstance ItemCopy = *SourceItem;  // Copy before removal
    Internal_RemoveItem(InstanceId, ItemCopy.StackCount);
    TargetInventory->Internal_AddItem(ItemCopy, TargetOpData.ResolvedSlot);

    return EInventoryOperationResult::Success;
}
```

**No partial execution.** If either validation fails, nothing happens. The copy-before-remove pattern ensures the item data isn't lost if something unexpected happens between the two operations.

---

## 4. Cross-Plugin Integration Architecture

### 4.1 Interface-Based Communication

Plugins never directly reference each other's concrete classes (with the acknowledged exception of InteractionSystem→ItemSystem for AWorldItem). All cross-plugin communication goes through interfaces defined in CommonGameFramework.

```
                    CommonGameFramework
                    ┌─────────────────┐
                    │  IInteractable   │◄──── InteractionPlugin implements on UInteractableComponent
                    │  IInventoryOwner │◄──── ItemInventoryPlugin implements on UInventoryComponent
                    │  IEquippable     │◄──── EquipmentPlugin queries against item definitions
                    │  IItemStorage    │◄──── ItemInventoryPlugin implements (Local, Remote)
                    │  IInteractSrc    │◄──── InteractionPlugin implements on UInteractionComponent
                    └─────────────────┘
```

### 4.2 Gameplay Tag Communication

Tags are the primary loose-coupling mechanism between plugins:

```
ITEM defines:        Item.Category.Weapon, Item.Rarity.Rare
EQUIPMENT reads:     Item.Category.Weapon → validates slot compatibility
INTERACTION reads:   Interaction.Type.Pickup → dispatches to pickup handler
INVENTORY reads:     Inventory.Type.Player → determines replication rules
LOOT reads:          Item.Rarity.* → conditions on loot entries
```

No plugin needs to know about another plugin's classes to use tags. A new interaction type, item category, or equipment slot is just a new tag — no code changes in other plugins.

### 4.3 Event/Delegate Communication

The event flow across plugins when a player picks up an item and equips it:

```
InteractionPlugin                  ItemInventoryPlugin               EquipmentPlugin
─────────────────                  ───────────────────               ───────────────
OnInteractionCompleted ──►
  (Pickup succeeded)          OnItemAdded ──►
                                (New item in inventory)
                                                                 (Player opens inventory,
                                                                  selects equip)
                              OnItemRemoved ──►
                                (Item left inventory)         OnItemEquipped ──►
                                                                (Sword in MainHand slot)
                                                                
                                                              GAS: GrantAbility()
                                                              Visual: AttachMesh()
```

Each plugin fires its own delegates. The **game layer** (player controller, UI, quest system) binds to whichever delegates it needs. Plugins do NOT bind to each other's delegates — that would create runtime coupling.

### 4.4 Integration Sequence Diagrams

**Full Pickup Flow:**
```
Player → InteractionComponent → IInteractable (on AWorldItem)
  → AWorldItem.OnPickupInteraction()
    → AWorldItem extracts FItemInstance
    → Gets UInventoryComponent from player (via IInventoryOwner)
    → Calls InventoryComponent->TryAddItem(ItemInstance)
      → Server validates, adds to slot
      → FFastArraySerializer replicates
    → AWorldItem returns to object pool
    → InteractionComponent fires OnInteractionCompleted
```

**Full Equip Flow:**
```
UI (Game Layer) → EquipmentManagerComponent.TryEquip(ItemInstance)
  → Validates item has Equipment fragment
  → Validates slot is available/compatible
  → Calls InventoryComponent->TryRemoveItem(InstanceId) [if inventory-backed]
    → Server validates, removes from slot
  → Sets item in equipment slot
  → Reads Equipment fragment for mesh data
  → Spawns/attaches visual component
  → If GAS module loaded:
    → EquipmentAbilityGranter.GrantAbilities(ASC, Abilities)
    → EquipmentAbilityGranter.ApplyEffects(ASC, Effects)
  → Fires OnItemEquipped delegate
  → Replicates equipment slot to clients
```

**Full Loot Container Flow:**
```
Level Designer places ALootContainer in level with:
  → UInventoryComponent (empty, configured with MaxSlots)
  → UInteractableComponent (Interaction.Type.Open)
  → LootTable reference

On BeginPlay (Server):
  → LootGenerationSubsystem.GenerateLoot(LootTable, Context)
  → Returns TArray<FItemInstance>
  → InventoryComponent->TryAddItem() for each result
  → Inventory replicates to clients in range

Player interacts (Open):
  → InteractionComponent sends ServerRPC
  → Server validates, fires IInteractable.Interact()
  → Game Layer opens loot UI showing container's inventory
  → Player drags items → TryMoveItem between inventories
```

---

## 5. Performance Architecture

### 5.1 Memory Model

```
LOW COST (thousands OK):
  FItemInstance              ← ~64-128 bytes per instance (struct)
  FInventorySlot             ← ~160 bytes (FItemInstance + metadata)
  FLootEntry                 ← ~96 bytes (struct, design-time only)

MEDIUM COST (hundreds OK):
  UItemInstanceFragment      ← UObject, but small and instanced
  UInventoryComponent        ← One per actor with inventory

HIGH COST (tens OK, pool these):
  AWorldItem                 ← Full actor with mesh, collision, physics
  UItemDefinition            ← Loaded on demand, cached in subsystem
```

### 5.2 Performance Strategies

**Object Pooling (AWorldItem)**
```
UWorldItemPoolSubsystem:
  PoolSize = 50 (configurable)
  
  BeginPlay → Pre-spawn 50 AWorldItems, deactivate all
  
  SpawnWorldItem(FItemInstance):
    If pool has inactive item → activate, InitializeFromItem(), return
    Else → SpawnActor (pool exhausted, expand)
  
  ReturnWorldItem(AWorldItem):
    ResetForPool() → clear mesh, clear item data, deactivate
    Return to pool
```

**Async Asset Loading**
```
Item definition references use FPrimaryAssetId (not direct pointers).
Meshes use TSoftObjectPtr (not direct pointers).
Icons use TSoftObjectPtr.

Loading pattern:
  1. ItemDatabaseSubsystem caches definitions on first access
  2. World item mesh loads async → item appears after load completes
  3. UI icon loads async → placeholder until loaded
  
  Never block the game thread on asset loading in gameplay paths.
```

**Inventory Replication Optimization**
```
FFastArraySerializer benefits:
  • Only dirty elements replicate (not full array)
  • Adding 1 item to a 50-slot inventory sends ~160 bytes, not 8000

Additional optimizations:
  • Batch operations: if adding 5 loot items, mark all dirty then let
    one replication pass send all 5 (not 5 separate passes)
  • Relevancy: container inventories only replicate to players who have
    opened them (set bOnlyRelevantToOwner until opened)
  • Dormancy: static containers go dormant when closed
```

**Detection Throttling (InteractionComponent)**
```
Detection does NOT run every frame.
  • Timer-based: every 0.1s (configurable DetectionTickRate)
  • Only traces/overlaps for the local player's interaction component
  • Server does NOT run detection — only validates on RPC receipt
  • Number of detection candidates limited to nearest N (default 10)
```

### 5.3 Scalability Targets

The architecture should support these scenarios without degradation:

| Scenario | Target |
|----------|--------|
| Items in a single inventory | 100 slots |
| Active inventories in world | 500 (players + containers) |
| World item actors simultaneously | 200 (pooled) |
| Item definitions in database | 10,000+ |
| Loot table entries | 500 per table |
| Concurrent players (server) | 64 |
| Inventory operations per second (server) | 1000 |

---

## 6. Persistence Architecture

### 6.1 Storage Abstraction

```
UInventoryComponent
  │
  │ bIsDirty = true (after any mutation)
  │
  ▼
UItemStorageSubsystem (UGameInstanceSubsystem)
  │
  │ Checks dirty inventories on timer / level transition / manual save
  │
  ├──► IItemStorage (interface)
  │     │
  │     ├── ULocalItemStorage
  │     │   └── JSON files in SaveGames/Inventories/{OwnerId}.json
  │     │
  │     └── URemoteItemStorage
  │         └── HTTP POST to configurable endpoint
  │
  │ Selected via UItemSystemSettings (UDeveloperSettings)
  │
  └──► Serialization format (shared by all implementations):
       {
         "ownerId": "Player_0",
         "inventoryType": "Inventory.Type.Player",
         "slots": [
           {
             "slotIndex": 0,
             "itemDefinitionId": "ItemDefinition:ID_Sword_Iron",
             "instanceId": "550e8400-e29b-41d4-a716-446655440000",
             "stackCount": 1,
             "instanceFragments": [
               {
                 "fragmentType": "InstanceFragment_Durability",
                 "data": { "currentDurability": 73.0 }
               }
             ]
           },
           // ...
         ]
       }
```

### 6.2 Save/Load Timing

```
AUTO-SAVE triggers:
  • Timer (default 30s, configurable)
  • Level transition (PreStreamingLevel)
  • Player disconnect
  • Explicit game request (e.g., checkpoint)

LOAD triggers:
  • Player login (server loads inventory, adds to component, replicates)
  • Level load (world containers loaded from level save data)

DIRTY TRACKING:
  • Every Internal_* mutation sets bIsDirty = true
  • Auto-save checks bIsDirty, only saves if true
  • After save completes, bIsDirty = false
  • Prevents redundant writes
```

---

## 7. Extension Points

These are the hooks game developers use to customize behavior without modifying plugin code:

| Extension Point | Mechanism | Example Use |
|-----------------|-----------|-------------|
| New item fragment | Subclass `UItemDefinitionFragment` | Add "Enchantable" fragment with enchantment slots |
| New instance fragment | Subclass `UItemInstanceFragment` | Track "soulbound" state, kill count |
| Custom interaction | Implement `IInteractable` | NPC dialogue trigger, vehicle mount |
| Custom detection | Subclass `UInteractionDetectionStrategy` | Cone-based detection for stealth game |
| Custom equipment visual | Override `OnPostEquip` | Modular character mesh swapping |
| Custom storage backend | Implement `IItemStorage` | Direct database, cloud save provider |
| Custom loot conditions | Extend `FLootContext` | Weather-based drops, time-of-day gating |
| Inventory slot rules | Configure `SlotTags` + `AllowedItemTags` | "This slot only accepts ammo" |
| New interaction types | Add gameplay tags | `Interaction.Type.Hack`, `Interaction.Type.Tame` |

---

## 8. Phased Implementation Plan

### Phase Overview

```
Phase 1:  CommonGameFramework          Foundation types & interfaces
Phase 2:  Item Definitions             Data assets & fragments
Phase 3:  Item Instances               Runtime structs & serialization
Phase 4:  Inventory (Single Player)    Component, operations, queries
Phase 5:  Inventory (Multiplayer)      Replication, RPCs, validation
Phase 6:  Interaction System           Detection, interaction flow, world items
Phase 7:  Loot Tables                  Data assets, generation, preview tools
Phase 8:  Equipment System             Slots, equip/unequip, visuals
Phase 9:  GAS Integration             Abilities, effects, attribute sets
Phase 10: Storage System               Local & remote persistence
Phase 11: Editor & Debug Tools         Browsers, previews, console commands
Phase 12: Testing & Example Content    Automation tests, sample assets
```

### Phase 1: CommonGameFramework

**Goal**: All shared types compile. Other plugins can reference them. No gameplay logic.

**Files to create in order:**

```
1.  CommonGameFramework.uplugin
2.  Source/CommonGameFramework/CommonGameFramework.Build.cs
3.  Source/CommonGameFramework/Public/CommonGameFramework.h          (Module API macro)
4.  Source/CommonGameFramework/Private/CommonGameFramework.cpp        (Module impl)
5.  Source/CommonGameFramework/Public/Types/CGFCommonEnums.h          (EItemRarity, EInventoryOperationResult, EInteractionResult, EEquipmentResult)
6.  Source/CommonGameFramework/Public/Types/CGFItemTypes.h            (FItemInstance, FInventorySlot, UItemInstanceFragment base)
7.  Source/CommonGameFramework/Private/Types/CGFItemTypes.cpp         (NetSerialize, IsValid, operators)
8.  Source/CommonGameFramework/Public/Types/CGFInteractionTypes.h     (FInteractionOption, FInteractionContext)
9.  Source/CommonGameFramework/Public/Types/CGFEquipmentTypes.h       (FEquipmentSlotDefinition)
10. Source/CommonGameFramework/Public/Types/CGFLootTypes.h            (FLootContext, FLootResult)
11. Source/CommonGameFramework/Public/Tags/CGFGameplayTags.h          (Native tag declarations)
12. Source/CommonGameFramework/Private/Tags/CGFGameplayTags.cpp       (Tag registration)
13. Source/CommonGameFramework/Public/Interfaces/CGFInteractableInterface.h
14. Source/CommonGameFramework/Public/Interfaces/CGFInventoryInterface.h
15. Source/CommonGameFramework/Public/Interfaces/CGFEquippableInterface.h
16. Source/CommonGameFramework/Public/Interfaces/CGFItemStorageInterface.h
17. Source/CommonGameFramework/Public/Interfaces/CGFInteractionSourceInterface.h
18. Source/CommonGameFramework/Public/Utilities/CGFStatics.h
19. Source/CommonGameFramework/Private/Utilities/CGFStatics.cpp
```

**Compile gate**: All 19 files compile. Can be referenced as a dependency from another module's Build.cs.

**Claude Code task prompt pattern:**
```
"Create file N of Phase 1: [filename]. Follow the architecture in 
.claude/instructions.md. This file should [specific purpose]. 
It depends on files 1-[N-1] which are already created."
```

### Phase 2: Item Definitions

**Goal**: Item data assets can be created in the editor with fragments.

**Files to create:**

```
1.  ItemInventoryPlugin.uplugin
2.  Source/ItemSystem/ItemSystem.Build.cs
3.  Source/ItemSystem/Public/ItemSystem.h
4.  Source/ItemSystem/Private/ItemSystem.cpp
5.  Source/ItemSystem/Public/Data/ItemDefinitionFragment.h            (Base class)
6.  Source/ItemSystem/Private/Data/ItemDefinitionFragment.cpp
7.  Source/ItemSystem/Public/Data/ItemDefinition.h                    (PrimaryDataAsset)
8.  Source/ItemSystem/Private/Data/ItemDefinition.cpp                 (GetPrimaryAssetId, FindFragment)
9.  Source/ItemSystem/Public/Data/Fragments/ItemFragment_Weapon.h
10. Source/ItemSystem/Public/Data/Fragments/ItemFragment_Consumable.h
11. Source/ItemSystem/Public/Data/Fragments/ItemFragment_Durability.h
12. Source/ItemSystem/Public/Data/Fragments/ItemFragment_Stackable.h
13. Source/ItemSystem/Public/Data/Fragments/ItemFragment_Equipment.h
14. Source/ItemSystem/Public/Data/Fragments/ItemFragment_WorldDisplay.h
15. Source/ItemSystem/Private/Data/Fragments/ItemFragment_Weapon.cpp       (if needed)
16. Source/ItemSystem/Public/Subsystems/ItemDatabaseSubsystem.h
17. Source/ItemSystem/Private/Subsystems/ItemDatabaseSubsystem.cpp
```

**Compile gate**: Can create UItemDefinition assets in editor, add fragments, browse via ItemDatabaseSubsystem.

### Phase 3: Item Instances

**Goal**: FItemInstance can be created from definitions, serialized, and deserialized.

**Files to create:**

```
1.  Source/ItemSystem/Public/Types/ItemInstanceFragments.h            (Built-in: DurabilityState, CustomDataMap)
2.  Source/ItemSystem/Private/Types/ItemInstanceFragments.cpp
    (FItemInstance itself is in Common — this phase adds the instance fragment subclasses
     and wires up ItemDatabaseSubsystem.CreateItemInstance())
3.  Update: Source/ItemSystem/Private/Subsystems/ItemDatabaseSubsystem.cpp
    (Add CreateItemInstance function that copies DefaultInstanceFragments)
```

**Compile gate**: `ItemDatabaseSubsystem->CreateItemInstance(AssetId, 5)` returns a valid FItemInstance with correct GUID, stack count, and instance fragments.

### Phase 4: Inventory Component (Single Player)

**Goal**: Full inventory operations work in standalone (no networking).

**Files to create:**

```
1.  Source/ItemSystem/Public/Types/InventoryOperationResult.h
2.  Source/ItemSystem/Public/Components/InventoryComponent.h
3.  Source/ItemSystem/Private/Components/InventoryComponent.cpp
    (All Try* operations, Validate* functions, Internal_* functions,
     Query functions, delegate declarations — all non-networked)
```

**Compile gate**: Unit tests pass for add, remove, move, split, merge, swap, including edge cases (full inventory, weight limit, tag filtering, partial stack overflow).

### Phase 5: Inventory Replication

**Goal**: Inventory works correctly in dedicated server topology.

**Files to create/modify:**

```
1.  Source/ItemSystem/Public/Types/FastArrayInventory.h               (FInventorySlotArray)
2.  Source/ItemSystem/Private/Types/FastArrayInventory.cpp
3.  Update: Source/ItemSystem/Public/Components/InventoryComponent.h  (Add RPCs, switch to FastArray)
4.  Update: Source/ItemSystem/Private/Components/InventoryComponent.cpp
    (Implement ServerRPCs, authority routing, OnRep wiring)
```

**Compile gate**: Two PIE clients can see each other's inventory changes. Server validates and rejects invalid operations.

### Phase 6: Interaction System

**Goal**: Players can interact with world objects. Items can be picked up and dropped.

**Files to create:**

```
1.  InteractionPlugin.uplugin
2.  Source/InteractionSystem/InteractionSystem.Build.cs
3.  Source/InteractionSystem/Public/InteractionSystem.h
4.  Source/InteractionSystem/Private/InteractionSystem.cpp
5.  Source/InteractionSystem/Public/Detection/InteractionDetectionStrategy.h
6.  Source/InteractionSystem/Public/Detection/SphereOverlapDetection.h
7.  Source/InteractionSystem/Private/Detection/SphereOverlapDetection.cpp
8.  Source/InteractionSystem/Public/Detection/LineTraceDetection.h
9.  Source/InteractionSystem/Private/Detection/LineTraceDetection.cpp
10. Source/InteractionSystem/Public/Components/InteractableComponent.h
11. Source/InteractionSystem/Private/Components/InteractableComponent.cpp
12. Source/InteractionSystem/Public/Components/InteractionComponent.h
13. Source/InteractionSystem/Private/Components/InteractionComponent.cpp
    (Detection loop, scoring, target management, channeled interactions, RPCs)
14. Source/InteractionSystem/Public/Actors/WorldItem.h
15. Source/InteractionSystem/Private/Actors/WorldItem.cpp
    (InitializeFromItem, pickup handler, pool support)
16. Source/InteractionSystem/Public/Subsystems/WorldItemPoolSubsystem.h
17. Source/InteractionSystem/Private/Subsystems/WorldItemPoolSubsystem.cpp
```

**Compile gate**: Player walks near a world item, sees interaction prompt (via delegate), presses interact, item appears in inventory. Drop from inventory spawns world item. Works in multiplayer.

### Phase 7: Loot Tables

**Goal**: Designers can create loot tables and generate random loot.

**Files to create:**

```
1.  Source/ItemSystem/Public/Data/LootEntry.h
2.  Source/ItemSystem/Public/Data/LootTable.h
3.  Source/ItemSystem/Private/Data/LootTable.cpp                     (Validation in PostEditChangeProperty)
4.  Source/ItemSystem/Public/Subsystems/LootGenerationSubsystem.h
5.  Source/ItemSystem/Private/Subsystems/LootGenerationSubsystem.cpp
    (GenerateLoot, ProcessEntry, ResolveNestedTable, WeightedRandom)
```

**Editor tools (ItemSystemEditor module):**
```
6.  Source/ItemSystemEditor/ItemSystemEditor.Build.cs
7.  Source/ItemSystemEditor/Public/ItemSystemEditor.h
8.  Source/ItemSystemEditor/Private/ItemSystemEditor.cpp
9.  Source/ItemSystemEditor/Public/LootTablePreview.h
10. Source/ItemSystemEditor/Private/LootTablePreview.cpp
```

**Compile gate**: Create a loot table asset in editor, call GenerateLoot, get valid FItemInstances. Preview tool shows distribution over N rolls.

### Phase 8: Equipment System

**Goal**: Items can be equipped to slots with visual attachment.

**Files to create:**

```
1.  EquipmentPlugin.uplugin
2.  Source/EquipmentSystem/EquipmentSystem.Build.cs
3.  Source/EquipmentSystem/Public/EquipmentSystem.h
4.  Source/EquipmentSystem/Private/EquipmentSystem.cpp
5.  Source/EquipmentSystem/Public/Types/EquipmentSystemTypes.h       (FEquipmentSlot runtime struct)
6.  Source/EquipmentSystem/Public/Components/EquipmentManagerComponent.h
7.  Source/EquipmentSystem/Private/Components/EquipmentManagerComponent.cpp
    (TryEquip, TryUnequip, validation, visual attachment, replication, RPCs)
```

**Compile gate**: Equip a weapon from inventory → mesh appears on character. Unequip → mesh disappears, item returns to inventory. Works in multiplayer.

### Phase 9: GAS Integration

**Goal**: Equipping items grants abilities and applies effects.

**Files to create:**

```
1.  Source/EquipmentGASIntegration/EquipmentGASIntegration.Build.cs
2.  Source/EquipmentGASIntegration/Public/EquipmentGASIntegration.h
3.  Source/EquipmentGASIntegration/Private/EquipmentGASIntegration.cpp
4.  Source/EquipmentGASIntegration/Public/EquipmentAbilityGranter.h
5.  Source/EquipmentGASIntegration/Private/EquipmentAbilityGranter.cpp
6.  Source/EquipmentGASIntegration/Public/EquipmentEffectApplier.h
7.  Source/EquipmentGASIntegration/Private/EquipmentEffectApplier.cpp
8.  Update: EquipmentManagerComponent to detect and use GAS module
```

**Compile gate**: Equip item → gameplay ability appears on ASC. Use ability. Unequip → ability removed. Passive stat effects apply/remove correctly.

### Phase 10: Storage System

**Goal**: Inventories persist across sessions.

**Files to create:**

```
1.  Source/ItemSystem/Public/Storage/ItemSystemSettings.h            (UDeveloperSettings)
2.  Source/ItemSystem/Private/Storage/ItemSystemSettings.cpp
3.  Source/ItemSystem/Public/Storage/ItemStorageSubsystem.h          (UGameInstanceSubsystem)
4.  Source/ItemSystem/Private/Storage/ItemStorageSubsystem.cpp       (Dirty check timer, save/load routing)
5.  Source/ItemSystem/Public/Storage/LocalItemStorage.h
6.  Source/ItemSystem/Private/Storage/LocalItemStorage.cpp           (JSON file I/O)
7.  Source/ItemSystem/Public/Storage/RemoteItemStorage.h
8.  Source/ItemSystem/Private/Storage/RemoteItemStorage.cpp          (HTTP client)
```

**Compile gate**: Play session → collect items → quit → restart → inventory restored. Storage backend swappable via project settings.

### Phase 11: Editor & Debug Tools

**Goal**: Development workflow is smooth.

**Files to create:**

```
1.  Source/ItemSystemEditor/Public/ItemDatabaseBrowser.h
2.  Source/ItemSystemEditor/Private/ItemDatabaseBrowser.cpp
3.  Console commands (in each runtime module):
    - `Inventory.Give <ItemId> <Count>` — add item to player inventory
    - `Inventory.Clear` — empty player inventory
    - `Inventory.List` — print inventory contents
    - `Loot.Roll <LootTableAsset> <Count>` — test loot generation
    - `Equipment.Equip <ItemId> <Slot>` — force equip
    - `Equipment.UnequipAll` — clear equipment
```

### Phase 12: Testing & Example Content

**Goal**: Confidence that everything works, reference for game developers.

```
Each plugin:
  Tests/
    Test_InventoryOperations.cpp     (unit tests for all operations)
    Test_InventoryReplication.cpp     (functional test with network simulation)
    Test_LootGeneration.cpp          (distribution validation)
    Test_Equipment.cpp               (equip/unequip/GAS)
    Test_Interaction.cpp             (detection, pickup, channeled)

Content/Examples/
    ExampleItems/                    (5-10 item definitions with various fragments)
    ExampleLootTables/               (2-3 loot tables demonstrating nesting, conditions)
    ExampleMap.umap                  (Test level with containers, world items, NPCs)
```

---

## 9. Working With Claude Code

### Task Decomposition Strategy

Each phase breaks down into Claude Code tasks. A good task is:

- **One file or one tightly-coupled pair** (header + implementation)
- **Has clear inputs** (what files already exist and what they provide)
- **Has a clear output** (the file compiles, or a specific behavior works)
- **References the right documentation** (point Claude Code to the relevant instructions.md)

### Example Task Sequence for Phase 4

```
Task 4.1: "Create Source/ItemSystem/Public/Types/InventoryOperationResult.h
           Define EInventoryOperationResult enum and FInventoryOperationData struct.
           Reference: Plugins/ItemInventoryPlugin/.claude/instructions.md, 
           section 'UInventoryComponent'."

Task 4.2: "Create Source/ItemSystem/Public/Components/InventoryComponent.h
           Full header with all public API, RPCs, validation, internal functions.
           All operations, queries, events as specified in instructions.md.
           Don't implement yet — header only."

Task 4.3: "Implement InventoryComponent.cpp — constructor, BeginPlay (slot 
           pre-allocation), GetLifetimeReplicatedProps. No operations yet."

Task 4.4: "Implement TryAddItem + ValidateAddItem + Internal_AddItem.
           Handle stacking: check existing stacks first, overflow to new slots.
           Include MarkDirty and delegate broadcasting."

Task 4.5: "Implement TryRemoveItem + ValidateRemoveItem + Internal_RemoveItem.
           Handle partial removal from stacks."

Task 4.6: "Implement TryMoveItem (atomic cross-inventory transfer), 
           TrySplitStack, TryMergeStacks, TrySwapSlots."

Task 4.7: "Implement all Query functions: HasItem, GetItemCount, 
           FindItemsByTag, GetItemInSlot, GetCurrentWeight, etc."

Task 4.8: "Write unit tests for all inventory operations."
```

### Context Management for Claude Code Sessions

At the start of each session, tell Claude Code:

```
"We're working on Phase [N] of the Game Framework Plugins.
Read CLAUDE.md and Plugins/[PluginName]/.claude/instructions.md.
Read the ARCHITECTURE.md section for Phase [N].
The previous phase is complete — here's what exists: [list key files].
Today's goal: [specific task]."
```

This ensures Claude Code has the right context without re-explaining the full architecture each time.

### Incremental Compilation Verification

After every 2-3 files, verify compilation:

```bash
# From project root
cd /path/to/UnrealProject
# Build just the module you're working on
UnrealBuildTool [ProjectName]Editor Development -Module=CommonGameFramework
```

Don't wait until the end of a phase to compile. Catch issues early.

---

## 10. Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| FFastArraySerializer complexity | Replication bugs, items disappearing | Study UE source examples (Lyra, ShooterGame). Write replication tests early (Phase 5). |
| Instance fragment UObject GC pressure | FPS drops with large inventories | Profile early. If needed, switch to FInstancedStruct (loses BP extensibility). |
| Cross-inventory atomicity in edge cases | Item duplication or loss | Comprehensive test matrix for move/split/merge across inventories. Server-side logging. |
| Asset loading stalls | Hitches when picking up new item types | All asset references are soft/async. Pre-warm common items at level load. |
| Loot table circular references | Infinite loop, stack overflow | Depth cap (5) with warning log. Editor validation in PostEditChangeProperty. |
| GAS module detection at runtime | Equipment works differently with/without GAS | Test both configurations explicitly. Feature toggle in project settings. |
