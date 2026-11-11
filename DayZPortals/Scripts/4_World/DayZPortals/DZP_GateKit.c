// The crated gate. Placed like a fence kit: the hologram is DZP_GateHologram (config
// projectionTypename), and on completion the kit turns into a DZP_Gate and is deleted
// by the deploy action.
class DZP_GateKit extends ItemBase
{
	override bool IsBasebuildingKit()
	{
		return true;
	}

	override bool IsDeployable()
	{
		return true;
	}

	override bool CanAssignAttachmentsToQuickbar()
	{
		return false;
	}

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);
		if (GetGame().IsServer())
		{
			DZP_Gate gate = DZP_Gate.Cast(GetGame().CreateObjectEx("DZP_Gate", position, ECE_CREATEPHYSICS | ECE_KEEPHEIGHT | ECE_UPDATEPATHGRAPH));
			if (gate)
			{
				gate.SetPosition(position);
				gate.SetOrientation(orientation);
				MiscGameplayFunctions.TransferItemProperties(this, gate);
			}
			HideAllSelections();
		}
	}

	override string GetPlaceSoundset()
	{
		return "Shelter_Site_Build_Start_SoundSet";
	}

	override string GetDeploySoundset()
	{
		return "putDown_FenceKit_SoundSet";
	}

	override string GetLoopDeploySoundset()
	{
		return "Shelter_Site_Build_Loop_SoundSet";
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(ActionTogglePlaceObject);
		AddAction(ActionDeployObject);
	}
}

// Placement preview: the cabinet plus the whole gate around it, so you can see where the
// ring and the mast will end up. Only the local (client) hologram gets the scenery.
class DZP_GateHologram extends ItemBase
{
	protected ref DZP_Decor m_DZP_Decor = new DZP_Decor();

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsDedicatedServer())
			return;
		int low, high;
		GetNetworkID(low, high);
		if (GetGame().IsMultiplayer() && (low != 0 || high != 0))
			return;		// the server's copy of the projection, not ours
		array<ref DZP_DecorPart> parts = new array<ref DZP_DecorPart>();
		DZP_Layout.Built(parts);
		m_DZP_Decor.Spawn(this, parts, true);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		m_DZP_Decor.Clear();
	}
}
