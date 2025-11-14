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

	static Plane3 FromThreePoints(const Vec3& p1, const Vec3& p2, const Vec3& p3);

	float GetDistanceToPoint(const Vec3& point) const;

	void Normalize();

	Vec3 GetNormal() const { return m_normal; }
};
