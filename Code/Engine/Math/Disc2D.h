#pragma once
#include "Vec2.hpp"

struct Disc2D
{
public:
    Vec2 m_center;
    float m_radius;
    float m_elasticity = 0.f;
public:
    Disc2D();
    Disc2D(const Vec2& _center, float _radius, float _elasticity);
    Disc2D(const Vec2& _center, float _radius);
};
