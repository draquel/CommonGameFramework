# CommonGameFramework

The foundation of the game framework plugins for Unreal Engine 5.8: the shared types (`FItemInstance`,
`FInventorySlot`, interaction, equipment, loot and combat structs), the eight `ICGF*` interfaces, the native gameplay
tags (`Item.*`, `Inventory.*`, `Equipment.*`, `Interaction.*`, `Loot.*`, `Crafting.*`, combat) and static helpers. It
holds no gameplay logic and depends on no other framework plugin; ItemInventoryPlugin, InteractionPlugin and
EquipmentPlugin build on it. Requires the GameplayAbilities plugin.

- [ARCHITECTURE.md](Documentation/ARCHITECTURE.md) — how the framework plugins fit together
- [COMMON_TYPES.md](Documentation/COMMON_TYPES.md) — every type, interface, delegate and tag
- [.claude/instructions.md](.claude/instructions.md) — rules for working in this plugin
