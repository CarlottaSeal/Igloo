#pragma once
#include "Plane3.h"
#include "Mat44.hpp"
#include <array>

class Camera;
struct AABB3;

enum class ContainmentType
{
	OUTSIDE,
	INSIDE,
	INTERSECTS
};

struct Frustum
{
public:
    enum{ Left = 0, Right = 1, Top = 2, Bottom = 3, Near = 4, Far = 5 };
    std::array<Plane3, 6> m_planes;
    Camera* m_camera = nullptr;

public:
    Frustum() = default;
    Frustum(const Frustum&) = default;
    
    Frustum(Plane3 const& left, Plane3 const& right, Plane3 const& bottom, Plane3 const& top,
            Plane3 const& nearP, Plane3 const& farP, Camera* const& camera);

    static Frustum FromCorners(Vec3 const corners[8], Camera* const& camera);
    static Frustum FromViewProjectionMatrix(const Mat44& viewProjectionMatrix, Camera* const& camera);

    bool IsPointInside(const Vec3& point) const;
    bool IsAABBOutside(const AABB3& aabb) const;
    ContainmentType DetectContainmentWithAABB(const AABB3& aabb) const;
    ContainmentType DetectContainmentWithSphere(const Vec3& center, float radius) const;

    void NormalizePlanes();
};
