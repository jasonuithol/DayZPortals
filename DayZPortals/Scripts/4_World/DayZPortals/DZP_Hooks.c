modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(DZP_ActionGateTuneUp);
		actions.Insert(DZP_ActionGateTuneDown);
	}
}

modded class PlayerBase
{
	bool m_DZP_InField;	// server: standing in a chamber the field has already dealt with
	bool m_DZP_TuneDown;	// client: last tuned a gate down, so "Tune down" is offered first

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		if (rpc_type != DZP_TravelScreen.RPC_TRAVEL || GetGame().IsDedicatedServer())
			return;
		DZP_TravelScreen.Show();
	}
}
