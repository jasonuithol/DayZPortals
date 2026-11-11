// Server side: knows every gate, watches for players stepping (or driving) into a chamber and
// sends them on. Stepping in brings lightning down on the chamber - setting off any explosives
// carried in - and a moment later everyone in it (on foot, and one vehicle with its crew) goes
// to the far gate together. A gate with no partner on its frequency is dead: nothing happens.
// The field only fires as you step in: you arrive standing in the far chamber, and nothing
// happens again (no jump back, no repeated refusal) until you have stepped out.
class DZP_GateRegistry
{
	static const int TICK_MS = 250;
	static const int STRIKE_LEAD_MS = 700;	// from the lightning to the jump

	protected static ref DZP_GateRegistry s_Instance;

	protected ref array<DZP_Gate> m_Gates = new array<DZP_Gate>();
	// vehicles standing in a chamber the field has already dealt with (Transport is an engine
	// class, so this can't live on the vehicle the way PlayerBase.m_DZP_InField does)
	protected ref array<Transport> m_VehiclesInField = new array<Transport>();
	// struck gates about to send, and who set them off or stepped in since (the rest of the
	// party is whoever else is standing in the chamber when it goes)
	protected ref map<DZP_Gate, ref array<EntityAI>> m_Boarding = new map<DZP_Gate, ref array<EntityAI>>();
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
		RefreshLinks();
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		array<Transport> vehicles = new array<Transport>();
		for (int p = 0; p < players.Count(); p++)
		{
			PlayerBase player = PlayerBase.Cast(players[p]);
			if (!player)
				continue;
			HumanCommandVehicle hcv = player.GetCommand_Vehicle();
			if (hcv && hcv.GetTransport())
			{
				// a vehicle goes once, with everyone in it
				if (vehicles.Find(hcv.GetTransport()) < 0)
					vehicles.Insert(hcv.GetTransport());
			}
			else
				CheckPlayer(player);
		}
		for (int v = 0; v < vehicles.Count(); v++)
			CheckVehicle(vehicles[v]);
	}

	// a player who has just stepped into a chamber sets it off; public for the test mission
	void CheckPlayer(PlayerBase player)
	{
		if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsInVehicle())
			return;
		DZP_Gate gate = GateAt(player.GetPosition(), DZP_Layout.FIELD_RADIUS);
		if (!gate)
		{
			player.m_DZP_InField = false;
			return;
		}
		if (player.m_DZP_InField)
			return;
		player.m_DZP_InField = true;
		array<PlayerBase> people = {player};
		SetOff(player, people, gate);
	}

	// the same for a vehicle (car or motorbike) driven into a chamber; public for the test
	// mission
	void CheckVehicle(Transport vehicle)
	{
		if (!vehicle)
			return;
		DZP_Gate gate = GateAt(vehicle.GetPosition(), DZP_Layout.FIELD_RADIUS_VEHICLE);
		if (!gate)
		{
			m_VehiclesInField.RemoveItem(vehicle);
			return;
		}
		if (m_VehiclesInField.Find(vehicle) >= 0)
			return;
		m_VehiclesInField.RemoveItem(null);
		m_VehiclesInField.Insert(vehicle);
		SetOff(vehicle, Crew(vehicle), gate);
	}

	protected array<PlayerBase> Crew(Transport vehicle)
	{
		array<PlayerBase> people = new array<PlayerBase>();
		for (int i = 0; i < vehicle.CrewSize(); i++)
		{
			PlayerBase crew = PlayerBase.Cast(vehicle.CrewMember(i));
			if (crew)
				people.Insert(crew);
		}
		return people;
	}

	// the gate whose chamber pos is in (horizontally within radius of its middle)
	protected DZP_Gate GateAt(vector pos, float radius)
	{
		for (int i = 0; i < m_Gates.Count(); i++)
		{
			DZP_Gate gate = m_Gates[i];
			if (!gate)
				continue;
			if (vector.DistanceSq(pos, gate.GetPosition()) > 900)
				continue;
			vector rel = gate.WorldToModel(pos) - DZP_Layout.CHAMBER;
			if (rel[0] * rel[0] + rel[2] * rel[2] > radius * radius)
				continue;
			if (rel[1] < -DZP_Layout.FIELD_BELOW || rel[1] > DZP_Layout.FIELD_ABOVE)
				continue;
			return gate;
		}
		return null;
	}

	// a vehicle sitting in the chamber blocks arrivals: landing on it would wreck both
	protected bool Obstructed(DZP_Gate to, Object traveller)
	{
		array<Object> objs = new array<Object>();
		GetGame().GetObjectsAtPosition(Arrival(to), DZP_Layout.FIELD_RADIUS_VEHICLE + 1.5, objs, null);
		for (int i = 0; i < objs.Count(); i++)
		{
			if (objs[i] != traveller && Transport.Cast(objs[i]))
				return true;
		}
		return false;
	}

	// the middle of the far chamber's floor, still inside its field: it won't fire until
	// whoever arrives steps out
	protected vector Arrival(DZP_Gate to)
	{
		vector arrival = to.ModelToWorld(DZP_Layout.CHAMBER);
		arrival[1] = GetGame().SurfaceY(arrival[0], arrival[2]) + 0.1;
		return arrival;
	}

	// whoever (or whatever vehicle) just came in brings the lightning down, and everyone in the
	// chamber goes a moment later. Nothing is ever announced: a gate that can't send (dead, or
	// the far chamber blocked) just doesn't strike.
	protected void SetOff(EntityAI traveller, array<PlayerBase> people, DZP_Gate from)
	{
		if (!from.DZP_IsLinked())
			return;		// nothing answers on this frequency: the gate just sits there
		if (m_Boarding.Contains(from))
		{
			m_Boarding.Get(from).Insert(traveller);		// already struck: they go with the others
			DetonateAll(traveller, people);
			return;
		}

		array<DZP_Gate> targets = new array<DZP_Gate>();
		for (int i = 0; i < m_Gates.Count(); i++)
		{
			DZP_Gate gate = m_Gates[i];
			if (gate && gate != from && gate.DZP_GetChannel() == from.DZP_GetChannel())
				targets.Insert(gate);
		}
		if (targets.Count() == 0)
			return;
		// more than two on one frequency: the field picks one. Nobody knows how.
		DZP_Gate to = targets.GetRandomElement();
		if (Obstructed(to, traveller))
			return;

		array<EntityAI> boarding = {traveller};
		m_Boarding.Insert(from, boarding);
		from.DZP_MarkStrike();
		DetonateAll(traveller, people);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Discharge, STRIKE_LEAD_MS, false, from, to);
	}

	// the lightning sets off everything volatile the traveller (and whoever's aboard) carries
	protected void DetonateAll(EntityAI traveller, array<PlayerBase> people)
	{
		if (traveller)
			DZP_Volatile.Detonate(traveller);
		for (int i = 0; i < people.Count(); i++)
			DZP_Volatile.Detonate(people[i]);
	}

	// gates with no other gate on their frequency go dead (no shimmer, no sparks)
	void RefreshLinks()
	{
		for (int i = 0; i < m_Gates.Count(); i++)
		{
			DZP_Gate gate = m_Gates[i];
			if (!gate)
				continue;
			bool linked = false;
			for (int j = 0; j < m_Gates.Count() && !linked; j++)
			{
				DZP_Gate other = m_Gates[j];
				linked = other && other != gate && other.DZP_GetChannel() == gate.DZP_GetChannel();
			}
			gate.DZP_SetLinked(linked);
		}
	}

	// after the strike: everyone in from's chamber goes to to - people on foot, and one vehicle
	// with its crew (the chamber holds one); anyone carrying contraband stays behind
	protected void Discharge(DZP_Gate from, DZP_Gate to)
	{
		array<EntityAI> boarded = m_Boarding.Get(from);
		m_Boarding.Remove(from);
		if (!from || !to)
			return;

		// everyone who might be going: those who boarded, everyone else standing there
		array<EntityAI> candidates = new array<EntityAI>();
		if (boarded)
			candidates.InsertAll(boarded);
		for (int iv = 0; iv < m_VehiclesInField.Count(); iv++)
		{
			if (candidates.Find(m_VehiclesInField[iv]) < 0)
				candidates.Insert(m_VehiclesInField[iv]);
		}
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		for (int ip = 0; ip < players.Count(); ip++)
		{
			if (candidates.Find(players[ip]) < 0)
				candidates.Insert(players[ip]);
		}

		Transport vehicle;
		array<PlayerBase> crew = new array<PlayerBase>();
		for (int v = 0; v < candidates.Count() && !vehicle; v++)
		{
			Transport candidate = Transport.Cast(candidates[v]);
			if (!candidate || GateAt(candidate.GetPosition(), DZP_Layout.FIELD_RADIUS_VEHICLE) != from)
				continue;
			vehicle = candidate;
			crew = Crew(candidate);
		}
		array<PlayerBase> walkers = new array<PlayerBase>();
		for (int p = 0; p < candidates.Count(); p++)
		{
			PlayerBase player = PlayerBase.Cast(candidates[p]);
			if (!player || !player.IsAlive() || player.IsInVehicle())
				continue;
			if (GateAt(player.GetPosition(), DZP_Layout.FIELD_RADIUS) == from)
				walkers.Insert(player);
		}
		// anyone who wandered in during the strike: what they carry goes off now
		DetonateAll(vehicle, crew);
		for (int dw = 0; dw < walkers.Count(); dw++)
			DZP_Volatile.Detonate(walkers[dw]);
		for (int dead = walkers.Count() - 1; dead >= 0; dead--)
		{
			if (!walkers[dead].IsAlive())
				walkers.Remove(dead);
		}
		if (vehicle && vehicle.IsDamageDestroyed())
			vehicle = null;
		if (!vehicle && walkers.Count() == 0)
			return;		// they all stepped out in time (or didn't survive the strike)
		if (Obstructed(to, vehicle))
			return;

		to.DZP_MarkJump();
		vector arrival = Arrival(to);
		float yaw = to.GetOrientation()[0];
		if (vehicle)
		{
			// dead stop, level, facing the way the far gate does; the crew go with it
			SetVelocity(vehicle, vector.Zero);
			dBodySetAngularVelocity(vehicle, vector.Zero);
			vehicle.SetPosition(arrival + Vector(0, DZP_Layout.VEHICLE_DROP, 0));
			vehicle.SetOrientation(Vector(yaw, 0, 0));
			for (int c = 0; c < crew.Count(); c++)
				crew[c].m_DZP_InField = true;
		}
		// people on foot in a ring round the middle (outside the vehicle, if one came along)
		float ring = DZP_Layout.WALKER_RING;
		if (vehicle)
			ring = DZP_Layout.WALKER_RING_VEHICLE;
		for (int w = 0; w < walkers.Count(); w++)
		{
			vector at = arrival;
			if (walkers.Count() > 1 || vehicle)
			{
				float angle = Math.PI2 * w / walkers.Count();
				at = to.DZP_ChamberPoint(Vector(Math.Cos(angle) * ring, 0, Math.Sin(angle) * ring), 0.1);
			}
			walkers[w].SetPosition(at);
			walkers[w].SetDirection(to.GetDirection());
			walkers[w].m_DZP_InField = true;
		}

		// in transit: each traveller's screen shows only the night sky for a moment
		SendTravel(walkers);
		SendTravel(crew);

		string who = walkers.Count().ToString() + " on foot";
		if (vehicle)
			who = who + ", " + vehicle.GetType() + " with " + crew.Count() + " aboard";
		Print("[DayZPortals] " + who + " jumped " + from.GetPosition() + " -> " + to.GetPosition() + " on " + from.DZP_GetFrequencyText());
	}

	protected void SendTravel(array<PlayerBase> people)
	{
		Param1<bool> go = new Param1<bool>(true);
		for (int i = 0; i < people.Count(); i++)
		{
			if (people[i].GetIdentity())
				GetGame().RPCSingleParam(people[i], DZP_TravelScreen.RPC_TRAVEL, go, true, people[i].GetIdentity());
		}
	}
}

// What goes off in the lightning, and what it does to whoever carries it:
// - explosives (grenades, mines, charges): go off, full strength
// - a box of rounds: one big blast - it kills
// - loose rounds, loaded magazines, loaded guns: every round cooks off, ROUND_DAMAGE each to
//   the carrier (a full AK magazine is most of a life); the rounds are gone, the gun ruined
// - smoke grenades: go off (smoke) and hurt, SMOKE_DAMAGE
class DZP_Volatile
{
	static const string BOX_BLAST = "RGD5Grenade_Ammo";	// a box of rounds goes up like a grenade
	static const float ROUND_DAMAGE = 2;
	static const float SMOKE_DAMAGE = 35;

	// already set off (a smoke grenade stays in the pocket, pouring smoke): not again
	protected static ref array<EntityAI> s_Spent = new array<EntityAI>();

	// set off everything volatile owner carries, where it is
	static void Detonate(EntityAI owner)
	{
		s_Spent.RemoveItem(null);
		array<EntityAI> items = Carried(owner);
		float hurt = 0;
		for (int i = 0; i < items.Count(); i++)
		{
			EntityAI item = items[i];
			if (s_Spent.Find(item) >= 0)
				continue;
			bool spent = true;
			if (IsSmoke(item))
			{
				Grenade_Base.Cast(item).ActivateImmediate();
				hurt += SMOKE_DAMAGE;
			}
			else if (IsExplosive(item))
			{
				item.Explode(DamageType.EXPLOSION);
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(item.DeleteSafe, 250);
			}
			else if (IsAmmoBox(item))
			{
				item.Explode(DamageType.EXPLOSION, BOX_BLAST);
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(item.DeleteSafe, 250);
			}
			else if (Rounds(item) > 0)
			{
				hurt += Rounds(item) * ROUND_DAMAGE;
				if (Weapon_Base.Cast(item))
					item.SetHealth("", "", 0);
				else
					GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(item.DeleteSafe, 250);
			}
			else
				spent = false;
			if (spent)
				s_Spent.Insert(item);
		}
		if (hurt > 0)
			owner.DecreaseHealth("", "", hurt);
	}

	// how many things owner carries would go off (for the test mission)
	static int Count(EntityAI owner)
	{
		array<EntityAI> items = Carried(owner);
		int n = 0;
		for (int i = 0; i < items.Count(); i++)
		{
			if (IsSmoke(items[i]) || IsExplosive(items[i]) || IsAmmoBox(items[i]) || Rounds(items[i]) > 0)
				n++;
		}
		return n;
	}

	protected static array<EntityAI> Carried(EntityAI owner)
	{
		array<EntityAI> items = new array<EntityAI>();
		owner.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		PlayerBase player = PlayerBase.Cast(owner);
		if (player)
		{
			EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
			if (inHands && items.Find(inHands) < 0)
				inHands.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		}
		items.RemoveItem(owner);
		items.RemoveItem(null);
		return items;
	}

	// live rounds in it: a magazine or a stack of loose rounds, or a gun's chambers and
	// internal magazine (a detachable magazine in a gun counts as itself)
	static int Rounds(EntityAI item)
	{
		Magazine mag = Magazine.Cast(item);
		if (mag)
			return mag.GetAmmoCount();

		int n = 0;
		Weapon_Base weapon = Weapon_Base.Cast(item);
		if (weapon)
		{
			for (int mi = 0; mi < weapon.GetMuzzleCount(); mi++)
			{
				if (weapon.IsChamberFull(mi) && !weapon.IsChamberFiredOut(mi))
					n++;
				if (weapon.HasInternalMagazine(mi))
					n += weapon.GetInternalMagazineCartridgeCount(mi);
			}
		}
		return n;
	}

	static bool IsAmmoBox(EntityAI item)
	{
		return item.IsInherited(Box_Base) && item.GetType().IndexOf("AmmoBox") == 0;
	}

	static bool IsSmoke(EntityAI item)
	{
		return item.IsInherited(SmokeGrenadeBase);
	}

	static bool IsExplosive(EntityAI item)
	{
		if (IsSmoke(item))
			return false;
		return item.IsInherited(ExplosivesBase) || item.IsInherited(LandMineTrap);
	}
}
