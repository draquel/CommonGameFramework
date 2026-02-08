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
}
