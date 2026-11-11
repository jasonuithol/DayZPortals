// Pereval transit apparatus: the radio panel of a derelict Soviet installation, which it
// builds round itself (DZP_Layout.Installation). Tuned to one of DZP_Layout.CHANNELS
// frequencies; stepping into its chamber sends you to another gate on the same frequency
// (DZP_GateRegistry does the watching and the jumping, on the server). Placed by missions and
// admins (DZP_Spawn), or anywhere else something creates a DZP_Gate.
class DZP_Gate extends ItemBase
{
	static const int STORE_VERSION = 2;	// 1 also saved an "assembled" flag (0.1-0.2, crafting)

	protected int m_DZP_Channel;
	protected int m_DZP_JumpFx;		// bumped by the server on every jump, clients play the burst
	protected int m_DZP_JumpFxSeen;
	protected bool m_DZP_Linked;		// another gate shares the frequency (server keeps it)
	protected int m_DZP_StrikeFx;		// bumped by the server when someone sets it off: lightning
	protected int m_DZP_StrikeFxSeen;
	protected bool m_DZP_Synced;		// the first sync only catches up, it plays nothing

	protected ref DZP_Decor m_DZP_Decor = new DZP_Decor();
	protected vector m_DZP_DecorAt;		// where the gate was when its installation went up
	protected vector m_DZP_DecorOri;

	// client only
	protected ref array<Particle> m_DZP_Vapour = new array<Particle>();
	protected EffectSound m_DZP_Hum;

	void DZP_Gate()
	{
		m_DZP_Channel = Math.RandomInt(0, DZP_Layout.CHANNELS);
		RegisterNetSyncVariableInt("m_DZP_Channel", 0, DZP_Layout.CHANNELS - 1);
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
		// whoever creates us sets position and orientation right after
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DZP_RefreshDecor, 200, false);
		// offline (the DayZ Editor) gates get dragged about: the installation follows
		if (!GetGame().IsMultiplayer())
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(DZP_FollowMoves, 500, true);
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (GetGame().IsServer())
			DZP_GateRegistry.Get().Unregister(this);
		if (!GetGame().IsDedicatedServer())
			DZP_GateShimmer.Unregister(this);
		if (GetGame())
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(DZP_FollowMoves);
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
		bool assembled;
		if (storeVersion < 2 && !ctx.Read(assembled))
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

	//! Server: a gate at pos (on the ground) tuned to channel, for missions, admins and test
	//! beds; set its orientation afterwards. Null if it can't be created.
	static DZP_Gate DZP_Spawn(vector pos, int channel)
	{
		DZP_Gate gate = DZP_Gate.Cast(GetGame().CreateObjectEx("DZP_Gate", pos, ECE_PLACE_ON_SURFACE));
		if (gate)
			gate.DZP_TuneTo(channel);
		return gate;
	}

	//! 0.2's name for DZP_Spawn (gates were assembled then)
	static DZP_Gate DZP_SpawnAssembled(vector pos, int channel)
	{
		return DZP_Spawn(pos, channel);
	}

	// the installation round the panel is up (it goes up a moment after the gate is created)
	bool DZP_IsInstalled()
	{
		return !m_DZP_Decor.IsEmpty();
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

	override void SetActions()
	{
		super.SetActions();
		AddAction(DZP_ActionGateTuneUp);
		AddAction(DZP_ActionGateTuneDown);
	}

	// --- looks -----------------------------------------------------------------------------

	// the installation round the panel, once (in play the gate never moves)
	void DZP_RefreshDecor()
	{
		if (!m_DZP_Decor.IsEmpty())
			return;
		array<ref DZP_DecorPart> parts = new array<ref DZP_DecorPart>();
		DZP_Layout.Installation(parts);
		m_DZP_Decor.Spawn(this, parts);
		m_DZP_DecorAt = GetPosition();
		m_DZP_DecorOri = GetOrientation();
		if (!GetGame().IsDedicatedServer())
			DZP_StartAmbience();
	}

	// offline only: rebuild the installation where the gate has been moved to
	protected void DZP_FollowMoves()
	{
		if (m_DZP_Decor.IsEmpty())
			return;
		if (vector.DistanceSq(GetPosition(), m_DZP_DecorAt) < 0.0001 && vector.DistanceSq(GetOrientation(), m_DZP_DecorOri) < 0.0001)
			return;
		m_DZP_Decor.Clear();
		DZP_StopAmbience();
		DZP_RefreshDecor();
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
		if (GetGame().IsDedicatedServer())
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
		if (GetGame().IsDedicatedServer())
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
