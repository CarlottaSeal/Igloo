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
