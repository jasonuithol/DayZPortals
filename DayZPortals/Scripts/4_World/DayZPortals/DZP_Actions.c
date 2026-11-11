// Tuning: two interact actions on a gate, one channel up or down. The one the
// player used last is offered first (and so selected by default): see StandardActionInput below.
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
		return gate != null;
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
		if (gate)
			gate.DZP_Tune(m_DZP_Step);
	}

	override void OnExecuteClient(ActionData action_data)
	{
		// remembered on the client, where the prompt is ordered
		action_data.m_Player.m_DZP_TuneDown = m_DZP_Step < 0;
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

// After tuning down, "Tune down" comes first. A change in the order makes vanilla reset the
// selection to the first action, so the direction just used stays selected for the next press.
modded class StandardActionInput
{
	override protected void SortActions(array<ActionBase> allActions)
	{
		super.SortActions(allActions);
		if (!m_Player || !m_Player.m_DZP_TuneDown)
			return;
		int up = -1;
		int down = -1;
		for (int i = 0; i < allActions.Count(); i++)
		{
			if (DZP_ActionGateTuneUp.Cast(allActions[i]))
				up = i;
			else if (DZP_ActionGateTuneDown.Cast(allActions[i]))
				down = i;
		}
		if (up >= 0 && down > up)
			allActions.SwapItems(up, down);
	}
}
