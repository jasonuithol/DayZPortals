// Client: every frame, finds the nearest assembled gate to the camera and shimmers the air
// around its grille (DZP_PPERequester_GateShimmer), stronger the closer you are.
class DZP_GateShimmer
{
	static const float RANGE = 12.0;		// no shimmer beyond this many metres from the grille
	static const float FULL_RANGE = 2.0;	// full strength inside this
	static const float MAX_DISTORT = 0.12;
	static const float MAX_CHROM_ABB = 0.35;

	protected static ref DZP_GateShimmer s_Instance;
	protected static ref array<DZP_Gate> s_Gates = new array<DZP_Gate>();

	protected float m_Time;
	protected bool m_Active;

	static void Register(DZP_Gate gate)
	{
		if (s_Gates.Find(gate) < 0)
			s_Gates.Insert(gate);
		if (!s_Instance)
		{
			s_Instance = new DZP_GateShimmer();
			GetGame().GetUpdateQueue(CALL_CATEGORY_GAMEPLAY).Insert(s_Instance.Update);
		}
	}

	static void Unregister(DZP_Gate gate)
	{
		s_Gates.RemoveItem(gate);
	}

	protected DZP_PPERequester_GateShimmer Requester()
	{
		return DZP_PPERequester_GateShimmer.Cast(PPERequesterBank.GetRequester(DZP_PPERequester_GateShimmer));
	}

	void Update(float timeslice)
	{
		m_Time += timeslice;

		vector cam = GetGame().GetCurrentCameraPosition();
		float best = RANGE;
		vector ring;
		for (int i = 0; i < s_Gates.Count(); i++)
		{
			DZP_Gate gate = s_Gates[i];
			if (!gate || !gate.DZP_IsBuilt())
				continue;
			vector centre = gate.DZP_RingCentre();
			float d = vector.Distance(cam, centre);
			if (d < best)
			{
				best = d;
				ring = centre;
			}
		}

		float strength = Math.Clamp((RANGE - best) / (RANGE - FULL_RANGE), 0, 1);
		strength = strength * strength;
		DZP_PPERequester_GateShimmer req = Requester();
		if (!req)
			return;
		if (strength <= 0)
		{
			if (m_Active)
			{
				req.Stop();
				m_Active = false;
			}
			return;
		}

		// lens centred on the grille while it's on screen; behind you it's just a faint wobble
		float cx = 0;
		float cy = 0;
		vector screen = GetGame().GetScreenPosRelative(ring);
		if (screen[2] > 0)
		{
			cx = Math.Clamp(screen[0] * 2 - 1, -1, 1);
			cy = Math.Clamp(screen[1] * 2 - 1, -1, 1);
		}
		else
		{
			strength *= 0.3;
		}

		// two out-of-step waves so it ripples rather than pulses
		float wobble = Math.Sin(m_Time * 9.0) * 0.6 + Math.Sin(m_Time * 23.0 + 1.3) * 0.4;
		float chrom = 0.5 + 0.5 * Math.Sin(m_Time * 5.0);
		if (!m_Active)
		{
			req.Start();
			m_Active = true;
		}
		req.SetShimmer(strength * MAX_DISTORT * wobble, strength * MAX_CHROM_ABB * chrom, cx, cy);
	}
}
