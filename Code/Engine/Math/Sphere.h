#pragma once
#include "Vec3.hpp"

struct Sphere
{
public:
    Vec3 m_center;
    float m_radius;

public:
    Sphere();
    ~Sphere();

    Sphere(Sphere const& copyFrom);
    explicit Sphere(Vec3 const& center, float radius);
};
