# Game Framework Architecture

> State as of 2026-11-02 (code review CR5).

## Document Purpose

How the four game framework plugins (CommonGameFramework, ItemInventoryPlugin, InteractionPlugin, EquipmentPlugin)
fit together: the layers and module dependencies, the item data lifecycle, the replication model, the cross-plugin
contracts, persistence and the extension points. Each plugin's own `Documentation/` folder holds its system's detail;
`COMMON_TYPES.md` (next to this file) is the reference for the shared types, interfaces and tags.

Everything the February 2026 build plan listed is built: the shared types and the eight `ICGF*` interfaces, item
definitions and fragments, the replicated inventory, loot tables, interaction with world items and their pool,
equipment with visuals and the GAS module, local and remote storage, the Item Database Browser, the console commands
and automation tests in every plugin. The plan's phase lists and its "working with Claude Code" section were cut in CR5
(git keeps them).

---

## 1. System Architecture Overview

### 1.1 Layered Architecture

Higher layers depend on lower layers, never the reverse.

```
┌──────────────────────────────────────────────────────────────┐
│  GAME LAYER (not part of these plugins)                      │
│  VoxelCharacterPlugin, VoxelWorldPOI, the game module:       │
│  character, controller, UI wiring, containers, campsites     │
├──────────────────────────────────────────────────────────────┤
│  PLUGIN LAYER                                                │
│  ItemInventoryPlugin ◄── InteractionPlugin                   │
│        ▲                                                     │
│        └──────────────── EquipmentPlugin (+ GAS module)      │
├──────────────────────────────────────────────────────────────┤
│  FOUNDATION LAYER                                            │
│  CommonGameFramework: shared types, interfaces, tags         │
├──────────────────────────────────────────────────────────────┤
│  ENGINE LAYER                                                │
│  Unreal Engine 5.8: GAS, NetCore, AssetManager, UMG          │
└──────────────────────────────────────────────────────────────┘
```

### 1.2 Module Dependency Map (Compile-Time)

| Module | Plugin | Depends on (framework modules) |
|--------|--------|-------------------------------|
| `CommonGameFramework` | CommonGameFramework | none (engine: Core, CoreUObject, Engine, GameplayTags, GameplayAbilities, NetCore) |
| `ItemInventoryPlugin` | ItemInventoryPlugin | `CommonGameFramework` (public) |
| `ItemSystemEditor` (editor) | ItemInventoryPlugin | `ItemInventoryPlugin`, `CommonGameFramework` |
| `InteractionPlugin` | InteractionPlugin | `CommonGameFramework` (public), `ItemInventoryPlugin` (private) |
| `EquipmentPlugin` | EquipmentPlugin | `CommonGameFramework` (public), `ItemInventoryPlugin` (private) |
| `EquipmentGASIntegration` | EquipmentPlugin | `CommonGameFramework` (public), `EquipmentPlugin`, `ItemInventoryPlugin` (private) |

The two dependencies on ItemInventoryPlugin are downward, not circular, and both are required: each `.uplugin` lists
ItemInventoryPlugin as a plugin it needs (code review decision D-CR22, FW-04/05).

- InteractionPlugin uses four of its classes in `AWorldItem`: `UItemDefinition`, `UItemFragment_WorldDisplay`,
  `UItemDatabaseSubsystem` and `UInventoryComponent`.
- EquipmentPlugin reads the equipment, durability and light-source fragments, resolves definitions through the item
  database, equips from and unequips to a `UInventoryComponent`, and its slot widget shows ItemInventoryPlugin's
  `UItemTooltipWidget`. It does not compile or run without ItemInventoryPlugin.

Nothing depends on InteractionPlugin or EquipmentPlugin, and those two never reference each other. CommonGameFramework
references no other framework plugin.

### 1.3 Runtime Object Ownership

```
Player character (game layer, VoxelCharacterPlugin's AVCCharacterBase)
├── UInventoryComponent              ← owns item instances, replicated
├── UEquipmentManagerComponent       ← equipped items, replicated
├── UInteractionComponent            ← detects and requests interactions
└── UAbilitySystemComponent (GAS)    ← receives abilities / effects from equipment (on the player state)

AWorldItem (InteractionPlugin)
├── UStaticMeshComponent             ← root; the world mesh or a fallback cube
├── UInteractableComponent           ← the pickup option
└── FItemInstance (replicated data)  ← the item this actor represents

A container (game layer, e.g. VoxelWorldPOI's APOILootChest)
├── UInventoryComponent or a loot table rolled on open
└── UInteractableComponent           ← "Open" / "Search"

UGameInstance
├── UItemDatabaseSubsystem           ← item definitions by FPrimaryAssetId, CreateItemInstance
├── UCraftingSubsystem               ← recipes, CanCraft / Craft
└── UItemStorageSubsystem            ← registered inventories, auto-save, the storage backend

UWorld
├── ULootGenerationSubsystem         ← rolls loot tables into FItemInstances
└── UWorldItemPoolSubsystem          ← pool of AWorldItem actors
```

---

## 2. Data Architecture

### 2.1 Item Data Lifecycle

```
DEFINITION (design time)          INSTANCE (runtime)             ACTOR (world)
┌─────────────────────┐     ┌─────────────────────┐     ┌─────────────────────┐
│ UItemDefinition     │     │ FItemInstance        │     │ AWorldItem          │
│ (UPrimaryDataAsset) │     │ (struct)             │     │ (actor, pooled)     │
│ DisplayName, Icon   │────▶│ InstanceId (GUID)    │────▶│ MeshComponent       │
│ ItemTags            │     │ ItemDefinitionId     │     │ InteractableComp    │
│ MaxStackSize        │     │ StackCount           │     │ FItemInstance copy  │
│ Weight, BaseValue   │     │ InstanceFragments    │     │                      │
│ Fragments (static)  │     │   (mutable state)    │     │                      │
│ DefaultInstance-    │     │ Lives in inventory   │     │ Lives in the world  │
│   Fragments         │     │ slots, equipment     │     │ or the pool         │
│                     │     │ slots, loot results, │     │                      │
│                     │     │ storage payloads     │     │                      │
└─────────────────────┘     └─────────────────────┘     └─────────────────────┘
  One per item type           Many per item type           One per visible drop
```

**Transitions between forms:**

```
Definition → Instance:   UItemDatabaseSubsystem::CreateItemInstance(AssetId, Count)
                         new GUID, copies DefaultInstanceFragments; server only

Instance → Actor:        UWorldItemPoolSubsystem::SpawnWorldItem(Item, Location, Rotation)
                         reuses a pooled AWorldItem, calls InitializeFromItem

Actor → Instance:        the pickup handler adds the FItemInstance to the interactor's
                         inventory and returns the actor to the pool

Instance → Storage:      ICGFItemStorageInterface::SaveInventory(OwnerId, Items)
Storage → Instance:      ICGFItemStorageInterface::LoadInventory(OwnerId)
```

### 2.2 Fragment Architecture

Fragments follow a two-layer pattern after Lyra's item model:

```
UItemDefinition
├── Fragments (UItemDefinitionFragment)                 ← static, shared
│   ├── UItemFragment_Weapon       { BaseDamage, AttackSpeed, DamageType, ... }
│   ├── UItemFragment_Equipment    { EquipmentSlotTag, EquipMesh, GrantedAbilities, StatModifiers, ... }
│   └── UItemFragment_WorldDisplay { WorldMesh, WorldMaterial, WorldRotation, ... }
│
└── DefaultInstanceFragments (UItemInstanceFragment)    ← template copied into new instances
    └── UInstanceFragment_DurabilityState { CurrentDurability = 100 }

FItemInstance (created from the definition above)
└── InstanceFragments                                   ← mutable, per instance
    └── UInstanceFragment_DurabilityState { CurrentDurability = 73 }
```

Lookup at runtime:

```cpp
// Static data (weapon damage): from the definition
UItemDefinition* Def = ItemDatabase->GetDefinition(Instance.ItemDefinitionId);
const UItemFragment_Weapon* Weapon = Def ? Def->FindFragment<UItemFragment_Weapon>() : nullptr;

// Instance data (current durability): from the instance
const UInstanceFragment_DurabilityState* Durability = Instance.FindFragment<UInstanceFragment_DurabilityState>();
```

Never cache instance fragment pointers across frames: the instance can be moved, split or destroyed. Definition
fragments are safe to cache; definitions are immutable at runtime.

### 2.3 Inventory Slot Model

```
UInventoryComponent
└── InventorySlots: FInventorySlotArray (FFastArraySerializer, CommonGameFramework)
    ├── FInventorySlot [0]  { Item, SlotIndex=0, bIsOccupied=true  }
    ├── FInventorySlot [1]  { Item, SlotIndex=1, bIsOccupied=false }
    └── ... MaxSlots entries
```

Slots are pre-allocated in `BeginPlay` from `MaxSlots`. Adding fills an empty slot or tops up a stack; removing clears a
slot, it never removes the array element. Slot indices are stable, which the UI and network references rely on. The
hotbar is not a separate store: `UHotbarWidget` shows the first slots of the player's inventory (nine by default).

`FInventorySlot::SlotTags` and `UInventoryComponent::InventoryTypeTag` are carried but read by no code; filtering is by
`AllowedItemTags` / `DeniedItemTags` on the component.

`FInventorySlotArray` lives in CommonGameFramework, which cannot name `UInventoryComponent`: it holds the owner as a
plain `UActorComponent*` and three `TFunction` callbacks (`OnSlotAddedCallback`, `OnSlotRemovedCallback`,
`OnSlotChangedCallback`) that the component binds to raise its delegates on clients.

---

## 3. Multiplayer Architecture

### 3.1 Authority Model

The server owns every `FItemInstance`, generates every GUID, and runs every inventory, equipment, loot and interaction
mutation. State reaches clients by replication:

- inventory slots: `FFastArraySerializer` (`FInventorySlotArray`), per-element deltas;
- equipment slots: a replicated `TArray<FEquipmentSlot>` with `OnRep_EquipmentSlots`;
- world items: the replicated `AWorldItem::ItemInstance`;
- interactables: the replicated `UInteractableComponent::bIsEnabled`.

Clients call the public `Try*` functions; on a client these send a reliable Server RPC and return `Pending` (inventory),
`Success` (equipment, optimistic) or `InProgress` (interaction). A rejected request comes back through a Client RPC
(`ClientRPC_OperationFailed`, `ClientRPC_EquipmentOperationFailed`, `ClientRPC_InteractionResult`) that fires the
component's failure delegate.

No replication functional test (dedicated server plus client) exists yet; the decision on writing one is open (D-CR23,
FW-34).

### 3.2 Pickup Flow (Multiplayer)

```
CLIENT                                   SERVER
──────                                   ──────
1. UInteractionComponent's detection
   timer picks the AWorldItem as the
   best target (local player only)
2. Interact input → TryInteract(Type)
   → ValidateInteraction (range,
     enabled, CanInteract)
3. ServerRPC_RequestInteract ──────────► 4. Re-checks: target valid, within 1.1 x
   (Target, InteractionType)                InteractionRange, has an enabled
                                            UInteractableComponent, CanInteract
                                         5. UInteractableComponent::Interact →
                                            AWorldItem's handler: inventory via
                                            ICGFInventoryInterface, TryAddItem,
                                            the actor back to the pool
                                         6. ClientRPC_InteractionResult ─────────►
7. OnInteractionCompleted / Failed
8. Inventory delta arrives:
   PostReplicatedAdd → OnItemAdded → UI
```

### 3.3 Validation and Server RPCs

The rule (code review decision D-CR16, amended to what the code does): every Server RPC validates inline before it
mutates anything, and never trusts the client's earlier check; where the client predicts or pre-checks, the check is a
const predicate both sides call. The plugins today:

| Component | Shared predicate | Inline in the Server RPC |
|-----------|------------------|--------------------------|
| `UInventoryComponent` | `ValidateAddItem` / `ValidateRemoveItem` (fill `FInventoryOperationData`), used by `TryAddItem`, `TryRemoveItem`, `TryMoveItem` (both inventories) and their RPCs | split, merge and swap repeat their checks in `TrySplitStack` / `TryMergeStacks` / `TrySwapSlots` and again in the RPC |
| `UEquipmentManagerComponent` | `ValidateEquip` (returns `EEquipmentResult`), `CanEquipItem` | the unequip RPCs check the slot and the target inventory's `CanAcceptItem` |
| `UInteractionComponent` | `ValidateInteraction` (client side, before the RPC) | `ServerRPC_RequestInteract` repeats range (with 10% tolerance), interactable and `CanInteract` |

```cpp
EInventoryOperationResult UInventoryComponent::TryAddItem(const FItemInstance& Item, int32 PreferredSlot)
{
    FInventoryOperationData OpData;
    if (!ValidateAddItem(Item, PreferredSlot, OpData))   // same check on client and server
    {
        return OpData.Result;
    }
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        ServerRPC_RequestAddItem(Item, PreferredSlot);    // re-validates, ClientRPC_OperationFailed on rejection
        return EInventoryOperationResult::Pending;
    }
    Internal_AddItemStacked(Item, PreferredSlot);         // stacks first, then empty slots
    return EInventoryOperationResult::Success;
}
```

### 3.4 Cross-Inventory Transfers

`TryMoveItem` checks both sides before touching either: the target's `ValidateAddItem` for a copy of the item, the
source's `ValidateRemoveItem`. Only then does the server remove the whole stack from the source and add the copy to the
target. Equipment's `TryEquipFromInventory` / `TryUnequipToInventory` follow the same order and re-equip the item if the
inventory add fails after validation.

---

## 4. Cross-Plugin Integration

### 4.1 Interfaces in CommonGameFramework

| Interface | Implemented by | Called by |
|-----------|----------------|-----------|
| `ICGFInventoryInterface` | the character (`AVCCharacterBase`), returning its `UInventoryComponent` | `AWorldItem` pickup, the dungeon boss door (VoxelWorldPOI) |
| `ICGFItemStorageInterface` | `ULocalItemStorage`, `URemoteItemStorage` | `UItemStorageSubsystem`, VoxelWorldSave (blobs) |
| `ICGFDamageableInterface` | VoxelCharacterPlugin's `UVCCombatComponent`, VoxelWorldPOI's breakable containers (`APOIDungeonContainer`) | `UCGFCombatStatics::FindDamageable` and the combat code |
| `ICGFLightBearerInterface` | the player character (`AVCCharacterBase`) | dungeon torches (VoxelWorldPOI), NPC AI (keeps away from a lit bearer) |
| `ICGFRestPointInterface` | VoxelWorldPOI's campsites | the character's campsite and crafting UI |
| `ICGFInteractableInterface`, `ICGFInteractionSourceInterface`, `ICGFEquippableInterface` | nothing in the project | nothing: the interaction flow finds `UInteractableComponent` with `FindComponentByClass`, equipment reads `UItemFragment_Equipment` |

### 4.2 Gameplay Tags

Tags are the loose-coupling mechanism; the shared roots are declared in CommonGameFramework (`COMMON_TYPES.md`).

```
Item.Category.*        item definitions; equipment slots filter on them (AcceptedItemTags)
Equipment.Slot.*       equipment slot identity; an equipment fragment names its slot
Interaction.Type.*     what an interaction option does (Pickup, Open, Search, Unlock, ...)
Loot.Source.*          context tags a loot roll carries (loot entries' Required / ExcludedContextTags)
Item.Key.*             key identity; locks match with UItemFragment_Key::Opens
Crafting.Station.*     a recipe's station; ICGFRestPointInterface::GetCraftingStationTag
Faction.* / Damage.* / State.* / Event.*    combat (VoxelCharacterPlugin)
```

A new interaction type, item category or equipment slot is a new tag; other plugins need no code change.

### 4.3 Events

Each plugin fires its own delegates: inventory `OnItemAdded` / `OnItemRemoved` / `OnItemMoved` / `OnItemUpdated` /
`OnInventoryChanged`, equipment `OnItemEquipped` / `OnItemUnequipped` / `OnEquipmentChanged` (plus durability, broken
item and carried light), interaction `OnInteractableFound` / `OnInteractableLost` / `OnInteractionCompleted` /
`OnInteractionFailed` / `OnChanneledProgress`. The game layer binds to what it needs. Plugins do not bind to each
other's delegates; EquipmentPlugin calls inventory functions directly when equipping from an inventory.

### 4.4 Integration Sequences

**Equip from inventory:**
```
UI (game layer) → UEquipmentManagerComponent::TryEquipFromInventory(InstanceId, Inventory, SlotTag)
  → finds the item in the inventory, picks a slot (FindTargetSlot) when none is given
  → ValidateEquip: valid item, an Equipment fragment, the slot exists, item tags match the slot
  → client: ServerRPC_RequestEquipFromInventory and return
  → server: slot occupied → the old item back to the inventory (NoInventorySpace when it does not fit)
  → Inventory->TryRemoveItem(InstanceId)
  → Internal_Equip: visuals (async mesh, socket), GAS (when the GAS module is loaded), OnItemEquipped, OnPostEquip
  → EquipmentSlots replicate; clients attach visuals in OnRep_EquipmentSlots
```

**Loot container:**
```
Game-layer container with a loot table and an UInteractableComponent ("Open")
  → on the server, ULootGenerationSubsystem::GenerateLoot(Table, Context) → TArray<FItemInstance>
  → the items go into the container's or the player's UInventoryComponent (TryAddItem), or burst as world items
    (UWorldItemPoolSubsystem::SpawnWorldItem)
```

---

## 5. Performance

**World item pooling.** `UWorldItemPoolSubsystem` pre-spawns `InitialPoolSize` (50) actors at world begin play,
`PreWarmBatchSize` (10) per timer tick, grows on demand up to `MaxPoolSize` (200), and returns an uncollected item to the
pool after `DespawnTimeout` (300 s, 0 = never). Placed items are ordinary actors; the pool is for runtime drops.

**Asset loading.** Definitions are referenced by `FPrimaryAssetId`; meshes and icons by `TSoftObjectPtr`. World item
meshes, equipment visuals and slot icons load asynchronously through the streamable manager. The item database loads
every definition synchronously once, when it initialises; a nested loot table loads synchronously when first rolled.

**Inventory replication.** `FFastArraySerializer` sends only the slots marked dirty, so one added item costs one slot's
delta, not the array.

**Detection.** Interaction detection is a timer (`DetectionTickRate`, 0.1 s), runs only while the owning pawn is
locally controlled, and never on the server.

---

## 6. Persistence

```
UInventoryComponent (StorageOwnerId set, bAutoRegisterStorage)
  │ registers in BeginPlay, unregisters in EndPlay (saving it when dirty)
  ▼
UItemStorageSubsystem (UGameInstanceSubsystem)
  │ auto-save timer: every AutoSaveIntervalSeconds (30 s) saves each dirty inventory
  │ Deinitialize: SaveAll
  │ ManualSave / ManualLoad on the component go through it
  ▼
ICGFItemStorageInterface backend, chosen by UItemSystemSettings::StorageBackend
(Project Settings > Plugins > Item System)
  ├── ULocalItemStorage   JSON at Saved/SaveGames/Inventories/<OwnerId>.json (write to .tmp, then rename)
  └── URemoteItemStorage  HTTP POST / GET / DELETE <RemoteEndpointURL>/<OwnerId>, bearer token, retries
```

Every mutation marks the component dirty; a save clears it. The same backend stores opaque JSON blobs
(`SaveBlob` / `LoadBlob` / `DeleteBlob` / `HasBlob`) that the world save (VoxelWorldSave) uses. The serialization format
and the backends' details are in `Plugins/ItemInventoryPlugin/Documentation/ITEM_INVENTORY_SYSTEM.md`.

---

## 7. Extension Points

| Extension point | Mechanism | Example |
|-----------------|-----------|---------|
| New definition data | Subclass `UItemDefinitionFragment` | an "Enchantable" fragment |
| New per-instance state | Subclass `UItemInstanceFragment` (implement `SerializeFragment` / `DeserializeFragment`) | soulbound flag, kill count |
| Custom interactable | Add a `UInteractableComponent`; bind its `InteractionHandler` and, for per-player text, `OptionsModifier` | NPC dialogue, a lever |
| Custom detection | Subclass `UInteractionDetectionStrategy` (`DetectInteractables`) | a cone for a stealth game |
| Custom target choice | Override `UInteractionComponent::ScoreCandidate` (BlueprintNativeEvent) | prefer quest objects |
| Custom equipment visual | Override `OnPostEquip` / `OnPostUnequip` (BlueprintNativeEvent) | modular mesh swaps |
| Custom storage backend | Implement `ICGFItemStorageInterface` | a cloud save provider |
| Loot conditions | Context tags on `FLootContext` against an entry's `RequiredContextTags` / `ExcludedContextTags`, levels | biome or source specific drops |
| Inventory filters | `AllowedItemTags` / `DeniedItemTags`, `MaxWeight` on the component | an ammo pouch |
| New interaction types | Add gameplay tags | `Interaction.Type.Hack` |

---

## 8. Risks

| Risk | Impact | Where it stands |
|------|--------|-----------------|
| `FFastArraySerializer` replication bugs | items disappear or duplicate on clients | no dedicated-server replication test yet (D-CR23 open) |
| Instance fragment UObject GC pressure | frame cost with large inventories | not profiled; `FInstancedStruct` is the fallback (loses Blueprint subclassing) |
| Cross-inventory atomicity | duplication or loss | validation before mutation; merge and cross-inventory move have tests (`ItemInventory.Operations.*`) |
| Loot table cycles | unbounded recursion | `ULootGenerationSubsystem::MaxRecursionDepth` 5 with a warning; no edit-time cycle check |
| GAS module absent | equipment without abilities or effects | by design: the core skips GAS when the module did not register its factory |
