// Pereval transit apparatus. Placed from a DZP_GateKit, assembled with a hammer once its
// materials are attached, then tuned to one of DZP_Layout.CHANNELS frequencies. Touching the
// chamber of an assembled gate sends you to another assembled gate on the same frequency
// (DZP_GateRegistry does the watching and the jumping, on the server).
class DZP_Gate extends ItemBase
{
	static const int STORE_VERSION = 1;

	protected int m_DZP_Channel;
	protected bool m_DZP_Built;
	protected int m_DZP_JumpFx;		// bumped by the server on every jump, clients play the burst
	protected int m_DZP_JumpFxSeen;
	protected bool m_DZP_Linked;		// another assembled gate shares the frequency (server keeps it)
	protected int m_DZP_StrikeFx;		// bumped by the server when someone sets it off: lightning
	protected int m_DZP_StrikeFxSeen;
	protected bool m_DZP_Synced;		// the first sync only catches up, it plays nothing

	protected ref DZP_Decor m_DZP_Decor = new DZP_Decor();
	protected bool m_DZP_DecorBuilt;

	// client only
	protected ref array<Particle> m_DZP_Vapour = new array<Particle>();
	protected EffectSound m_DZP_Hum;

	void DZP_Gate()
	{
		m_DZP_Channel = Math.RandomInt(0, DZP_Layout.CHANNELS);
		RegisterNetSyncVariableInt("m_DZP_Channel", 0, DZP_Layout.CHANNELS - 1);
		RegisterNetSyncVariableBool("m_DZP_Built");
		RegisterNetSyncVariableInt("m_DZP_JumpFx", 0, 255);
		RegisterNetSyncVariableInt("m_DZP_StrikeFx", 0, 255);
		RegisterNetSyncVariableBool("m_DZP_Linked");
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
		if (m_DZP_StrikeFx != m_DZP_StrikeFxSeen)
		{
			m_DZP_StrikeFxSeen = m_DZP_StrikeFx;
			if (m_DZP_Synced)
				DZP_PlayStrike();
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

	void DZP_TuneTo(int channel)
	{
		DZP_Tune(channel - m_DZP_Channel);
	}

	//! Server: a ready-to-use gate - parts fitted, assembled and tuned to channel - for
	//! missions, test beds and admin tools. Null if the gate can't be created.
	static DZP_Gate DZP_SpawnAssembled(vector pos, int channel)
	{
		DZP_Gate gate = DZP_Gate.Cast(GetGame().CreateObjectEx("DZP_Gate", pos, ECE_PLACE_ON_SURFACE));
		if (!gate)
			return null;
		ItemBase plates = ItemBase.Cast(gate.GetInventory().CreateAttachment("MetalPlate"));
		if (plates)
			plates.SetQuantity(DZP_Layout.PLATES_NEEDED);
		ItemBase wire = ItemBase.Cast(gate.GetInventory().CreateAttachment("MetalWire"));
		if (wire)
			wire.SetQuantity(DZP_Layout.WIRE_NEEDED);
		gate.GetInventory().CreateAttachment("TruckBattery");
		gate.GetInventory().CreateAttachment("PersonalRadio");
		gate.DZP_Assemble();
		gate.DZP_TuneTo(channel);
		return gate;
	}

	bool DZP_IsLinked()
	{
		return m_DZP_Linked;
	}

	// server (DZP_GateRegistry): a gate nothing answers is dead - no shimmer, no sparks
	void DZP_SetLinked(bool linked)
	{
		if (m_DZP_Linked == linked)
			return;
		m_DZP_Linked = linked;
		SetSynchDirty();
	}

	// lightning on the chamber, just before whoever is in it is sent
	void DZP_MarkStrike()
	{
		m_DZP_StrikeFx = (m_DZP_StrikeFx + 1) & 255;
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

	// a point in the chamber, height metres over its floor (the ground there, not the panel's)
	vector DZP_ChamberPoint(vector offset, float height)
	{
		vector at = ModelToWorld(DZP_Layout.CHAMBER + offset);
		at[1] = GetGame().SurfaceY(at[0], at[2]) + height;
		return at;
	}

	// the middle of the chamber at head height: the jump burst, the shimmer
	vector DZP_FieldCentre()
	{
		return DZP_ChamberPoint(vector.Zero, DZP_Layout.EFFECT_HEIGHT);
	}

	protected void DZP_StartAmbience()
	{
		// faint vapour rising off the floor round the edge of the field
		if (m_DZP_Vapour.Count() == 0)
		{
			float radius = DZP_Layout.FIELD_RADIUS + 0.2;
			for (int i = 0; i < 7; i++)
			{
				float angle = Math.PI2 * i / 7;
				vector at = DZP_ChamberPoint(Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius), 0.1);
				Particle p = ParticleManager.GetInstance().PlayInWorld(ParticleList.ITEM_HOT_VAPOR, at);
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

	// a random crackle somewhere up in the gantries round the chamber
	protected void DZP_Spark()
	{
		if (!m_DZP_Built)
			return;
		// a dead gate (nothing on its frequency) doesn't crackle; keep checking
		if (m_DZP_Linked)
			DZP_SparkNow();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DZP_Spark, Math.RandomInt(1500, 6000), false);
	}

	protected void DZP_SparkNow()
	{
		float angle = Math.RandomFloat(0, Math.PI2);
		float radius = Math.RandomFloat(2.5, 3.5);
		vector pos = DZP_ChamberPoint(Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius), Math.RandomFloat(2, 6));
		ParticleManager.GetInstance().PlayInWorld(ParticleList.LEVER_SPARKS, pos);
		EffectSound snd = SEffectManager.PlaySound("AutoBattery_Spark_SoundSet", pos);
		if (snd)
			snd.SetAutodestroy(true);
	}

	protected void DZP_PlayBurst()
	{
		if (GetGame().IsDedicatedServer() || !m_DZP_Built)
			return;
		vector centre = DZP_FieldCentre();
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

	// a bolt straight down into the middle of the chamber: the vanilla lightning models (as the
	// weather uses), a flash and a near thunderclap
	protected void DZP_PlayStrike()
	{
		if (GetGame().IsDedicatedServer() || !m_DZP_Built)
			return;
		vector floor = DZP_ChamberPoint(vector.Zero, 0);
		float yaw = Math.RandomFloat(0, 360);
		Object bolt1 = GetGame().CreateStaticObjectUsingP3D("DZ/data/data/blesk1.p3d", floor, Vector(yaw, 0, 0), 1.5, true);
		Object bolt2 = GetGame().CreateStaticObjectUsingP3D("DZ/data/data/blesk2.p3d", floor, Vector(yaw + 90, 0, 0), 1.5, true);
		if (bolt1)
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(bolt1.Delete, Math.RandomInt(150, 300));
		if (bolt2)
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(bolt2.Delete, Math.RandomInt(250, 500));

		ScriptedLightBase flash = ScriptedLightBase.CreateLight(PointLightBase, floor + Vector(0, 4, 0));
		if (flash)
		{
			flash.SetBrightnessTo(200);
			flash.SetRadiusTo(80);
			flash.SetLifetime(0.35);
			flash.SetFadeOutTime(0.2);
		}
		ParticleManager.GetInstance().PlayInWorld(ParticleList.BARBED_WIRE_SPARKS, DZP_FieldCentre());
		EffectSound thunder = SEffectManager.PlaySound("ThunderHeavy_Near_SoundSet", floor);
		if (thunder)
			thunder.SetAutodestroy(true);
	}
}
