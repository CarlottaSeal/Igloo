#pragma once
#include "Vec2.hpp"

struct Capsule2D
{
public:
    Vec2 m_start;
    Vec2 m_end;
    float m_radius = 0.f;
    
    float m_elasticity = 0.f;
    float m_circumscribedRadius = 0.f;

public:
    Capsule2D();
    Capsule2D(Vec2 const& start, Vec2 const& end, float const& radius);
    Capsule2D(Vec2 const& start, Vec2 const& end, float const& radius, float const& elasticity);
    float GetCircumscribedRadius() const;
};
