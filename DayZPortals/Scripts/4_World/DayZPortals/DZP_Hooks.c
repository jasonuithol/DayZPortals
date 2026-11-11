// Crate a gate: an electronic repair kit (used up) and one metal wire, combined in hands.
class DZP_CraftGateKit extends RecipeBase
{
	override void Init()
	{
		m_Name = "Crate a Pereval apparatus";
		m_IsInstaRecipe = false;
		m_AnimationLength = 1.5;

		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = -1;
		m_MaxQuantityIngredient[0] = -1;

		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = 1;
		m_MaxQuantityIngredient[1] = -1;

		InsertIngredient(0, "ElectronicRepairKit");
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = true;

		InsertIngredient(1, "MetalWire");
		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = -1;
		m_IngredientDestroy[1] = false;

		AddResult("DZP_GateKit");
		m_ResultSetFullQuantity[0] = false;
		m_ResultSetQuantity[0] = -1;
		m_ResultSetHealth[0] = -1;
		m_ResultInheritsHealth[0] = -1;
		m_ResultInheritsColor[0] = -1;
		m_ResultToInventory[0] = -2;
		m_ResultReplacesIngredient[0] = -1;
	}

	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		return !ingredients[0].GetInventory().IsAttachment() && !ingredients[1].GetInventory().IsAttachment();
	}
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new DZP_CraftGateKit);
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(DZP_ActionGateTuneUp);
		actions.Insert(DZP_ActionGateTuneDown);
		actions.Insert(DZP_ActionAssembleGate);
		actions.Insert(DZP_ActionDismantleGate);
	}
}

modded class Hammer
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(DZP_ActionAssembleGate);
		AddAction(DZP_ActionDismantleGate);
	}
}

modded class PlayerBase
{
	float m_DZP_NextGateTime;	// server: no gate before this tick time (after a jump or a refusal)
}
