#include "Capsule2D.h"

Capsule2D::Capsule2D()
    : m_radius(0.f)
    , m_start(Vec2::ZERO)
    , m_end(Vec2::ONE)
    , m_elasticity(0.f)
{
}

Capsule2D::Capsule2D(Vec2 const& start, Vec2 const& end, float const& radius)
    : m_radius(radius)
    , m_start(start)
    , m_end(end)
    , m_elasticity(0.f)
{
    m_circumscribedRadius = (end - start).GetLength() + m_radius;
}

Capsule2D::Capsule2D(Vec2 const& start, Vec2 const& end, float const& radius, float const& elasticity)
    : m_radius(radius)
    , m_start(start)
    , m_end(end)
    , m_elasticity(elasticity)
{
    m_circumscribedRadius = (end - start).GetLength() + m_radius;
}

float Capsule2D::GetCircumscribedRadius() const
{
    return m_circumscribedRadius;
}
