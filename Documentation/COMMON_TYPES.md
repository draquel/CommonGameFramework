# CommonGameFramework — Shared Types Reference

> State as of 2026-11-02 (code review CR5).

## Overview

The reference for every type, interface, delegate and gameplay tag CommonGameFramework defines. The other framework
plugins and the game layer build on them, so a change here ripples across the suite: change conservatively. Headers are
under `Source/CommonGameFramework/Public/` (`Types/`, `Interfaces/`, `Tags/`, `Utilities/`, `Testing/`).

---

## Core Structs

### FItemInstance

`Types/CGFItemTypes.h`. The runtime representation of one item or one stack.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FItemInstance
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FGuid InstanceId;                    // Globally unique, server-generated

    UPROPERTY(BlueprintReadOnly)
    FPrimaryAssetId ItemDefinitionId;    // The UItemDefinition asset (ItemInventoryPlugin)

    UPROPERTY(BlueprintReadOnly)
    int32 StackCount = 1;                // >= 1 for valid instances

    UPROPERTY(BlueprintReadOnly, Instanced)
    TArray<TObjectPtr<UItemInstanceFragment>> InstanceFragments;  // Mutable per-instance data

    bool IsValid() const;                // a GUID, a definition id and StackCount >= 1
    bool operator==(const FItemInstance& Other) const;   // equal when InstanceId matches
    bool operator!=(const FItemInstance& Other) const;
    bool Identical(const FItemInstance* Other, uint32 PortFlags) const;  // id, definition, count, fragment objects
                                                                         // and the fragment revision
    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

    template<typename T> const T* FindFragment() const;  // first instance fragment of type T, to read
    template<typename T> T* EditFragment();              // the same, to change in place: bumps the fragment revision

private:
    uint32 FragmentRevision = 0;         // a count of fragment edits; compared by Identical, never sent or saved
};
```

**Lifecycle rules:**
- Created only on the server, through `UItemDatabaseSubsystem::CreateItemInstance()` (ItemInventoryPlugin)
- Identified across the network by `InstanceId`, never by array index
- `StackCount = 0` means the instance should be removed; nothing may hold it after that
- Resolve `ItemDefinitionId` through `UItemDatabaseSubsystem`; never load the asset directly
- `InstanceFragments` are owned, instanced UObjects: the one place UObjects carry per-item data

**Serialization:**
- Network: custom `NetSerialize` (`TStructOpsTypeTraits`: `WithNetSerializer`, `WithIdentical`)
- Two comparisons: `operator==` is identity (the same `InstanceId`), what game code finds an instance by; `Identical`
  compares what `NetSerialize` sends (the id, the definition, `StackCount`, the fragment objects) and the fragment
  revision, and is what replication compares a property with its last sent state through, so a count changed in place
  reaches the clients (G135) and so do a fragment's contents changed in place (G136)
- The fragments are compared as objects, never read: replication's copy of the last sent state shares the live item's
  fragment objects (a copy of an item is shallow) without keeping them alive, so nothing could see their contents change.
  The revision is a plain number in the item, copied into that state by value: `EditFragment<T>()` bumps it, and
  `FindFragment<T>()` returns a const fragment, so a change in place goes through `EditFragment` or does not compile
- Change a fragment on the item where it is held: an inventory slot's own item, an equipment slot's. A copy of an item
  shares its fragment objects with the original, so a change through a copy lands on both and marks only the copy; write
  the copy back (`UInventoryComponent::TryReplaceItem` with its own id), or the change never replicates. A held item in
  a fast array also needs its slot marked dirty, which the owning component's operations do
- Storage: ItemInventoryPlugin's `FItemSerializationUtils` writes the JSON; each fragment writes its own data through
  `SerializeFragment`. The revision is neither sent nor saved: a client's copy gets new fragment objects at every
  receive, and a loaded item starts at 0

### UItemInstanceFragment (Base Class)

```cpp
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class COMMONGAMEFRAMEWORK_API UItemInstanceFragment : public UObject
{
    GENERATED_BODY()
public:
    virtual TSharedPtr<FJsonObject> SerializeFragment() const;           // persistence
    virtual void DeserializeFragment(const TSharedPtr<FJsonObject>& Data);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Item|Fragment")
    FText GetFragmentDisplayText() const;                                 // UI text

    virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
    virtual void OnAttachedToInstance(const FItemInstance& OwningInstance);
};
```

**Rules:**
- `EditInlineNew` + `DefaultToInstanced`, so they are edited inline on item definitions and created as instanced subobjects
- Subclasses declare their own `UPROPERTY` fields for mutable state and implement `SerializeFragment` /
  `DeserializeFragment` for persistence and `NetSerialize` for replication
- Those fields are `EditAnywhere` (a definition's defaults) and `BlueprintReadOnly`: a held item's fragment changes in C++
  through `FItemInstance::EditFragment`, which a Blueprint writing the field would bypass (G136)
- The base lives here (not in ItemInventoryPlugin) so that every plugin can carry `FItemInstance`; the subclasses
  (`UInstanceFragment_DurabilityState`, `UInstanceFragment_CustomDataMap`) live in ItemInventoryPlugin

### FInventorySlot and FInventorySlotArray

`Types/CGFItemTypes.h`. One inventory slot, and the `FFastArraySerializer` that replicates the slot array.

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
    FGameplayTagContainer SlotTags;    // carried; no code reads it today

    UPROPERTY(BlueprintReadOnly)
    bool bIsOccupied = false;

    void PreReplicatedRemove(const FInventorySlotArray& ArraySerializer);
    void PostReplicatedAdd(const FInventorySlotArray& ArraySerializer);
    void PostReplicatedChange(const FInventorySlotArray& ArraySerializer);
};

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInventorySlotArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FInventorySlot> Items;

    UActorComponent* OwningComponent = nullptr;     // the UInventoryComponent (not nameable here)

    // Bound by the owning component so its delegates fire on clients
    TFunction<void(const FInventorySlot&)> OnSlotAddedCallback;
    TFunction<void(const FInventorySlot&)> OnSlotRemovedCallback;
    TFunction<void(const FInventorySlot&)> OnSlotChangedCallback;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);  // WithNetDeltaSerializer
};
```

**Rules:**
- `SlotIndex` is stable: it never changes after the component allocates its slots
- Removing an item sets `bIsOccupied = false` and clears `Item`; the slot stays in the array

### FInteractionOption

`Types/CGFInteractionTypes.h`.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FInteractionOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag InteractionType;      // Interaction.Type.Pickup, etc.

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayText;                 // "Pick Up", "Open", "Search"

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Priority = 0;                // Higher = shown first

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bRequiresHold = false;        // True = channeled interaction

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bRequiresHold"))
    float HoldDuration = 0.0f;         // Seconds to hold
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
    FVector InteractionLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float Distance = 0.0f;                       // Between the two actors
};
```

### FEquipmentSlotDefinition

`Types/CGFEquipmentTypes.h`. Configuration of one equipment slot, set on `UEquipmentManagerComponent::AvailableSlots`.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FEquipmentSlotDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag SlotTag;                        // Equipment.Slot.Head, etc.

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTagContainer AcceptedItemTags;      // The item needs any of these (empty = any)

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText SlotDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AttachSocket;                          // Skeletal mesh socket for the visual

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FTransform AttachOffset = FTransform::Identity;  // The slot's skeleton correction for the visual
};
```

### FLootContext and FLootResult

`Types/CGFLootTypes.h`.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FLootContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 1;                   // Against an entry's MinLevel / MaxLevel

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ContextTags; // Loot.Source.*, biome, difficulty

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PartySize = 1;               // carried; the loot subsystem does not read it

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LuckModifier = 1.0f;         // carried; the loot subsystem does not read it

    UPROPERTY(BlueprintReadOnly)
    FRandomStream RandomStream;        // Used when its initial seed is non-zero (deterministic rolls)
};

USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FLootResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FItemInstance> Items;

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;
};
```

### FCGFDamageContext

`Types/CGFCombatTypes.h`. Built by whatever causes a hit (ability, trap, projectile, debug command) and handed to the
target's damageable. It carries no logic: the receiving plugin maps it onto its own attribute and mitigation model.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FCGFDamageContext
{
    GENERATED_BODY()

    TWeakObjectPtr<AActor> InstigatorActor;  // Responsible actor (pawn); null for hazards
    TWeakObjectPtr<AActor> CauserActor;      // Weapon / trap / projectile; may equal the instigator
    FGameplayTag DamageType;                 // Damage.Type.*; empty = Physical
    float BaseDamage = 0.f;                  // Before receiver-side modifiers
    FVector HitLocation, HitNormal;          // Impact, if known
    FGuid SourceItemInstanceId;              // The FItemInstance behind the hit, if any
    bool bIgnoreFaction = false;             // Hazards: skip the hostility check
    FGameplayTagContainer ContextTags;       // Free-form (Damage.Critical, ...)

    bool HasDamage() const;                  // BaseDamage > 0 and finite
};
```

### FCGFAttributeModifier

`Types/CGFCombatTypes.h`. One additive attribute change ("+3 Defense"), authored on item fragments (equipment stats,
consumable effects) and applied by `UCGFGameplayEffectStatics` without a gameplay-effect asset.

```cpp
USTRUCT(BlueprintType)
struct COMMONGAMEFRAMEWORK_API FCGFAttributeModifier
{
    GENERATED_BODY()
    FGameplayAttribute Attribute;   // Picked from any registered attribute set
    float Magnitude = 0.f;          // Added (negative subtracts)
    bool IsValid() const;
};
```

---

## Enums

`Types/CGFCommonEnums.h` and, for damage, `Types/CGFCombatTypes.h`. Rarity is the `Item.Rarity.*` gameplay tag alone
(the `EItemRarity` enum was named by nothing and was deleted in CR1, FW-11).

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

### ECGFDamageResult
```
Applied, Rejected_NoTarget, Rejected_NotAuthority, Rejected_Dead, Rejected_Invulnerable,
Rejected_Friendly, Rejected_NoAbilitySystem, Rejected_ByTarget
```
The outcome of a damage request; each rejection has its own value so callers can log or react without re-deriving it.

---

## Interfaces

All eight are `UINTERFACE(MinimalAPI, BlueprintType)` with `BlueprintNativeEvent` functions (call them through
`Execute_*`). Who implements and calls each is in `ARCHITECTURE.md` §4.1.

### ICGFInteractableInterface

```cpp
bool CanInteract(AActor* Interactor, const FInteractionContext& Context) const;
TArray<FInteractionOption> GetInteractionOptions(AActor* Interactor) const;
EInteractionResult Interact(AActor* Interactor, FGameplayTag InteractionType);
```

Declared for interactables; nothing in the project implements it. InteractionPlugin's flow finds the target's
`UInteractableComponent` (which has the same three functions) with `FindComponentByClass`.

### ICGFInventoryInterface

```cpp
UActorComponent* GetInventoryComponent() const;             // the UInventoryComponent
TArray<UActorComponent*> GetInventoryComponents() const;    // actors with several inventories
```

Returns `UActorComponent*` because `UInventoryComponent` lives in ItemInventoryPlugin; callers cast. Implemented by the
player character (`AVCCharacterBase`).

### ICGFEquippableInterface

```cpp
FGameplayTag GetEquipmentSlot() const;
TArray<TSubclassOf<UGameplayAbility>> GetGrantedAbilities() const;
TArray<TSubclassOf<UGameplayEffect>> GetAppliedEffects() const;
```

Declared; nothing implements it. EquipmentPlugin reads `UItemFragment_Equipment` from the item definition instead.

### ICGFItemStorageInterface

Persistence abstraction. Implemented by `ULocalItemStorage` and `URemoteItemStorage` (ItemInventoryPlugin).

```cpp
bool SaveInventory(const FString& OwnerId, const TArray<FItemInstance>& Items);
TArray<FItemInstance> LoadInventory(const FString& OwnerId);
bool DeleteInventory(const FString& OwnerId);

// Opaque JSON documents next to the inventories (the world save, VoxelWorldSave)
bool SaveBlob(const FString& Key, const FString& Json);
bool LoadBlob(const FString& Key, FString& OutJson);
bool DeleteBlob(const FString& Key);
bool HasBlob(const FString& Key);

// Asynchronous (G93): return at once, report on the game thread through the callback
void SaveInventoryAsync(const FString& OwnerId, const TArray<FItemInstance>& Items, const FOnStorageComplete& Callback);
void LoadInventoryAsync(const FString& OwnerId, const FOnStorageLoadComplete& Callback);
void DeleteInventoryAsync(const FString& OwnerId, const FOnStorageComplete& Callback);
```

Generated defaults of the blob functions do nothing and return false. The asynchronous calls default to the synchronous
ones with the callback run at once, so a backend that answers at once implements only the synchronous calls; a backend
that waits on a network implements the asynchronous ones (`URemoteItemStorage`). The ordering rule every backend keeps:
for one `OwnerId`, operations land in the order requested, and of overlapping saves the last one requested is the one
that lands (a superseded save's callback reports the save that superseded it). A load's `bSuccess` false means the read
failed (keep what you have); true with no items means nothing is stored.

### ICGFInteractionSourceInterface

```cpp
UActorComponent* GetInteractionComponent() const;
```

Declared for actors that perform interactions; nothing implements it.

### ICGFDamageableInterface

Implemented by anything that can be damaged and belongs to a faction, usually a combat component on a pawn
(VoxelCharacterPlugin's `UVCCombatComponent`). Find it with `UCGFCombatStatics::FindDamageable`. Damage application is
not part of the contract: it lives with the implementer's attribute model.

```cpp
FGameplayTag GetFactionTag() const;                          // Faction.*; empty = not a combatant
bool IsDead() const;
bool IsImmuneToDamage(const FCGFDamageContext& Context) const;   // Per-hit veto; default false
```

Generated defaults are empty faction, not dead, not immune.

### ICGFLightBearerInterface

```cpp
float GetCarriedLightLevel() const;          // 0 = dark, about 1 = a torch; every machine
bool ConsumeCarriedLightFuel(float Seconds); // authority
```

Implemented by actors that can carry a light (the player character, through its equipment manager's
`UItemFragment_LightSource` item, whose durability is the fuel). Read by dungeon sconces that need a flame and by NPC AI.
Generated defaults are dark. `Item.Category.Light` marks light-source items.

### ICGFRestPointInterface

```cpp
bool Rest(AActor* User);                     // authority; false when refused
bool SleepUntilMorning(AActor* User);        // authority; false when refused
bool CanSleepNow() const;
FGameplayTag GetCraftingStationTag() const;  // a Crafting.Station.* tag; empty = no crafting
virtual FSimpleMulticastDelegate* GetSleepAvailabilityChangedEvent();   // C++ only, may be null
```

Implemented by VoxelWorldPOI's campsites. The character's campsite UI calls it through server RPCs; the rest point
decides what resting means. `Crafting.Station.Campfire` is the first station; recipes with no station craft by hand.

---

## Gameplay Tags

Declared in `Tags/CGFGameplayTags.h` (`UE_DECLARE_GAMEPLAY_TAG_EXTERN`, namespace `CGFGameplayTags`) and defined in
`Private/Tags/CGFGameplayTags.cpp` (`UE_DEFINE_GAMEPLAY_TAG_COMMENT`).

### Item Tags
```
Item.Category.Weapon          — Weapons (swords, bows, staves)
Item.Category.Armor           — Armor pieces
Item.Category.Consumable      — Potions, food, scrolls
Item.Category.Material        — Crafting materials, resources
Item.Category.Quest           — Quest items
Item.Category.Misc            — Everything else
Item.Category.Light           — Carried light sources (torches)

Item.Rarity.Common / Uncommon / Rare / Epic / Legendary

Item.Key.DungeonBoss          — Key identity: opens a dungeon boss-room door (keys are items with a Key fragment)
```

### Inventory Tags
```
Inventory.Type.Player / Container / NPC / Loot   — carried on UInventoryComponent::InventoryTypeTag; not read by code
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

An equipment fragment naming the parent Equipment.Slot.Accessory goes to the first empty child slot.
```

### Interaction Tags
```
Interaction.Type.Pickup       — Pick up an item
Interaction.Type.Drop         — Drop an item
Interaction.Type.Use          — Use / activate (context-dependent)
Interaction.Type.Open         — Open a container, door, etc.
Interaction.Type.Equip        — Equip an item directly from the world
Interaction.Type.Inspect      — Examine without taking
Interaction.Type.Search       — Search a container for loot
Interaction.Type.Unlock       — Unlock a lock with a key
```

### Crafting Tags
```
Crafting.Station.Campfire     — The campsite's station; UCraftingRecipe::RequiredStationTag
```

### Loot Tags
```
Loot.Source.DungeonTreasure   — Roll context: a dungeon treasure-room chest
Loot.Source.DungeonContainer  — Roll context: a searchable dungeon crate / barrel
Loot.Source.DungeonBoss       — Roll context: a dungeon boss's death reward
Loot.Source.DungeonEnemy      — Roll context: a regular dungeon enemy's drop
Loot.Source.SurfaceNPC        — Roll context: surface wildlife (deer, wolves)
```

### Combat Tags
```
Faction.Player                — Player characters
Faction.Monster               — Hostile creatures and dungeon enemies
Faction.Neutral               — Never hostile to anyone
Faction.Object                — Breakable props: a valid target for anyone, never an attacker
Faction.Animal                — Wildlife: hunted by Player and Monster; animals never fight each other

Damage.Type.Physical          — Mitigated by defense
Damage.Type.Fire
Damage.Type.Poison
Damage.Type.Pure              — Unmitigated
Damage.Critical               — Context tag on a hit that rolled a critical

State.Dead                    — Rejects damage and ability activation
State.Downed                  — Knocked out but recoverable
State.Invulnerable            — Takes no damage while held

Event.Combat.Damaged          — Gameplay event sent to the target ASC after a hit
Event.Combat.Downed
Event.Combat.Died
Event.Objective.Completed     — Sent to the completing player's ASC when an objective is cleared

Objective.Kind.BossKill       — Objective kind: defeat the room's boss (dungeon objectives)

Ability.Attack.Melee          — Ability identity for TryActivateAbilitiesByTag
SetByCaller.Damage            — Magnitude key on damage effect specs
SetByCaller.Stat              — Root of SetByCaller.Stat.<AttributeName> keys on stat-modifier effects
```

### Extending Tags

Plugins and game code add child tags under any of these roots, for example:

```
Item.Category.Weapon.Melee.Sword    (game-specific)
Interaction.Type.Hack               (game-specific)
Equipment.Slot.Mount                (game-specific)
```

A new root namespace belongs in CommonGameFramework only when several plugins share the concept.

---

## Utility Functions

### UCGFStatics

`Utilities/CGFStatics.h`, a Blueprint function library.

```cpp
static bool IsItemInstanceValid(const FItemInstance& Instance);
static FString GetItemInstanceDebugString(const FItemInstance& Instance);
static bool ItemMatchesTagFilter(const FGameplayTagContainer& ItemTags,
    const FGameplayTagContainer& AllowedTags, const FGameplayTagContainer& DeniedTags);
static FGuid GenerateNewGuid();
static bool IsGuidValid(const FGuid& Guid);
```

### UCGFCombatStatics

`Utilities/CGFCombatStatics.h`. Pure helpers over the combat contracts; no state, no damage application.

```cpp
// Actor itself first, then its components (C++ or Blueprint implementers)
static TScriptInterface<ICGFDamageableInterface> FindDamageable(AActor* Actor);
static FGameplayTag GetFactionTag(AActor* Actor);
static bool IsActorDead(AActor* Actor);

// The hostility rule, in one place so a relationship table can replace it
static bool AreHostileFactions(FGameplayTag FactionA, FGameplayTag FactionB);
static bool AreHostile(AActor* ActorA, AActor* ActorB);
```

Blueprint implementers leave the native interface pointer inside `TScriptInterface` null, so C++ callers go through
`ICGFDamageableInterface::Execute_*` on `GetObject()`.

### UCGFGameplayEffectStatics

`Utilities/CGFGameplayEffectStatics.h`. Applies `FCGFAttributeModifier` lists without effect assets. Server only; callers
check authority.

```cpp
// Build a modifier from an attribute set class + property name (scripts, data tools)
static FCGFAttributeModifier MakeAttributeModifier(TSubclassOf<UAttributeSet> AttributeSet, FName AttributeName,
    float Magnitude, bool& bOutValid);
static FGameplayTag MakeStatSetByCallerTag(const FGameplayAttribute& Attribute);   // SetByCaller.Stat.<Name>

// Instant: a transient instant effect (never kept as an active effect, so safe to replicate)
static bool ApplyInstantAttributeModifiers(UAbilitySystemComponent* ASC, const TArray<FCGFAttributeModifier>& Modifiers,
    UObject* SourceObject);

// Lasting: a source-defined class whose modifiers are SetByCaller keyed SetByCaller.Stat.<Name>;
// unrequested stats are set to 0. (A transient infinite effect would not replicate to the owning client.)
static FActiveGameplayEffectHandle ApplyStatModifierEffect(UAbilitySystemComponent* ASC,
    TSubclassOf<UGameplayEffect> EffectClass, const TArray<FCGFAttributeModifier>& Modifiers, UObject* SourceObject);
static TArray<FGameplayAttribute> GetStatEffectAttributes(TSubclassOf<UGameplayEffect> EffectClass);
```

### CGFTesting::FTestGameInstance

`Testing/CGFTestGameInstance.h`, compiled only with `WITH_AUTOMATION_TESTS`: a standalone game instance with a game world
for the framework plugins' automation tests, so game-instance subsystems (the item database) and world subsystems (loot
generation) exist. Play has not begun: a component's `BeginPlay` does not run.

---

## Delegate Signatures

Inventory, equipment and storage delegates are declared in `Types/CGFItemTypes.h`; interaction delegates in
`Types/CGFInteractionTypes.h`.

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

// Storage callbacks
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnStorageComplete, const FString&, OwnerId, bool, bSuccess);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnStorageLoadComplete, const FString&, OwnerId, bool, bSuccess, const TArray<FItemInstance>&, Items);

// Interaction events (CGFInteractionTypes.h)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFound, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableLost, AActor*, InteractableActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionStarted, const FInteractionContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionCompleted, const FInteractionContext&, Context, EInteractionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionFailed, const FInteractionContext&, Context, EInteractionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChanneledInteractionProgress, float, Progress);
```
