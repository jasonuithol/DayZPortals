// Pereval transit apparatus. Placed from a DZP_GateKit, assembled with a hammer once its
// materials are attached, then tuned to one of DZP_Layout.CHANNELS frequencies. Touching the
// plate of an assembled gate sends you to another assembled gate on the same frequency
// (DZP_GateRegistry does the watching and the jumping, on the server).
class DZP_Gate extends ItemBase
{
	static const int STORE_VERSION = 1;

	protected int m_DZP_Channel;
	protected bool m_DZP_Built;
	protected int m_DZP_JumpFx;		// bumped by the server on every jump, clients play the burst
	protected int m_DZP_JumpFxSeen;
	protected bool m_DZP_Synced;		// the first sync only catches up, it plays nothing

	protected ref DZP_Decor m_DZP_Decor = new DZP_Decor();
	protected bool m_DZP_DecorBuilt;

	// client only
	protected Particle m_DZP_Mist;
	protected ref array<Particle> m_DZP_Vapour = new array<Particle>();
	protected EffectSound m_DZP_Hum;

	void DZP_Gate()
	{
		m_DZP_Channel = Math.RandomInt(0, DZP_Layout.CHANNELS);
		RegisterNetSyncVariableInt("m_DZP_Channel", 0, DZP_Layout.CHANNELS - 1);
		RegisterNetSyncVariableBool("m_DZP_Built");
		RegisterNetSyncVariableInt("m_DZP_JumpFx", 0, 255);
	}

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer())
			DZP_GateRegistry.Get().Register(this);
		if (!GetGame().IsDedicatedServer())
			DZP_GateShimmer.Register(this);
		// the kit sets position and orientation right after creating us
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DZP_RefreshDecor, 200, false);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (GetGame().IsServer())
			DZP_GateRegistry.Get().Unregister(this);
		if (!GetGame().IsDedicatedServer())
			DZP_GateShimmer.Unregister(this);
		m_DZP_Decor.Clear();
		DZP_StopAmbience();
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		DZP_RefreshDecor();
		if (m_DZP_JumpFx != m_DZP_JumpFxSeen)
		{
			m_DZP_JumpFxSeen = m_DZP_JumpFx;
			if (m_DZP_Synced)
				DZP_PlayBurst();
		}
		m_DZP_Synced = true;
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);
		ctx.Write(STORE_VERSION);
		ctx.Write(m_DZP_Channel);
		ctx.Write(m_DZP_Built);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;
		int storeVersion;
		if (!ctx.Read(storeVersion))
			return false;
		if (!ctx.Read(m_DZP_Channel))
			return false;
		if (!ctx.Read(m_DZP_Built))
			return false;
		return true;
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		SetSynchDirty();
		DZP_RefreshDecor();
	}

	// --- state -----------------------------------------------------------------------------

	bool DZP_IsBuilt()
	{
		return m_DZP_Built;
	}

	int DZP_GetChannel()
	{
		return m_DZP_Channel;
	}

	string DZP_GetFrequencyText()
	{
		return DZP_Layout.FrequencyText(m_DZP_Channel);
	}

	void DZP_Tune(int step)
	{
		m_DZP_Channel += step;
		if (m_DZP_Channel < 0)
			m_DZP_Channel += DZP_Layout.CHANNELS;
		if (m_DZP_Channel >= DZP_Layout.CHANNELS)
			m_DZP_Channel -= DZP_Layout.CHANNELS;
		SetSynchDirty();
	}

	void DZP_MarkJump()
	{
		m_DZP_JumpFx = (m_DZP_JumpFx + 1) & 255;
		SetSynchDirty();
	}

	// --- assembly --------------------------------------------------------------------------

	protected ItemBase DZP_Att(string slot)
	{
		return ItemBase.Cast(FindAttachmentBySlotName(slot));
	}

	bool DZP_HasAnyAttachment()
	{
		return GetInventory().AttachmentCount() > 0;
	}

	bool DZP_CanAssemble()
	{
		if (m_DZP_Built)
			return false;
		ItemBase plates = DZP_Att("Material_MetalSheets");
		ItemBase wire = DZP_Att("Material_MetalWire");
		ItemBase battery = DZP_Att("TruckBattery");
		ItemBase radio = DZP_Att("WalkieTalkie");
		if (!plates || !wire || !battery || !radio)
			return false;
		if (battery.IsRuined() || radio.IsRuined())
			return false;
		return plates.GetQuantity() >= DZP_Layout.PLATES_NEEDED && wire.GetQuantity() >= DZP_Layout.WIRE_NEEDED;
	}

	// what is still missing, for the inventory description and the action text
	string DZP_MissingText()
	{
		array<string> missing = new array<string>();
		ItemBase plates = DZP_Att("Material_MetalSheets");
		int plateCount = 0;
		if (plates)
			plateCount = plates.GetQuantity();
		if (plateCount < DZP_Layout.PLATES_NEEDED)
			missing.Insert((DZP_Layout.PLATES_NEEDED - plateCount).ToString() + " metal sheets");
		if (!DZP_Att("Material_MetalWire"))
			missing.Insert("metal wire");
		if (!DZP_Att("TruckBattery"))
			missing.Insert("a truck battery");
		if (!DZP_Att("WalkieTalkie"))
			missing.Insert("a walkie-talkie");
		string text = "";
		for (int i = 0; i < missing.Count(); i++)
		{
			if (i > 0)
				text += ", ";
			text += missing[i];
		}
		return text;
	}

	void DZP_Assemble()
	{
		if (!DZP_CanAssemble())
			return;
		DZP_Att("Material_MetalSheets").AddQuantity(-DZP_Layout.PLATES_NEEDED);
		DZP_Att("Material_MetalWire").AddQuantity(-DZP_Layout.WIRE_NEEDED);
		GetGame().ObjectDelete(DZP_Att("TruckBattery"));
		GetGame().ObjectDelete(DZP_Att("WalkieTalkie"));
		m_DZP_Built = true;
		SetSynchDirty();
		DZP_RefreshDecor();
		DZP_MarkJump();
	}

	// back into a crate; the materials that went into an assembled gate are lost
	bool DZP_CanDismantle()
	{
		return m_DZP_Built || !DZP_HasAnyAttachment();
	}

	void DZP_Dismantle()
	{
		if (!DZP_CanDismantle())
			return;
		EntityAI kit = EntityAI.Cast(GetGame().CreateObjectEx("DZP_GateKit", GetPosition(), ECE_PLACE_ON_SURFACE));
		if (kit)
			MiscGameplayFunctions.TransferItemProperties(this, kit);
		DeleteSafe();
	}

	// the gate stays where it was put: no carrying, no stashing
	override bool CanPutInCargo(EntityAI parent)
	{
		return false;
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		return false;
	}

	override bool IsTakeable()
	{
		return false;
	}

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (m_DZP_Built)
			return false;
		return super.CanReceiveAttachment(attachment, slotId);
	}

	override bool DisassembleOnLastDetach()
	{
		return false;
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(DZP_ActionGateTuneUp);
		AddAction(DZP_ActionGateTuneDown);
	}

	// --- looks -----------------------------------------------------------------------------

	void DZP_RefreshDecor()
	{
		if (!m_DZP_Decor.IsEmpty() && m_DZP_DecorBuilt == m_DZP_Built)
			return;
		array<ref DZP_DecorPart> parts = new array<ref DZP_DecorPart>();
		if (m_DZP_Built)
			DZP_Layout.Built(parts);
		else
			DZP_Layout.Unbuilt(parts);
		m_DZP_Decor.Spawn(this, parts, false);
		m_DZP_DecorBuilt = m_DZP_Built;

		if (!GetGame().IsDedicatedServer())
		{
			if (m_DZP_Built)
				DZP_StartAmbience();
			else
				DZP_StopAmbience();
		}
	}

	vector DZP_RingCentre()
	{
		return ModelToWorld(DZP_Layout.RING);
	}

	protected void DZP_StartAmbience()
	{
		if (!m_DZP_Mist)
			m_DZP_Mist = ParticleManager.GetInstance().PlayInWorld(ParticleList.CONTAMINATED_AREA_GAS_TINY, DZP_RingCentre());
		// faint vapour rising off the lower half of the grille, both faces
		if (m_DZP_Vapour.Count() == 0)
		{
			float radius = 1.0 * DZP_Layout.RING_SCALE;
			for (int i = 0; i < 7; i++)
			{
				float angle = Math.PI + Math.PI * (i + 0.5) / 7;
				float face = 0.15;
				if (i % 2 == 1)
					face = -0.15;
				vector at = DZP_Layout.RING + Vector(Math.Cos(angle) * radius, Math.Sin(angle) * radius, face);
				Particle p = ParticleManager.GetInstance().PlayInWorld(ParticleList.ITEM_HOT_VAPOR, ModelToWorld(at));
				if (p)
					m_DZP_Vapour.Insert(p);
			}
		}
		if (!m_DZP_Hum)
			m_DZP_Hum = SEffectManager.PlaySoundOnObject("Bunker_Lamp_Hum_SoundSet", this, 1, 1, true);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(DZP_Spark);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DZP_Spark, Math.RandomInt(1500, 6000), false);
	}

	protected void DZP_StopAmbience()
	{
		if (GetGame())
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(DZP_Spark);
		if (m_DZP_Mist)
		{
			m_DZP_Mist.StopParticle();
			m_DZP_Mist = null;
		}
		for (int i = 0; i < m_DZP_Vapour.Count(); i++)
		{
			if (m_DZP_Vapour[i])
				m_DZP_Vapour[i].StopParticle();
		}
		m_DZP_Vapour.Clear();
		if (m_DZP_Hum)
		{
			m_DZP_Hum.SoundStop();
			SEffectManager.DestroyEffect(m_DZP_Hum);
			m_DZP_Hum = null;
		}
	}

	// a random crackle somewhere on the rim of the ring
	protected void DZP_Spark()
	{
		if (!m_DZP_Built)
			return;
		float angle = Math.RandomFloat(0, Math.PI2);
		float radius = 1.1 * DZP_Layout.RING_SCALE;
		vector rel = DZP_Layout.RING + Vector(Math.Cos(angle) * radius, Math.Sin(angle) * radius, 0);
		vector pos = ModelToWorld(rel);
		ParticleManager.GetInstance().PlayInWorld(ParticleList.LEVER_SPARKS, pos);
		EffectSound snd = SEffectManager.PlaySound("AutoBattery_Spark_SoundSet", pos);
		if (snd)
			snd.SetAutodestroy(true);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DZP_Spark, Math.RandomInt(1500, 6000), false);
	}

	protected void DZP_PlayBurst()
	{
		if (GetGame().IsDedicatedServer() || !m_DZP_Built)
			return;
		vector centre = DZP_RingCentre();
		ParticleManager.GetInstance().PlayInWorld(ParticleList.BARBED_WIRE_SPARKS, centre);
		ParticleManager.GetInstance().PlayInWorld(ParticleList.LEVER_SPARKS, centre + Vector(0, 0.8, 0));
		ParticleManager.GetInstance().PlayInWorld(ParticleList.LEVER_SPARKS, centre - Vector(0, 0.8, 0));
		EffectSound shock = SEffectManager.PlaySound("defibrillator_shock_SoundSet", centre);
		if (shock)
			shock.SetAutodestroy(true);
		EffectSound surge = SEffectManager.PlaySound("Bunker_bunker_electricity_on_SoundSet", centre);
		if (surge)
			surge.SetAutodestroy(true);
	}
}
