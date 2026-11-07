# CommonGameFramework — Plugin Instructions

> State as of 2026-11-02 (code review CR5).

## Purpose

The foundation every game framework plugin depends on: shared types, the eight `ICGF*` interfaces, the native gameplay
tags, and static helpers. **This module has no dependency on any other game framework plugin**, and it holds no gameplay
logic.

## Reading Order

1. `Documentation/ARCHITECTURE.md` — how the four framework plugins fit together: layers and dependencies, the item
   lifecycle, replication and validation, cross-plugin contracts, persistence, extension points.
2. `Documentation/COMMON_TYPES.md` — the reference for every struct, enum, interface, delegate and tag defined here.
3. The consuming plugin's instructions and system document when a change here affects it.

## Module Structure

```
CommonGameFramework/
├── Source/CommonGameFramework/
│   ├── Public/
│   │   ├── Interfaces/
│   │   │   ├── CGFInteractableInterface.h       ← ICGFInteractableInterface (implemented by nothing today)
│   │   │   ├── CGFInventoryInterface.h          ← ICGFInventoryInterface: an actor's inventory component(s)
│   │   │   ├── CGFEquippableInterface.h         ← ICGFEquippableInterface (implemented by nothing today)
│   │   │   ├── CGFItemStorageInterface.h        ← ICGFItemStorageInterface: inventories and blobs persistence
│   │   │   ├── CGFInteractionSourceInterface.h  ← ICGFInteractionSourceInterface (implemented by nothing today)
│   │   │   ├── CGFDamageableInterface.h         ← ICGFDamageableInterface: faction, dead, per-hit veto
│   │   │   ├── CGFLightBearerInterface.h        ← ICGFLightBearerInterface: carried light level, fuel spend
│   │   │   └── CGFRestPointInterface.h          ← ICGFRestPointInterface: rest / sleep / crafting station
│   │   ├── Types/
│   │   │   ├── CGFItemTypes.h                   ← UItemInstanceFragment, FItemInstance, FInventorySlot,
│   │   │   │                                       FInventorySlotArray; inventory, equipment, storage delegates
│   │   │   ├── CGFInteractionTypes.h            ← FInteractionOption, FInteractionContext, interaction delegates
│   │   │   ├── CGFEquipmentTypes.h              ← FEquipmentSlotDefinition
│   │   │   ├── CGFLootTypes.h                   ← FLootContext, FLootResult
│   │   │   ├── CGFCombatTypes.h                 ← FCGFDamageContext, ECGFDamageResult, FCGFAttributeModifier
│   │   │   └── CGFCommonEnums.h                 ← EInventoryOperationResult, EInteractionResult, EEquipmentResult
│   │   ├── Tags/CGFGameplayTags.h               ← native tag declarations (namespace CGFGameplayTags)
│   │   ├── Utilities/
│   │   │   ├── CGFStatics.h                     ← item / tag / GUID helpers
│   │   │   ├── CGFCombatStatics.h               ← FindDamageable, the hostility rule
│   │   │   └── CGFGameplayEffectStatics.h       ← apply FCGFAttributeModifier lists (instant / SetByCaller.Stat.*)
│   │   ├── Testing/CGFTestGameInstance.h        ← CGFTesting::FTestGameInstance, FRPCCall (automation tests only)
│   │   └── CommonGameFramework.h                ← module header, log category LogCGF
│   └── Private/ (Tags, Types, Utilities, CommonGameFramework.cpp)
└── CommonGameFramework.uplugin                  ← requires GameplayAbilities
```

## Rules for This Module

1. **No gameplay logic.** Types and contracts only. Damage application, for instance, is not in
   `ICGFDamageableInterface`: it belongs to the plugin that owns the attribute model (VoxelCharacterPlugin).
2. **No dependencies on other game framework plugins.** A type both sides need moves here. Where a contract must name a
   downstream class it uses a base type instead (`ICGFInventoryInterface` returns `UActorComponent*`;
   `FInventorySlotArray` holds its owner as `UActorComponent*` and binds `TFunction` callbacks).
3. **Every type serializes** for both the network and storage (`NetSerialize`, and `SerializeFragment` /
   `DeserializeFragment` on instance fragments).
4. **Interface functions are `BlueprintNativeEvent`**, called through `Execute_*`.
5. **Changes here affect every plugin.** Add a type only when it is shared.
6. **`FItemInstance` rules:** GUIDs are made on the server only (`UItemDatabaseSubsystem::CreateItemInstance`);
   `InstanceId` is the key for every operation, never an array index; `StackCount` is at least 1 for a valid instance;
   instance fragments are the one place UObjects carry per-item data. A fragment changes through `EditFragment` on
   the item where it is held, which bumps the fragment revision `Identical` compares; `FindFragment` is for reading
   (it returns a const fragment). A copy of an item shares its fragment objects (G136, `COMMON_TYPES.md`).
7. **Definition fragments vs instance fragments:** the definition fragment base (`UItemDefinitionFragment`) lives in
   ItemInventoryPlugin, which owns item data assets; the instance fragment base (`UItemInstanceFragment`) lives here
   because every plugin carries `FItemInstance`.
8. **Tags:** shared roots are declared here (`COMMON_TYPES.md` lists them); plugins and games add children. New
   interaction kinds, slots and categories are new tags, never new enum values.
9. **The hostility rule** lives in `UCGFCombatStatics::AreHostileFactions` and nowhere else.
10. **Lasting stats go through a source-defined effect class** (`UCGFGameplayEffectStatics::ApplyStatModifierEffect`,
    `SetByCaller.Stat.<Name>`): a transient infinite effect does not replicate to the owning client.

## Build.cs Dependencies

```csharp
PublicDependencyModuleNames: Core, CoreUObject, Engine, GameplayTags,
                             GameplayAbilities (GAS types in interfaces and the effect statics), NetCore (FFastArraySerializer)
```

## Logging and Tests

- Log category `LogCGF` (`CommonGameFramework.h`).
- The module has no tests of its own; its types are exercised by ItemInventoryPlugin's `ItemInventory.Types.*` tests and
  the other plugins' suites. `CGFTesting::FTestGameInstance` gives those tests a game instance and world;
  `SpawnRemoteOwner` an actor as a client sees its own pawn (no authority), and `CGFTesting::FRPCCall` delivers an RPC
  by name with its parameters (a standalone world runs it locally), so a test plays both machines of a request.
