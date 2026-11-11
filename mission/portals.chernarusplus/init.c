// Portals
// Test bed for our DayZPortals mod (~/Projects/DayZPortals): a pair of Pereval
// gates on 412.0 MHz, one at Balota airfield where players spawn and one at the north-west
// airfield; a spread of every vanilla explosive and a loaded AKM to carry into the lightning,
// and an Ada and a motorbike to drive through.
// -missiontest: builds and checks the gates headless, then quits.
// -tour: every 12 s moves the first player to the next vantage point around the gates and
// logs it ([Tour]), for screenshots without anyone at the controls.
void main()
{
	Hive ce = CreateHive();
	if (ce)
		ce.InitOffline();

	GetGame().GetWorld().SetDate(2026, 9, 20, 10, 0);
}

class CustomMission: MissionServer
{
	static const vector GATE_A = "4810 0 2440";
	static const vector GATE_B = "4580 0 10240";

	protected DZP_Gate m_GateA;
	protected DZP_Gate m_GateB;
	protected Transport m_Car;
	protected Transport m_Bike;
	protected PlayerBase m_Dummy;
	protected PlayerBase m_Dummy2;
	protected int m_Failures;

	override void OnInit()
	{
		super.OnInit();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Setup, 3000, false);

		string value;
		if (GetGame().CommandlineGetParam("missiontest", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestBuild, 6000, false);
		if (GetGame().CommandlineGetParam("tour", value))
			GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TourStep, 12000, true);
	}

	// --- -tour -----------------------------------------------------------------------------

	protected int m_TourStep;

	void TourStep()
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		if (players.Count() == 0)
			return;
		// eye position and point looked at, in gate A's model space
		array<vector> eyes = new array<vector>();
		array<vector> looks = new array<vector>();
		vector c = DZP_Layout.CHAMBER;
		eyes.Insert(c + Vector(0, 0, 18));	looks.Insert(c);
		eyes.Insert(c + Vector(-14, 0, 10));	looks.Insert(c);
		eyes.Insert(c + Vector(18, 0, 0));	looks.Insert(c);
		eyes.Insert(c + Vector(0, 0, -18));	looks.Insert(c);
		eyes.Insert(c + Vector(0, 0, 1));	looks.Insert("0 0 0");
		int i = m_TourStep - (m_TourStep / eyes.Count()) * eyes.Count();
		m_TourStep++;
		vector eye = m_GateA.ModelToWorld(eyes[i]);
		eye[1] = GetGame().SurfaceY(eye[0], eye[2]);
		vector dir = m_GateA.ModelToWorld(looks[i]) - eye;
		dir[1] = 0;
		dir.Normalize();
		PlayerBase player = PlayerBase.Cast(players[0]);
		player.SetPosition(eye);
		player.SetDirection(dir);
		Print("[Tour] view " + i + " eye " + eyes[i] + " look " + looks[i] + " gate yaw " + m_GateA.GetOrientation()[0]);
	}

	// with persistence, the gates from last time are still there: only add what's missing
	void Setup()
	{
		DZP_GateRegistry reg = DZP_GateRegistry.Get();
		for (int i = 0; i < reg.Count(); i++)
		{
			DZP_Gate g = reg.GetGate(i);
			vector flat = g.GetPosition();
			flat[1] = 0;
			if (vector.Distance(flat, GATE_A) < 600 && !m_GateA)
				m_GateA = g;
			else if (vector.Distance(flat, GATE_B) < 600 && !m_GateB)
				m_GateB = g;
		}
		Print("[Portals] " + reg.Count() + " gates persisted");
		if (!m_GateA)
			m_GateA = NewGate(Clearing(GATE_A), 0);
		if (!m_GateB)
			m_GateB = NewGate(Clearing(GATE_B), 0);

		// things to carry into the lightning: every kind of explosive, smoke, ammo boxes and loose
		// rounds, three of each, and a loaded rifle with spare magazines
		vector loot = m_GateA.ModelToWorld("2 0 4");
		array<string> bangs = {"RGD5Grenade", "M67Grenade", "FlashGrenade", "Grenade_ChemGas", "Plastic_Explosive", "ImprovisedExplosive", "ClaymoreMine", "LandMineTrap", "RDG2SmokeGrenade_Black", "AmmoBox_762x39_20Rnd", "Ammo_762x39"};
		for (int k = 0; k < bangs.Count(); k++)
		{
			for (int n = 0; n < 3; n++)
				Drop(bangs[k], loot + Vector(k * 0.8, 0, n * 0.6));
		}
		Drop("AKM", loot + Vector(0, 0, 2.5));
		for (int m = 0; m < 3; m++)
			Drop("Mag_AKM_30Rnd", loot + Vector(0.8 + m * 0.5, 0, 2.5));

		string value;
		if (!GetGame().CommandlineGetParam("missiontest", value))
		{
			// something to drive through the chamber; once, they persist
			if (!VehicleNear(m_GateA.ModelToWorld("8 0 12"), 12))
			{
				ReadyVehicle("OffroadHatchback", m_GateA.ModelToWorld("9 0 12"));
				ReadyVehicle("Motorbike_01_Red", m_GateA.ModelToWorld("5 0 14"));
			}
		}
	}

	bool VehicleNear(vector pos, float radius)
	{
		array<Object> objs = new array<Object>();
		GetGame().GetObjectsAtPosition(pos, radius, objs, null);
		for (int i = 0; i < objs.Count(); i++)
		{
			if (Transport.Cast(objs[i]))
				return true;
		}
		return false;
	}

	// a vehicle with every part fitted and full tanks, facing the same way as gate A
	void ReadyVehicle(string type, vector pos)
	{
		pos[1] = GetGame().SurfaceY(pos[0], pos[2]) + 0.3;
		EntityAI v = EntityAI.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE | ECE_SETUP));
		if (!v)
		{
			Print("[Portals] could not create a " + type);
			return;
		}
		v.SetOrientation(m_GateA.GetOrientation());
		v.OnDebugSpawn();
		Print("[Portals] " + type + " ready at " + pos);
	}

	// nearest spot to the anchor with nothing at all (trees, bushes, walls) within 16 m,
	// searched outwards in 20 m rings
	vector Clearing(vector anchor)
	{
		array<Object> objs = new array<Object>();
		for (float r = 0; r <= 600; r += 20)
		{
			int steps = Math.Max(1, Math.Round(r * Math.PI2 / 20));
			for (int s = 0; s < steps; s++)
			{
				float a = s * Math.PI2 / steps;
				vector p = Vector(anchor[0] + Math.Cos(a) * r, 0, anchor[2] + Math.Sin(a) * r);
				p[1] = GetGame().SurfaceY(p[0], p[2]);
				if (GetGame().SurfaceIsSea(p[0], p[2]) || GetGame().SurfaceIsPond(p[0], p[2]))
					continue;
				objs.Clear();
				GetGame().GetObjectsAtPosition(p, 16, objs, null);
				if (objs.Count() == 0)
				{
					Print("[Portals] clearing " + r + " m from " + anchor + " at " + p);
					return p;
				}
			}
		}
		Print("[Portals] no clearing near " + anchor);
		return anchor;
	}

	EntityAI Drop(string type, vector pos)
	{
		pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
		return EntityAI.Cast(GetGame().CreateObjectEx(type, pos, ECE_PLACE_ON_SURFACE));
	}

	DZP_Gate NewGate(vector pos, int channel)
	{
		pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
		DZP_Gate gate = DZP_Gate.DZP_Spawn(pos, channel);
		if (!gate)
		{
			Print("[Portals] FAIL can't create DZP_Gate");
			m_Failures++;
			return null;
		}
		Print("[Portals] gate at " + gate.GetPosition() + " on " + gate.DZP_GetFrequencyText());
		return gate;
	}

	void Check(bool ok, string what)
	{
		if (ok)
		{
			Print("[Portals] ok   " + what);
		}
		else
		{
			Print("[Portals] FAIL " + what);
			m_Failures++;
		}
	}

	// --- -missiontest ----------------------------------------------------------------------

	void TestBuild()
	{
		Check(m_GateA && m_GateB, "gates A and B");
		Check(m_GateA.DZP_IsInstalled() && m_GateB.DZP_IsInstalled(), "A and B have their installations up");
		Check(m_GateA.DZP_GetChannel() == m_GateB.DZP_GetChannel(), "A and B on the same channel (" + m_GateA.DZP_GetFrequencyText() + ")");
		Check(DZP_Layout.FrequencyText(1) == "412.5 MHz" && DZP_Layout.FrequencyText(19) == "421.5 MHz", "frequency text " + DZP_Layout.FrequencyText(1) + " / " + DZP_Layout.FrequencyText(19));
		Check(m_GateA.DZP_IsLinked() && m_GateB.DZP_IsLinked(), "A and B linked (both alive)");

		// created the plain way - as the DayZ Editor or a server's object spawner would - a gate
		// puts its whole installation up by itself; alone on its frequency, it's dead
		vector p = m_GateA.ModelToWorld("0 0 60");
		p[1] = GetGame().SurfaceY(p[0], p[2]);
		DZP_Gate c = DZP_Gate.Cast(GetGame().CreateObjectEx("DZP_Gate", p, ECE_PLACE_ON_SURFACE));
		c.DZP_TuneTo(7);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestPlainGate, 1000, false, c);
	}

	void TestPlainGate(DZP_Gate c)
	{
		Check(c.DZP_IsInstalled(), "a plainly created gate puts its installation up");
		Check(!c.DZP_IsLinked(), "alone on " + c.DZP_GetFrequencyText() + ", it's dead");
		GetGame().ObjectDelete(c);

		// a survivor with no one behind it, to walk through the gates
		vector at = m_GateA.DZP_ChamberPoint(vector.Zero, 0.1);
		m_Dummy = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorM_Mirek", at, 0, "NONE"));
		Check(m_Dummy != null, "test survivor created");
		if (!m_Dummy)
		{
			Finish();
			return;
		}
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestDeadGate, 1000, false);
	}

	// tuned away from B, gate A has nobody to talk to: it goes dead and does nothing
	void TestDeadGate()
	{
		m_GateA.DZP_Tune(1);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestDeadGateStep, 600, false);
	}

	void TestDeadGateStep()
	{
		Check(!m_GateA.DZP_IsLinked() && !m_GateB.DZP_IsLinked(), "tuned apart, both gates go dead");
		vector inside = m_Dummy.GetPosition();
		m_Dummy.SetPosition(m_GateA.DZP_ChamberPoint(Vector(DZP_Layout.FIELD_RADIUS + 1, 0, 0), 0.1));
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy);
		m_Dummy.SetPosition(inside);
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestDeadGateRetune, 1500, false, inside);
	}

	void TestDeadGateRetune(vector inside)
	{
		Check(vector.Distance(m_Dummy.GetPosition(), inside) < 1, "a dead gate does nothing (survivor still at gate A)");
		m_GateA.DZP_Tune(-1);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestJump, 600, false);
	}

	void TestJump()
	{
		// the field already turned the survivor away: nothing until it steps out and back in
		vector before = m_Dummy.GetPosition();
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy);
		Check(vector.Distance(m_Dummy.GetPosition(), before) < 1, "no second try without stepping out");
		m_Dummy.SetPosition(m_GateA.DZP_ChamberPoint(Vector(DZP_Layout.FIELD_RADIUS + 1, 0, 0), 0.1));
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy);
		m_Dummy.SetPosition(before);
		// a second survivor already standing in the chamber goes along with the first
		m_Dummy2 = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorF_Eva", m_GateA.DZP_ChamberPoint("0.8 0 0", 0.1), 0, "NONE"));
		Check(m_Dummy2 != null, "second survivor in the chamber");
		Print("[Portals] survivor at " + m_Dummy.GetPosition() + ", gate A " + m_GateA.GetPosition());
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy);
		Check(vector.Distance(m_Dummy.GetPosition(), before) < 1, "lightning first: still at gate A as it strikes");
		// the second steps in while it's striking (survivors with no one behind them aren't in
		// GetPlayers, so the registry has to be told)
		if (m_Dummy2)
			DZP_GateRegistry.Get().CheckPlayer(m_Dummy2);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestArrived, 2000, false);
	}

	void TestArrived()
	{
		vector arrival = m_GateB.DZP_ChamberPoint(vector.Zero, 0.1);
		float d = vector.Distance(m_Dummy.GetPosition(), arrival);
		Check(d < 3, "survivor arrived in gate B's chamber (" + d + " m from its middle, at " + m_Dummy.GetPosition() + ")");
		if (m_Dummy2)
		{
			float d2 = vector.Distance(m_Dummy2.GetPosition(), arrival);
			Check(d2 < 3, "the second survivor came too (" + d2 + " m from the middle)");
			GetGame().ObjectDelete(m_Dummy2);
		}
		// arriving inside the far field mustn't send it straight back
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy);
		Check(vector.Distance(m_Dummy.GetPosition(), arrival) < 3, "no bounce back from gate B");
		m_Dummy.SetPosition(m_GateB.DZP_ChamberPoint(Vector(0, 0, 12), 0.1));
		TestCar();
	}

	// --- vehicles: an Ada, then a motorbike ---------------------------------------------------

	Transport Vehicle(string type, DZP_Gate gate, array<string> parts)
	{
		vector at = gate.DZP_ChamberPoint(vector.Zero, 0.6);
		Transport v = Transport.Cast(GetGame().CreateObjectEx(type, at, ECE_CREATEPHYSICS));
		if (!v)
			return null;
		v.SetOrientation(gate.GetOrientation());
		for (int i = 0; i < parts.Count(); i++)
			v.GetInventory().CreateInInventory(parts[i]);
		return v;
	}

	// step a vehicle out of the field and back in, the way a driver would to try again
	void Recheck(Transport v, DZP_Gate gate)
	{
		vector back = v.GetPosition();
		v.SetPosition(gate.DZP_ChamberPoint(Vector(0, 0, DZP_Layout.FIELD_RADIUS_VEHICLE + 2), 0.6));
		DZP_GateRegistry.Get().CheckVehicle(v);
		v.SetPosition(back);
		DZP_GateRegistry.Get().CheckVehicle(v);
	}

	void TestCar()
	{
		m_Car = Vehicle("OffroadHatchback", m_GateA, {"HatchbackWheel", "HatchbackWheel", "HatchbackWheel", "HatchbackWheel"});
		Check(m_Car != null, "Ada created in gate A's chamber");
		if (!m_Car)
		{
			Finish();
			return;
		}
		EntityAI mag = m_Car.GetInventory().CreateInInventory("Mag_AKM_30Rnd");
		Check(mag != null, "loaded magazine in the Ada's boot");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestCarJump, 1000, false, mag);
	}

	void TestCarJump(EntityAI mag)
	{
		Recheck(m_Car, m_GateA);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestCarArrived, 3000, false, mag);
	}

	void TestCarArrived(EntityAI mag)
	{
		Check(mag == null, "the magazine in the boot cooked off in the lightning");
		vector arrival = m_GateB.DZP_ChamberPoint(vector.Zero, 0);
		float d = vector.Distance(m_Car.GetPosition(), arrival);
		vector ori = m_Car.GetOrientation();
		Check(d < 3, "Ada arrived in gate B's chamber (" + d + " m from its middle)");
		Check(Math.AbsFloat(ori[1]) < 15 && Math.AbsFloat(ori[2]) < 15, "Ada landed on its wheels (pitch " + ori[1] + ", roll " + ori[2] + ")");
		Check(!m_Car.IsDamageDestroyed(), "Ada survived the trip (health " + m_Car.GetHealth() + ")");
		DZP_GateRegistry.Get().CheckVehicle(m_Car);
		Check(vector.Distance(m_Car.GetPosition(), arrival) < 3, "no bounce back for the Ada");
		TestBike();
	}

	void TestBike()
	{
		m_Bike = Vehicle("Motorbike_02_Red", m_GateA, {"Motorbike_02_Wheel_1", "Motorbike_02_Wheel_2"});
		Check(m_Bike != null, "motorbike created in gate A's chamber");
		if (!m_Bike)
		{
			Finish();
			return;
		}
		vector before = m_Bike.GetPosition();
		DZP_GateRegistry.Get().CheckVehicle(m_Bike);
		Check(vector.Distance(m_Bike.GetPosition(), before) < 1, "refused: the Ada is still in gate B's chamber");
		GetGame().ObjectDelete(m_Car);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestBikeJump, 1000, false);
	}

	void TestBikeJump()
	{
		Recheck(m_Bike, m_GateA);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestBikeArrived, 3000, false);
	}

	void TestBikeArrived()
	{
		float d = vector.Distance(m_Bike.GetPosition(), m_GateB.DZP_ChamberPoint(vector.Zero, 0));
		Check(d < 3, "motorbike arrived in gate B's chamber (" + d + " m from its middle)");
		Check(!m_Bike.IsDamageDestroyed(), "motorbike survived the trip (health " + m_Bike.GetHealth() + ")");
		GetGame().ObjectDelete(m_Bike);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestExplosives, 500, false);
	}

	// nothing is refused: the lightning sets off whatever can go off
	void TestExplosives()
	{
		Check(m_GateA.DZP_IsLinked(), "retuned, gate A is alive again");
		PlayerBase bomber = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorM_Mirek", m_GateA.DZP_ChamberPoint(vector.Zero, 0.1), 0, "NONE"));
		if (!bomber)
		{
			Finish();
			return;
		}
		bomber.GetInventory().CreateInInventory("TShirt_Grey");
		EntityAI shirt = bomber.FindAttachmentBySlotName("Body");
		EntityAI grenade = null;
		EntityAI mag = null;
		if (shirt)
		{
			grenade = shirt.GetInventory().CreateInInventory("RGD5Grenade");
			mag = shirt.GetInventory().CreateInInventory("Mag_AKM_30Rnd");
		}
		Check(grenade != null && mag != null, "a survivor with a grenade and a loaded magazine in the chamber");
		Check(DZP_Volatile.Count(bomber) == 2, "both would go off (" + DZP_Volatile.Count(bomber) + ")");
		DZP_GateRegistry.Get().CheckPlayer(bomber);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestExploded, 1500, false, bomber, grenade, mag);
	}

	void TestExploded(PlayerBase bomber, EntityAI grenade, EntityAI mag)
	{
		Check(grenade == null, "the grenade went off in the lightning");
		Check(mag == null, "so did the magazine");
		bool alive = bomber.IsAlive();
		float health = bomber.GetHealth("", "");
		bool hurt = !alive || health < 100;
		Check(hurt, "its carrier was caught in it (alive " + alive + ", health " + health + ")");
		TestCarried("Mag_AKM_30Rnd");
	}

	// one survivor at a time walks into gate A carrying one thing; checked 2 s later
	void TestCarried(string type)
	{
		PlayerBase carrier = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorM_Mirek", m_GateA.DZP_ChamberPoint(vector.Zero, 0.1), 0, "NONE"));
		if (!carrier)
		{
			Finish();
			return;
		}
		carrier.GetInventory().CreateInInventory("TShirt_Grey");
		EntityAI shirt = carrier.FindAttachmentBySlotName("Body");
		EntityAI item = null;
		if (shirt)
			item = shirt.GetInventory().CreateInInventory(type);
		Check(item != null, "a survivor carrying " + type);
		DZP_GateRegistry.Get().CheckPlayer(carrier);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestCarriedAfter, 2000, false, carrier, type);
	}

	void TestCarriedAfter(PlayerBase carrier, string type)
	{
		bool alive = carrier.IsAlive();
		float health = carrier.GetHealth("", "");
		bool atB = vector.Distance(carrier.GetPosition(), m_GateB.DZP_ChamberPoint(vector.Zero, 0)) < 3;
		string how = " (alive " + alive + ", health " + health + ", at gate B " + atB + ")";
		if (type == "Mag_AKM_30Rnd")
		{
			Check(alive && health < 55 && atB, "30 rounds pop, 2 each: hurt badly but through" + how);
			TestCarried("RDG2SmokeGrenade_Black");
		}
		else if (type == "RDG2SmokeGrenade_Black")
		{
			Check(alive && health < 80 && health > 30 && atB, "a smoke grenade hurts moderately, and through" + how);
			TestCarried("AmmoBox_762x39_20Rnd");
		}
		else
		{
			Check(!alive, "a box of rounds goes up and kills" + how);
			Finish();
		}
	}

	void Finish()
	{
		Print("[Portals] done, " + m_Failures + " failures");
		GetGame().RequestExit(0);
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		player.SetPosition(m_GateA.ModelToWorld("2 0 7"));
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
