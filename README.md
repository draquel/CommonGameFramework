# CommonGameFramework

Shared foundation module for the Game Framework plugin suite. Provides core types, interfaces, gameplay tags, and utilities that all other plugins depend on.

## What This Plugin Does

CommonGameFramework defines the **contracts** that the rest of the system builds on. It contains no gameplay logic — only type definitions, interfaces, and shared constants. Think of it as the vocabulary that ItemInventoryPlugin, InteractionPlugin, and EquipmentPlugin use to communicate without knowing about each other.

**Core types:**
- `FItemInstance` — The runtime representation of an item (GUID, definition reference, stack count, instance fragments)
- `FInventorySlot` — A slot in an inventory backed by `FFastArraySerializer` for efficient replication
- `FInteractionOption` / `FInteractionContext` — Data structures for the interaction system
- `FEquipmentSlotDefinition` — Configuration for equipment slots
- `FLootContext` — Parameters for loot generation (level, tags, luck)
- `UItemInstanceFragment` — Base class for per-instance mutable data (durability, enchantments, etc.)

**Interfaces:**
- `IInteractable` — Implemented by anything a player can interact with
- `IInventoryOwner` — Implemented by actors that have inventories
- `IEquippable` — Queried on items to determine equipment behavior
- `IItemStorage` — Persistence abstraction (local files, remote database)
- `IInteractionSource` — Implemented by actors that perform interactions

**Gameplay tags:**
Native tag declarations for `Item.*`, `Inventory.*`, `Equipment.*`, `Interaction.*` hierarchies used across all plugins.

## Requirements

- Unreal Engine 5.7
- GameplayAbilities plugin (enabled in .uproject)
- No other Game Framework plugins required — this is the dependency root

## Installation

Clone into your project's `Plugins/` directory:

```bash
git clone <repo-url> Plugins/CommonGameFramework
```

Add `"CommonGameFramework"` to any module's `Build.cs` that needs the shared types:

```csharp
PublicDependencyModuleNames.Add("CommonGameFramework");
```

## Module Dependencies

```
CommonGameFramework
├── Core
├── CoreUObject
├── Engine
├── GameplayTags
├── GameplayAbilities  (for GAS types in interfaces)
├── GameplayTasks
└── NetCore            (for FFastArraySerializer base)
```

## Plugin Structure

```
CommonGameFramework/
├── Source/CommonGameFramework/
│   ├── Public/
│   │   ├── Types/           Core structs and enums
│   │   ├── Interfaces/      UInterface declarations
│   │   ├── Tags/            Native gameplay tag declarations
│   │   └── Utilities/       Static helper functions
│   └── Private/
├── Documentation/
│   ├── ARCHITECTURE.md      Master architecture for the entire plugin suite
│   └── COMMON_TYPES.md      Detailed reference for all shared types
└── .claude/
    └── instructions.md      Claude Code implementation instructions
```

## Usage

CommonGameFramework is not used directly by game code in most cases. It's a dependency of the other plugins. However, game code that needs to work with item instances, check inventory interfaces, or reference shared tags will include headers from this module.

```cpp
#include "Types/CGFItemTypes.h"
#include "Interfaces/CGFInventoryInterface.h"
#include "Tags/CGFGameplayTags.h"

// Check if an actor has an inventory
if (Actor->Implements<UCGFInventoryInterface>())
{
    UInventoryComponent* Inv = ICGFInventoryInterface::Execute_GetInventoryComponent(Actor);
}

// Work with item instances
if (ItemInstance.IsValid())
{
    FPrimaryAssetId DefId = ItemInstance.ItemDefinitionId;
    int32 Count = ItemInstance.StackCount;
}

// Use shared tags
if (ItemDef->ItemTags.HasTag(CGFGameplayTags::Item_Category_Weapon))
{
    // This is a weapon
}
```

## Related Plugins

| Plugin | Relationship |
|--------|-------------|
| [ItemInventoryPlugin](../ItemInventoryPlugin/) | Implements item definitions, inventories, loot, storage |
| [InteractionPlugin](../InteractionPlugin/) | Implements interaction detection, world items |
| [EquipmentPlugin](../EquipmentPlugin/) | Implements equipment slots, GAS integration |

## Documentation

- [ARCHITECTURE.md](Documentation/ARCHITECTURE.md) — Complete system architecture, data flow, replication model, phased build plan
- [COMMON_TYPES.md](Documentation/COMMON_TYPES.md) — Detailed reference for every type, interface, enum, and tag

## License

[Your license here]
