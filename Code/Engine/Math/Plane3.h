#pragma once
#include "Vec3.hpp"

struct Plane3
{
public:
    Vec3 m_normal;
    float m_distToPlaneAloneNormalFromOrigin;

public:
    Plane3();
    Plane3(Vec3 const& normal, float distToPlaneAloneNormalFromOrigin);

    bool IsPointInFrontOfPlane(Vec3 const& referencePos) const;
};
