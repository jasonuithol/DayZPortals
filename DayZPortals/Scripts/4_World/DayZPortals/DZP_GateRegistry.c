// Server side: knows every gate, watches for players touching a plate and sends them on.
class DZP_GateRegistry
{
	static const int TICK_MS = 250;
	static const float COOLDOWN = 4.0;		// after a jump, before the next one
	static const float REFUSE_COOLDOWN = 3.0;	// between "can't go" messages

	protected static ref DZP_GateRegistry s_Instance;

	protected ref array<DZP_Gate> m_Gates = new array<DZP_Gate>();
	protected bool m_Ticking;

	static DZP_GateRegistry Get()
	{
		if (!s_Instance)
			s_Instance = new DZP_GateRegistry();
		return s_Instance;
	}

	void Register(DZP_Gate gate)
	{
		if (m_Gates.Find(gate) < 0)
			m_Gates.Insert(gate);
		if (!m_Ticking)
		{
			m_Ticking = true;
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Tick, TICK_MS, true);
		}
	}

	void Unregister(DZP_Gate gate)
	{
		m_Gates.RemoveItem(gate);
	}

	int Count()
	{
		return m_Gates.Count();
	}

	DZP_Gate GetGate(int i)
	{
		return m_Gates[i];
	}

	protected void Tick()
	{
		if (m_Gates.Count() == 0)
			return;
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		float now = GetGame().GetTickTime();
		for (int p = 0; p < players.Count(); p++)
			CheckPlayer(PlayerBase.Cast(players[p]), now);
	}

	// jump the player if they are at a plate; public for the test mission
	void CheckPlayer(PlayerBase player, float now)
	{
		if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsInVehicle())
			return;
		if (now < player.m_DZP_NextGateTime)
			return;
		DZP_Gate gate = TouchedGate(player);
		if (gate)
			TryJump(player, gate, now);
	}

	// the assembled gate whose plate the player is standing at, if any
	protected DZP_Gate TouchedGate(PlayerBase player)
	{
		vector pos = player.GetPosition();
		for (int i = 0; i < m_Gates.Count(); i++)
		{
			DZP_Gate gate = m_Gates[i];
			if (!gate || !gate.DZP_IsBuilt())
				continue;
			if (vector.DistanceSq(pos, gate.GetPosition()) > 100)
				continue;
			vector rel = gate.WorldToModel(pos) - DZP_Layout.RING;
			if (Math.AbsFloat(rel[0]) > DZP_Layout.TRIGGER_HALF_WIDTH)
				continue;
			if (Math.AbsFloat(rel[2]) > DZP_Layout.TRIGGER_DEPTH)
				continue;
			// rel[1] is relative to the ring centre; the player's feet are below it
			float feet = rel[1] + DZP_Layout.RING[1];
			if (feet < -0.6 || feet > DZP_Layout.TRIGGER_HEIGHT)
				continue;
			return gate;
		}
		return null;
	}

	protected void TryJump(PlayerBase player, DZP_Gate from, float now)
	{
		EntityAI contraband = DZP_Contraband.Find(player);
		if (contraband)
		{
			player.m_DZP_NextGateTime = now + REFUSE_COOLDOWN;
			Notify(player, "INTERLOCK ENGAGED", "The field will not take " + contraband.GetDisplayName() + ". No ammunition or explosives through the gate.");
			return;
		}

		array<DZP_Gate> targets = new array<DZP_Gate>();
		for (int i = 0; i < m_Gates.Count(); i++)
		{
			DZP_Gate gate = m_Gates[i];
			if (gate && gate != from && gate.DZP_IsBuilt() && gate.DZP_GetChannel() == from.DZP_GetChannel())
				targets.Insert(gate);
		}
		if (targets.Count() == 0)
		{
			player.m_DZP_NextGateTime = now + REFUSE_COOLDOWN;
			Notify(player, "NO CARRIER", "Nothing answers on " + from.DZP_GetFrequencyText() + ".");
			return;
		}

		// more than two on one frequency: the field picks one. Nobody knows how.
		DZP_Gate to = targets.GetRandomElement();
		vector arrival = to.ModelToWorld(DZP_Layout.RING + Vector(0, -DZP_Layout.RING[1] + 0.1, DZP_Layout.ARRIVAL_DISTANCE));
		float ground = GetGame().SurfaceY(arrival[0], arrival[2]);
		if (arrival[1] < ground)
			arrival[1] = ground;

		player.m_DZP_NextGateTime = now + COOLDOWN;
		from.DZP_MarkJump();
		to.DZP_MarkJump();
		player.SetPosition(arrival);
		player.SetDirection(to.GetDirection());
		string who = "?";
		if (player.GetIdentity())
			who = player.GetIdentity().GetName();
		Print("[DayZPortals] " + who + " jumped " + from.GetPosition() + " -> " + to.GetPosition() + " on " + from.DZP_GetFrequencyText());
	}

	protected void Notify(PlayerBase player, string title, string text)
	{
		if (player.GetIdentity())
			NotificationSystem.SendNotificationToPlayerExtended(player, 5, "PEREVAL: " + title, text);
	}
}

// What the gate won't take: live rounds (loose, in magazines, in a chamber or internal
// magazine) and explosives (grenades other than smoke, mines, charges).
class DZP_Contraband
{
	static EntityAI Find(PlayerBase player)
	{
		array<EntityAI> items = new array<EntityAI>();
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
		if (inHands && items.Find(inHands) < 0)
			inHands.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);

		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items[i];
			if (!item || item == player)
				continue;
			if (IsContraband(item))
				return item;
		}
		return null;
	}

	static bool IsContraband(EntityAI item)
	{
		Magazine mag = Magazine.Cast(item);
		if (mag)
			return mag.GetAmmoCount() > 0;

		Weapon_Base weapon = Weapon_Base.Cast(item);
		if (weapon)
		{
			for (int mi = 0; mi < weapon.GetMuzzleCount(); mi++)
			{
				if (weapon.IsChamberFull(mi) && !weapon.IsChamberFiredOut(mi))
					return true;
				if (weapon.HasInternalMagazine(mi) && weapon.GetInternalMagazineCartridgeCount(mi) > 0)
					return true;
			}
			return false;
		}

		if (item.IsInherited(SmokeGrenadeBase))
			return false;
		return item.IsInherited(ExplosivesBase) || item.IsInherited(LandMineTrap);
	}
}
