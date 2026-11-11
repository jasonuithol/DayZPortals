// Where the vanilla pieces of the gate go, relative to the control cabinet (DZP_Gate's own
// model, origin on the floor). The ring is the plate players touch to jump; its local
// position also defines the trigger box and the arrival point.
class DZP_DecorPart
{
	string m_Type;
	vector m_Pos;
	vector m_Ori;	// yaw pitch roll, degrees, relative to the gate
	float m_Scale;

	void DZP_DecorPart(string type, vector pos, vector ori, float scale)
	{
		m_Type = type;
		m_Pos = pos;
		m_Ori = ori;
		m_Scale = scale;
	}
}

class DZP_Layout
{
	static const vector RING = "2.4 1.55 0";
	static const float RING_SCALE = 1.35;		// the hatch is 2.2 m across; this makes it ~3 m
	static const float TRIGGER_HALF_WIDTH = 1.1;
	static const float TRIGGER_DEPTH = 0.9;		// either side of the plate
	static const float TRIGGER_HEIGHT = 2.8;
	static const float ARRIVAL_DISTANCE = 2.6;	// in front (+z) of the plate at the far gate

	static const int CHANNELS = 20;
	static const int FIRST_MHZ_X2 = 824;		// channel 0 = 412.0 MHz, 0.5 MHz steps

	static const int PLATES_NEEDED = 10;
	static const int WIRE_NEEDED = 1;

	// Built: the hatch ring (a wire grille) on a Livonia bunker blast-door leaf, standing
	// between two equipment racks, a mast and a transformer behind, a cable drum beside the
	// cabinet.
	static void Built(array<ref DZP_DecorPart> parts)
	{
		parts.Insert(new DZP_DecorPart("StaticObj_Underground_Corridor_Main_Gate_L", Vector(RING[0], 0, RING[2] - 0.3), "0 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Misc_ConcretePipe_Gate", RING, "0 0 0", RING_SCALE));
		parts.Insert(new DZP_DecorPart("StaticObj_Furniture_radar_rack_quad", Vector(RING[0] - 1.9, 0, -0.4), "90 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Furniture_radar_rack_quad", Vector(RING[0] + 1.9, 0, 0.4), "-90 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Misc_Antenna", Vector(RING[0], 4.57, -2.6), "0 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Misc_Transformer", Vector(RING[0], 1.45, -5.6), "90 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Misc_Coil", "-2.4 1.1 -1.2", "0 0 0", 1));
	}

	// Placed but not assembled: the blast door lies flat beside the cabinet.
	static void Unbuilt(array<ref DZP_DecorPart> parts)
	{
		parts.Insert(new DZP_DecorPart("StaticObj_Underground_Corridor_Main_Gate_L", Vector(RING[0], 0.2, 0.5), "0 -90 0", 1));
	}

	static string FrequencyText(int channel)
	{
		int x2 = FIRST_MHZ_X2 + channel;
		int whole = x2 / 2;
		if (x2 - whole * 2 == 1)
			return whole.ToString() + ".5 MHz";
		return whole.ToString() + ".0 MHz";
	}
}
