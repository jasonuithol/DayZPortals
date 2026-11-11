// Portals
// Test bed for our DayZPortals mod (~/Projects/DayZPortals): an assembled pair of Pereval
// gates on 412.0 MHz, one at Balota airfield where players spawn and one at the north-west
// airfield, plus everything needed to crate, place and assemble two more (and an AKM with
// a loaded magazine to try the interlock with).
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
	protected DZP_Gate m_GateC;
	protected PlayerBase m_Dummy;
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
	protected DZP_Gate m_GateUnbuilt;

	void TourStep()
	{
		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		if (players.Count() == 0)
			return;
		if (!m_GateUnbuilt)
		{
			vector u = m_GateA.ModelToWorld("-14 0 0");
			u[1] = GetGame().SurfaceY(u[0], u[2]);
			m_GateUnbuilt = DZP_Gate.Cast(GetGame().CreateObjectEx("DZP_Gate", u, ECE_PLACE_ON_SURFACE));
			m_GateUnbuilt.SetOrientation(m_GateA.GetOrientation());
		}
		// eye position and point looked at, in gate A's model space
		array<vector> eyes = new array<vector>();
		array<vector> looks = new array<vector>();
		eyes.Insert("2.4 0 10");	looks.Insert(DZP_Layout.RING);
		eyes.Insert("-7 0 8");		looks.Insert(DZP_Layout.RING);
		eyes.Insert("13 0 0");		looks.Insert(DZP_Layout.RING);
		eyes.Insert("2.4 0 -13");	looks.Insert(DZP_Layout.RING);
		eyes.Insert("2.4 0 4");		looks.Insert(DZP_Layout.RING);
		eyes.Insert("-11.6 0 7");	looks.Insert("-11.6 0.5 1.8");
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
			if (vector.Distance(flat, GATE_A) < 600 && !m_GateA && g.DZP_IsBuilt())
				m_GateA = g;
			if (vector.Distance(flat, GATE_B) < 600 && !m_GateB && g.DZP_IsBuilt())
				m_GateB = g;
		}
		Print("[Portals] " + reg.Count() + " gates persisted");
		if (!m_GateA)
			m_GateA = BuiltGate(Clearing(GATE_A), 0);
		if (!m_GateB)
			m_GateB = BuiltGate(Clearing(GATE_B), 0);

		vector loot = m_GateA.ModelToWorld("-2 0 4");
		for (int k = 0; k < 2; k++)
			Drop("DZP_GateKit", loot + Vector(k * 1.5, 0, 0));
		Drop("Hammer", loot + Vector(0, 0, 1.5));
		Drop("MetalPlate", loot + Vector(1, 0, 1.5));
		Drop("MetalPlate", loot + Vector(1.5, 0, 1.5));
		Drop("MetalWire", loot + Vector(2, 0, 1.5));
		Drop("MetalWire", loot + Vector(2.2, 0, 1.5));
		Drop("TruckBattery", loot + Vector(2.5, 0, 1.5));
		Drop("TruckBattery", loot + Vector(3, 0, 1.5));
		Drop("PersonalRadio", loot + Vector(3.5, 0, 1.5));
		Drop("PersonalRadio", loot + Vector(3.7, 0, 1.5));
		Drop("ElectronicRepairKit", loot + Vector(4, 0, 1.5));
		Drop("AKM", loot + Vector(0, 0, 3));
		Drop("Mag_AKM_30Rnd", loot + Vector(0.5, 0, 3));
	}

	// nearest spot to the anchor with nothing at all (trees, bushes, walls) within 12 m,
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
				GetGame().GetObjectsAtPosition(p, 12, objs, null);
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

	DZP_Gate BuiltGate(vector pos, int channel)
	{
		pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
		DZP_Gate gate = DZP_Gate.Cast(GetGame().CreateObjectEx("DZP_Gate", pos, ECE_PLACE_ON_SURFACE));
		if (!gate)
		{
			Print("[Portals] FAIL can't create DZP_Gate");
			m_Failures++;
			return null;
		}
		gate.GetInventory().CreateAttachment("MetalPlate");
		gate.GetInventory().CreateAttachment("MetalWire");
		gate.GetInventory().CreateAttachment("TruckBattery");
		gate.GetInventory().CreateAttachment("PersonalRadio");
		gate.DZP_Assemble();
		gate.DZP_Tune(channel - gate.DZP_GetChannel());
		Print("[Portals] gate at " + gate.GetPosition() + " built " + gate.DZP_IsBuilt() + " on " + gate.DZP_GetFrequencyText() + ", " + gate.GetInventory().AttachmentCount() + " attachments left");
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
		Check(m_GateA && m_GateA.DZP_IsBuilt(), "gate A assembled");
		Check(m_GateB && m_GateB.DZP_IsBuilt(), "gate B assembled");
		Check(m_GateA.DZP_GetChannel() == m_GateB.DZP_GetChannel(), "A and B on the same channel (" + m_GateA.DZP_GetFrequencyText() + ")");
		Check(DZP_Layout.FrequencyText(1) == "412.5 MHz" && DZP_Layout.FrequencyText(19) == "421.5 MHz", "frequency text " + DZP_Layout.FrequencyText(1) + " / " + DZP_Layout.FrequencyText(19));

		// an unbuilt gate refuses to assemble until everything is there
		vector p = m_GateA.ModelToWorld("0 0 40");
		p[1] = GetGame().SurfaceY(p[0], p[2]);
		DZP_Gate c = DZP_Gate.Cast(GetGame().CreateObjectEx("DZP_Gate", p, ECE_PLACE_ON_SURFACE));
		Check(!c.DZP_CanAssemble() && c.DZP_CanDismantle(), "empty gate: can't assemble, can crate");
		c.GetInventory().CreateAttachment("MetalPlate");
		Check(!c.DZP_CanAssemble() && !c.DZP_CanDismantle(), "partial gate: can't assemble or crate, missing '" + c.DZP_MissingText() + "'");
		c.GetInventory().CreateAttachment("MetalWire");
		c.GetInventory().CreateAttachment("TruckBattery");
		c.GetInventory().CreateAttachment("PersonalRadio");
		Check(c.DZP_CanAssemble(), "full gate can assemble");
		c.DZP_Assemble();
		Check(c.DZP_IsBuilt(), "full gate assembled");
		m_GateC = c;

		// a survivor with no one behind it, to walk through the gates
		vector at = m_GateA.ModelToWorld(DZP_Layout.RING + Vector(0, -DZP_Layout.RING[1], 0.4));
		m_Dummy = PlayerBase.Cast(GetGame().CreatePlayer(null, "SurvivorM_Mirek", at, 0, "NONE"));
		Check(m_Dummy != null, "test survivor created");
		if (!m_Dummy)
		{
			Finish();
			return;
		}
		m_Dummy.GetInventory().CreateInInventory("TShirt_Grey");
		EntityAI shirt = m_Dummy.FindAttachmentBySlotName("Body");
		EntityAI mag = null;
		if (shirt)
			mag = shirt.GetInventory().CreateInInventory("Mag_AKM_30Rnd");
		Check(mag != null, "gave the survivor a loaded magazine");
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestContraband, 1000, false, mag);
	}

	void TestContraband(EntityAI mag)
	{
		// used-up stacks are deleted a frame later, so this is checked here
		Check(m_GateC.GetInventory().AttachmentCount() == 0, "assembly used up the materials (" + m_GateC.GetInventory().AttachmentCount() + " left)");
		m_GateC.DZP_Dismantle();

		EntityAI found = DZP_Contraband.Find(m_Dummy);
		Check(found == mag, "interlock finds the magazine (" + found + ")");
		vector before = m_Dummy.GetPosition();
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy, GetGame().GetTickTime());
		Check(vector.Distance(m_Dummy.GetPosition(), before) < 1, "refused: survivor still at gate A");

		GetGame().ObjectDelete(mag);
		m_Dummy.m_DZP_NextGateTime = 0;
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestJump, 1000, false);
	}

	void TestJump()
	{
		Check(DZP_Contraband.Find(m_Dummy) == null, "nothing forbidden left");
		Print("[Portals] survivor at " + m_Dummy.GetPosition() + ", gate A " + m_GateA.GetPosition());
		DZP_GateRegistry.Get().CheckPlayer(m_Dummy, GetGame().GetTickTime());
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TestArrived, 2000, false);
	}

	void TestArrived()
	{
		vector arrival = m_GateB.ModelToWorld(DZP_Layout.RING + Vector(0, 0, DZP_Layout.ARRIVAL_DISTANCE));
		float d = vector.Distance(m_Dummy.GetPosition(), arrival);
		Check(d < 3, "survivor arrived at gate B (" + d + " m from the arrival point, at " + m_Dummy.GetPosition() + ")");
		Finish();
	}

	void Finish()
	{
		Print("[Portals] done, " + m_Failures + " failures");
		GetGame().RequestExit(0);
	}

	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		player.SetPosition(m_GateA.ModelToWorld("-2 0 7"));
	}
};

Mission CreateCustomMission(string path)
{
	return new CustomMission();
}
