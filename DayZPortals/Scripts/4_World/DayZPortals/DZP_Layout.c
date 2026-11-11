// Where the vanilla pieces of the installation go, relative to the radio panel (DZP_Gate's own
// model). Installation() is generated from a DayZ Editor save by tools/layout.py; the chamber
// in the middle of the gantries is where players stand to jump, and where they arrive.
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
	static const vector CHAMBER = "-6.342 0 1.384";		// the jump spot, on the chamber floor (y: see FIELD_*)
	static const float FIELD_RADIUS = 1.5;		// standing this close to CHAMBER (horizontally) jumps
	static const float FIELD_BELOW = 1.5;		// feet this far below the panel's floor level...
	static const float FIELD_ABOVE = 3.0;		// ...to this far above it still count (uneven ground)
	static const float EFFECT_HEIGHT = 1.4;		// burst, lightning flash and shimmer: this high over the floor
	static const float FIELD_RADIUS_VEHICLE = 2.5;	// a vehicle's middle this close jumps it, crew and all
	static const float VEHICLE_DROP = 0.5;		// vehicles arrive this high and settle onto the floor
	static const float WALKER_RING = 0.9;		// a party on foot arrives spread round the middle...
	static const float WALKER_RING_VEHICLE = 2.9;	// ...or round the vehicle that came with them

	static const int CHANNELS = 20;
	static const int FIRST_MHZ_X2 = 824;		// channel 0 = 412.0 MHz, 0.5 MHz steps

	// The installation as laid out in the editor (tools/layout.py import).
	static void Installation(array<ref DZP_DecorPart> parts)
	{
		parts.Insert(new DZP_DecorPart("StaticObj_Misc_DeconShower_Large", "-4.88 3.204 -2.735", "158.32 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Misc_DeconShower_Large", "-10.669 3.153 1.91", "-79.103 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Wall_Fen4_4", "-8.372 1.426 6.818", "-25.353 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Wall_Fen4_4", "-0.846 1.495 0.656", "91.647 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Wall_Fen4_4", "-10.126 1.344 -3.202", "-142.353 0 0", 1));
		parts.Insert(new DZP_DecorPart("dz\\structures\\industrial\\tanks\\tank_small_gas.p3d", "-11.222 0.225 -4.787", "179.155 0 0", 1));
		parts.Insert(new DZP_DecorPart("dz\\structures\\industrial\\power\\power_transformer_small.p3d", "-9.364 0.904 8.114", "-175.63 0 0", 1));
		parts.Insert(new DZP_DecorPart("StaticObj_Misc_DeconShower_Large", "-3.478 3.266 4.977", "-142.353 0 0", 1));
		parts.Insert(new DZP_DecorPart("DZP_DangerSign", "-0.711 2.345 0.653", "91.647 0 0", 1));
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
