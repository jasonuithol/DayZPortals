// The gate's scenery: vanilla static objects created locally (not networked, not persisted)
// on the server, for collision, and on every client, for looks.
class DZP_Decor
{
	protected ref array<Object> m_Objects = new array<Object>();

	// the pieces go in world space once, solid (the gate never moves)
	void Spawn(Object owner, array<ref DZP_DecorPart> parts)
	{
		Clear();
		vector ownerOri = owner.GetOrientation();
		int flags = ECE_LOCAL | ECE_KEEPHEIGHT | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD | ECE_CREATEPHYSICS;
		for (int i = 0; i < parts.Count(); i++)
		{
			DZP_DecorPart part = parts[i];
			Object obj;
			// a bare model (no class): placed the way the DayZ Editor places it, so the layout
			// matches what was built there
			if (part.m_Type.Contains(".p3d"))
				obj = GetGame().CreateStaticObjectUsingP3D(part.m_Type, owner.ModelToWorld(part.m_Pos), ownerOri + part.m_Ori, part.m_Scale, true);
			else
				obj = GetGame().CreateObjectEx(part.m_Type, owner.ModelToWorld(part.m_Pos), flags);
			if (!obj)
			{
				Print("[DayZPortals] can't create decor " + part.m_Type);
				continue;
			}
			obj.SetOrientation(ownerOri + part.m_Ori);
			obj.SetPosition(owner.ModelToWorld(part.m_Pos));
			if (part.m_Scale != 1)
				obj.SetScale(part.m_Scale);
			m_Objects.Insert(obj);
		}
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
