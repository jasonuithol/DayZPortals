class CfgPatches
{
	class DayZPortals
	{
		units[] = {"DZP_GateKit", "DZP_Gate"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Gear_Camping", "DZ_Gear_Crafting", "DZ_Gear_Tools", "DZ_Radio", "DZ_Vehicles_Parts", "DZ_Structures_Furniture", "DZ_Structures_Industrial", "DZ_Structures_Military", "DZ_Structures_Bliss_Industrial", "DZ_Structures_Bliss_Underground", "DZ_Sounds_Effects", "DZ_Sounds_Bliss"};
	};
};

class CfgMods
{
	class DayZPortals
	{
		dir = "DayZPortals";
		name = "DayZPortals";
		type = "mod";
		dependencies[] = {"Game", "World"};

		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"DayZPortals/Scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"DayZPortals/Scripts/4_World"};
			};
		};
	};
};

class CfgVehicles
{
	class Inventory_Base;

	// Crated gate, crafted from an electronic repair kit and metal wire, deployed from the hands
	class DZP_GateKit: Inventory_Base
	{
		scope = 2;
		displayName = "Pereval apparatus (crated)";
		descriptionShort = "A Soviet crate stencilled OBJECT 47 - PEREVAL. Inside: a control cabinet, a hatch ring and a great deal of copper. Place it, then fit 10 metal sheets, metal wire, a truck battery and a walkie-talkie and assemble it with a hammer.";
		model = "\dz\gear\camping\wooden_case.p3d";
		projectionTypename = "DZP_GateHologram";
		weight = 15000;
		itemSize[] = {6, 6};
		itemBehaviour = 0;
		heavyItem = 1;
		physLayer = "item_large";
		rotationFlags = 2;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 500;
				};
			};
		};
	};

	// What the player sees while placing the kit
	class DZP_GateHologram: Inventory_Base
	{
		scope = 1;
		model = "DZ\structures\Furniture\radar_equipment\radar_panel.p3d";
		alignHologramToTerain = 0;
		slopeTolerance = 0.3;
	};

	// The gate itself: the control cabinet carries the inventory, the rest is spawned around it
	class DZP_Gate: Inventory_Base
	{
		scope = 2;
		displayName = "Pereval transit apparatus";
		descriptionShort = "Object 47. Tune two of them to the same frequency and touch the plate. Ammunition and explosives detonate in the field: the interlock will not let you through with them.";
		model = "DZ\structures\Furniture\radar_equipment\radar_panel.p3d";
		weight = 100000;
		itemSize[] = {10, 10};
		itemBehaviour = 0;
		heavyItem = 1;
		carveNavmesh = 1;
		physLayer = "item_large";
		attachments[] = {"Material_MetalSheets", "Material_MetalWire", "TruckBattery", "WalkieTalkie"};
		class GUIInventoryAttachmentsProps
		{
			class Parts
			{
				name = "Assembly";
				description = "";
				attachmentSlots[] = {"Material_MetalSheets", "Material_MetalWire", "TruckBattery", "WalkieTalkie"};
				icon = "set:dayz_inventory image:cat_bb_material";
				selection = "";
			};
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 100000;
				};
			};
		};
	};
};
