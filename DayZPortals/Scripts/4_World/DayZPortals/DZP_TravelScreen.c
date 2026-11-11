// Client: for a few seconds while travelling, the world is gone - only deep space, the stars
// rushing past. A full-screen overlay (gui/dzp_travel.layout): a far starfield with the Milky Way
// zooming in slowly and a near one zooming fast, both rolling, faded in and out. Pictures by
// tools/starfield.py. Nothing about the player's camera or input is touched: a script camera
// (staticcamera) and HumanInputController.SetDisabled both left the client stuck after the trip.
class DZP_TravelScreen
{
	static const int RPC_TRAVEL = 830047;		// "Object 47"
	static const float DURATION = 5.0;		// seconds in transit
	static const float FADE_IN = 0.25;
	static const float FADE_OUT = 0.6;
	static const float SIZE = 1.7;			// the layers' size at the start (bigger than the screen: they roll)
	static const float FAR_ZOOM = 0.35;		// how much bigger the far layer gets by the end...
	static const float NEAR_ZOOM = 3.0;		// ...and the near one: the stars rushing past
	static const float ROLL = 7.0;			// degrees a second

	protected static ref DZP_TravelScreen s_Active;

	protected Widget m_Root;
	protected ImageWidget m_Far;
	protected ImageWidget m_Near;
	protected float m_Time;
	protected float m_Turn;			// +1 or -1: which way this trip rolls

	static void Show()
	{
		if (s_Active)
			return;
		s_Active = new DZP_TravelScreen();
		s_Active.Begin();
	}

	protected void Begin()
	{
		m_Turn = 1;
		if (Math.RandomInt(0, 2) == 0)
			m_Turn = -1;
		m_Root = GetGame().GetWorkspace().CreateWidgets("DayZPortals/gui/dzp_travel.layout");
		if (!m_Root)
		{
			s_Active = null;
			return;
		}
		m_Far = ImageWidget.Cast(m_Root.FindAnyWidget("Far"));
		m_Near = ImageWidget.Cast(m_Root.FindAnyWidget("Near"));
		m_Far.LoadImageFile(0, "DayZPortals/data/stars_far_co.paa");
		m_Far.SetImage(0);
		m_Near.LoadImageFile(0, "DayZPortals/data/stars_near_ca.paa");
		m_Near.SetImage(0);
		Update(0);
		GetGame().GetUpdateQueue(CALL_CATEGORY_GAMEPLAY).Insert(Update);
	}

	protected void Update(float timeslice)
	{
		m_Time += timeslice;
		if (m_Time >= DURATION)
		{
			End();
			return;
		}
		float t = m_Time / DURATION;
		float far = SIZE * (1 + FAR_ZOOM * t);
		float near = SIZE * (1 + NEAR_ZOOM * t * t);
		m_Far.SetSize(far, far);
		m_Near.SetSize(near, near);
		m_Far.SetRotation(0, 0, m_Turn * ROLL * m_Time);
		m_Near.SetRotation(0, 0, m_Turn * ROLL * 1.6 * m_Time);
		float alpha = Math.Min(1, Math.Min(m_Time / FADE_IN, (DURATION - m_Time) / FADE_OUT));
		m_Root.SetAlpha(alpha);
	}

	protected void End()
	{
		GetGame().GetUpdateQueue(CALL_CATEGORY_GAMEPLAY).Remove(Update);
		if (m_Root)
			m_Root.Unlink();
		s_Active = null;
	}
}
