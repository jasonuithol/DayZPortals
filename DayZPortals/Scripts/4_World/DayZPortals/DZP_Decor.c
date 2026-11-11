// The gate's scenery: vanilla static objects created locally (not networked, not persisted)
// on the server, for collision, and on every client, for looks.
class DZP_Decor
{
	protected ref array<Object> m_Objects = new array<Object>();

	// attach = parent the pieces to the owner so they follow it, without collision (the
	// placement hologram, whose placement raycasts must not hit them); otherwise they are
	// put in world space once, solid (the gate never moves).
	void Spawn(Object owner, array<ref DZP_DecorPart> parts, bool attach)
	{
		Clear();
		vector ownerOri = owner.GetOrientation();
		int flags = ECE_LOCAL | ECE_KEEPHEIGHT | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD;
		if (!attach)
			flags |= ECE_CREATEPHYSICS;
		for (int i = 0; i < parts.Count(); i++)
		{
			DZP_DecorPart part = parts[i];
			Object obj = GetGame().CreateObjectEx(part.m_Type, owner.ModelToWorld(part.m_Pos), flags);
			if (!obj)
			{
				Print("[DayZPortals] can't create decor " + part.m_Type);
				continue;
			}
			if (attach)
			{
				obj.SetOrientation(part.m_Ori);
				obj.SetPosition(part.m_Pos);
				owner.AddChild(obj, -1);
			}
			else
			{
				obj.SetOrientation(ownerOri + part.m_Ori);
				obj.SetPosition(owner.ModelToWorld(part.m_Pos));
			}
			if (part.m_Scale != 1)
				obj.SetScale(part.m_Scale);
			m_Objects.Insert(obj);
		}
		if (attach)
			owner.Update();
	}

	void Clear()
	{
		for (int i = 0; i < m_Objects.Count(); i++)
		{
			if (m_Objects[i])
				GetGame().ObjectDelete(m_Objects[i]);
		}
		m_Objects.Clear();
	}

	bool IsEmpty()
	{
		return m_Objects.Count() == 0;
	}
}
