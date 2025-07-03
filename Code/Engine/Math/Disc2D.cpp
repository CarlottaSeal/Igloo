#include "Disc2D.h"

Disc2D::Disc2D()
    : m_center(Vec2::ZERO)
    , m_radius(0.f)
    , m_elasticity(0.f)
{
}

Disc2D::Disc2D(const Vec2& _center, float _radius, float _elasticity)
    : m_center(_center)
    , m_radius(_radius)
    , m_elasticity(_elasticity)
{
}

Disc2D::Disc2D(const Vec2& _center, float _radius)
    : m_center(_center)
    , m_radius(_radius)
    , m_elasticity(0.f)
{
}
