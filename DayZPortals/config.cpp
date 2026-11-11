class CfgPatches
{
	class DayZPortals
	{
		units[] = {"DZP_Gate", "DZP_DangerSign"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Structures_Industrial", "DZ_Structures_Bliss_Military", "DZ_Structures_Bliss_Walls", "DZ_Structures_Signs", "DZ_Structures_Specific", "DZ_Sounds_Effects", "DZ_Sounds_Bliss"};
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
	class HouseNoDestruct;

	// The installation's warning sign: DANGER, no explosives - a bare board (no posts) hung on the
	// fence by the radio panel. Model by tools/danger_board.py, picture by tools/danger_sign.py.
	// Not that the gate refuses explosives: the lightning sets them off.
	class DZP_DangerSign: HouseNoDestruct
	{
		scope = 2;
		displayName = "Pereval danger sign";
		model = "DayZPortals\data\danger_board.p3d";
	};

	// The gate itself: the radio panel; the rest of the installation is spawned round it
	class DZP_Gate: Inventory_Base
	{
		scope = 2;
		displayName = "Pereval transit apparatus";
		descriptionShort = "Object 47. Tune two of them to the same frequency and step into the chamber. Whatever you carry that can go off, will.";
		model = "DZ\structures\specific\radio\radio_panelpas.p3d";
		weight = 100000;
		itemSize[] = {10, 10};
		itemBehaviour = 0;
		heavyItem = 1;
		carveNavmesh = 1;
		physLayer = "item_large";
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
