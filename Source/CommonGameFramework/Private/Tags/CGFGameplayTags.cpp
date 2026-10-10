#include "Tags/CGFGameplayTags.h"

namespace CGFGameplayTags
{
	// Item.Category
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Weapon,     "Item.Category.Weapon",     "Weapons (swords, bows, staves)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Armor,      "Item.Category.Armor",      "Armor pieces");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Consumable, "Item.Category.Consumable", "Potions, food, scrolls");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Material,   "Item.Category.Material",   "Crafting materials, resources");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Quest,      "Item.Category.Quest",      "Quest items (often non-droppable)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Category_Misc,       "Item.Category.Misc",       "Everything else");

	// Item.Rarity
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Common,    "Item.Rarity.Common",    "Common rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Uncommon,  "Item.Rarity.Uncommon",  "Uncommon rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Rare,      "Item.Rarity.Rare",      "Rare rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Epic,      "Item.Rarity.Epic",      "Epic rarity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Rarity_Legendary, "Item.Rarity.Legendary", "Legendary rarity");

	// Item.Key
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Item_Key_DungeonBoss, "Item.Key.DungeonBoss", "Opens a dungeon boss-room door");

	// Inventory.Type
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Type_Player,    "Inventory.Type.Player",    "Player's personal inventory");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Type_Container, "Inventory.Type.Container", "World containers (chests, barrels)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Type_NPC,       "Inventory.Type.NPC",       "NPC inventories (vendors, enemies)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Inventory_Type_Loot,      "Inventory.Type.Loot",      "Temporary loot drop inventories");

	// Equipment.Slot
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Head,       "Equipment.Slot.Head",       "Head armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Chest,      "Equipment.Slot.Chest",      "Chest armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Legs,       "Equipment.Slot.Legs",       "Leg armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Feet,       "Equipment.Slot.Feet",       "Foot armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Hands,      "Equipment.Slot.Hands",      "Hand armor slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_MainHand,   "Equipment.Slot.MainHand",   "Main hand weapon slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_OffHand,    "Equipment.Slot.OffHand",    "Off hand weapon/shield slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Accessory1, "Equipment.Slot.Accessory1", "First accessory slot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_Accessory2, "Equipment.Slot.Accessory2", "Second accessory slot");

	// Interaction.Type
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Pickup,  "Interaction.Type.Pickup",  "Pick up an item");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Drop,    "Interaction.Type.Drop",    "Drop an item");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Use,     "Interaction.Type.Use",     "Use/activate (context-dependent)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Open,    "Interaction.Type.Open",    "Open a container, door, etc.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Equip,   "Interaction.Type.Equip",   "Equip an item directly from world");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Inspect, "Interaction.Type.Inspect", "Examine/read without taking");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Search,  "Interaction.Type.Search",  "Search a container for loot");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interaction_Type_Unlock,  "Interaction.Type.Unlock",  "Unlock a lock with a key");

	// Loot.Source
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Loot_Source_DungeonTreasure,  "Loot.Source.DungeonTreasure",  "Roll context: a dungeon treasure-room chest");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Loot_Source_DungeonContainer, "Loot.Source.DungeonContainer", "Roll context: a searchable dungeon crate / barrel");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Loot_Source_DungeonBoss,      "Loot.Source.DungeonBoss",      "Roll context: a dungeon boss's death reward");

	// Faction
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Player,  "Faction.Player",  "Player characters");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Monster, "Faction.Monster", "Hostile creatures and dungeon enemies");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Faction_Neutral, "Faction.Neutral", "Never hostile to anyone (training dummies, townsfolk)");

	// Damage.Type
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Physical, "Damage.Type.Physical", "Melee/ranged physical damage; mitigated by defense");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Fire,     "Damage.Type.Fire",     "Fire damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Poison,   "Damage.Type.Poison",   "Poison damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Type_Pure,     "Damage.Type.Pure",     "Unmitigated damage (ignores defense)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Damage_Critical,      "Damage.Critical",      "Context tag on a hit that rolled a critical");

	// State
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead,         "State.Dead",         "Combatant has died; rejects damage and ability activation");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Downed,       "State.Downed",       "Knocked out but recoverable; rejects ability activation");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Takes no damage while held");

	// Event.Combat
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_Damaged, "Event.Combat.Damaged", "Sent to the target ASC after damage is applied");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_Downed,  "Event.Combat.Downed",  "Sent to the target ASC when it is knocked out");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Combat_Died,    "Event.Combat.Died",    "Sent to the target ASC when it dies");

	// Objectives
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Objective_Kind_BossKill,     "Objective.Kind.BossKill",     "Objective kind: defeat the room's boss");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Objective_Completed,   "Event.Objective.Completed",   "Sent to the completing player's ASC when an objective is cleared");

	// Ability
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack_Melee, "Ability.Attack.Melee", "Melee attack ability identity");

	// SetByCaller
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage", "Base damage magnitude on a damage effect spec");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Stat,   "SetByCaller.Stat",   "Root of SetByCaller.Stat.<AttributeName> keys on stat-modifier effects");
}
