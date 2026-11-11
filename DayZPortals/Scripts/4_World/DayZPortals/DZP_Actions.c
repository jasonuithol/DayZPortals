// Tuning: two interact actions on an assembled gate, one channel up or down.
class DZP_ActionGateTune: ActionInteractBase
{
	protected int m_DZP_Step;
	protected string m_DZP_Verb;

	void DZP_ActionGateTune()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "Tune";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone();
		m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		DZP_Gate gate = DZP_Gate.Cast(target.GetObject());
		return gate && gate.DZP_IsBuilt();
	}

	override void OnActionInfoUpdate(PlayerBase player, ActionTarget target, ItemBase item)
	{
		DZP_Gate gate = DZP_Gate.Cast(target.GetObject());
		if (gate)
			m_Text = m_DZP_Verb + " (now " + gate.DZP_GetFrequencyText() + ")";
	}

	override void OnExecuteServer(ActionData action_data)
	{
		DZP_Gate gate = DZP_Gate.Cast(action_data.m_Target.GetObject());
		if (gate && gate.DZP_IsBuilt())
			gate.DZP_Tune(m_DZP_Step);
	}

	override void OnExecuteClient(ActionData action_data)
	{
		EffectSound snd = SEffectManager.PlaySound("Shockpistol_charge_close_release_SoundSet", action_data.m_Target.GetObject().GetPosition());
		if (snd)
			snd.SetAutodestroy(true);
	}
}

class DZP_ActionGateTuneUp: DZP_ActionGateTune
{
	void DZP_ActionGateTuneUp()
	{
		m_DZP_Step = 1;
		m_DZP_Verb = "Tune up";
	}
}

class DZP_ActionGateTuneDown: DZP_ActionGateTune
{
	void DZP_ActionGateTuneDown()
	{
		m_DZP_Step = -1;
		m_DZP_Verb = "Tune down";
	}
}

// Assembling and dismantling: a hammer in hand, a few seconds of banging.
class DZP_ActionGateWorkCB: ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(UATimeSpent.BASEBUILDING_CONSTRUCT_MEDIUM);
	}
}

class DZP_ActionAssembleGate: ActionContinuousBase
{
	void DZP_ActionAssembleGate()
	{
		m_CallbackClass = DZP_ActionGateWorkCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_ASSEMBLE;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
		m_Text = "Assemble the Pereval apparatus";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTCursor(UAMaxDistances.BASEBUILDING);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		DZP_Gate gate = DZP_Gate.Cast(target.GetObject());
		return gate && !gate.DZP_IsBuilt() && gate.DZP_HasAnyAttachment();
	}

	// with materials missing the action still shows, saying what's missing, but can't start
	override void OnActionInfoUpdate(PlayerBase player, ActionTarget target, ItemBase item)
	{
		DZP_Gate gate = DZP_Gate.Cast(target.GetObject());
		if (!gate)
			return;
		string missing = gate.DZP_MissingText();
		if (missing == "")
			m_Text = "Assemble the Pereval apparatus";
		else
			m_Text = "Assemble (needs " + missing + ")";
	}

	override bool ActionConditionContinue(ActionData action_data)
	{
		DZP_Gate gate = DZP_Gate.Cast(action_data.m_Target.GetObject());
		return gate && gate.DZP_CanAssemble();
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		DZP_Gate gate = DZP_Gate.Cast(action_data.m_Target.GetObject());
		if (gate && gate.DZP_CanAssemble())
		{
			gate.DZP_Assemble();
			action_data.m_MainItem.DecreaseHealth(UADamageApplied.BUILD, false);
		}
	}

	override string GetSoundCategory(ActionData action_data)
	{
		return "Base_building";
	}
}

class DZP_ActionDismantleGate: ActionContinuousBase
{
	void DZP_ActionDismantleGate()
	{
		m_CallbackClass = DZP_ActionGateWorkCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_DISASSEMBLE;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
		m_Text = "Crate the Pereval apparatus";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTCursor(UAMaxDistances.BASEBUILDING);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		DZP_Gate gate = DZP_Gate.Cast(target.GetObject());
		return gate && gate.DZP_CanDismantle();
	}

	override void OnActionInfoUpdate(PlayerBase player, ActionTarget target, ItemBase item)
	{
		DZP_Gate gate = DZP_Gate.Cast(target.GetObject());
		if (gate && gate.DZP_IsBuilt())
			m_Text = "Crate the Pereval apparatus (materials are lost)";
		else
			m_Text = "Crate the Pereval apparatus";
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		DZP_Gate gate = DZP_Gate.Cast(action_data.m_Target.GetObject());
		if (gate && gate.DZP_CanDismantle())
		{
			gate.DZP_Dismantle();
			action_data.m_MainItem.DecreaseHealth(UADamageApplied.BUILD, false);
		}
	}

	override string GetSoundCategory(ActionData action_data)
	{
		return "Base_building";
	}
}
