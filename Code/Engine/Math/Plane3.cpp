#include "Plane3.h"

#include "MathUtils.hpp"

Plane3::Plane3()
{
}

Plane3::Plane3(Vec3 const& normal, float distToPlaneAloneNormalFromOrigin)
    : m_normal(normal)
    , m_distToPlaneAloneNormalFromOrigin(distToPlaneAloneNormalFromOrigin)
{
}

bool Plane3::IsPointInFrontOfPlane(Vec3 const& referencePos) const
{
    Vec3 planePos = m_normal * m_distToPlaneAloneNormalFromOrigin;
    Vec3 toward = referencePos - planePos;
    if (DotProduct3D(toward, m_normal) > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

Plane3 Plane3::FromThreePoints(const Vec3& p1, const Vec3& p2, const Vec3& p3)
{
	Vec3 v1 = p2 - p1;
	Vec3 v2 = p3 - p1;
	Vec3 normal = CrossProduct3D(v1, v2).GetNormalized();
	float distance = DotProduct3D(normal, p1);

	return Plane3(normal, distance);
}

float Plane3::GetDistanceToPoint(const Vec3& point) const
{
	return DotProduct3D(m_normal, point) - m_distToPlaneAloneNormalFromOrigin;
}

void Plane3::Normalize()
{
	float length = m_normal.GetLength();
	if (length > 0.0f)
	{
		m_normal /= length;
		m_distToPlaneAloneNormalFromOrigin /= length;
	}
}
