#include "Sphere.h"

Sphere::Sphere()
{
    
}

Sphere::~Sphere()
{
}

Sphere::Sphere(Sphere const& copyFrom)
{
    m_center = copyFrom.m_center;
    m_radius = copyFrom.m_radius;
}

Sphere::Sphere(Vec3 const& center, float radius)
{
    m_center = center;
    m_radius = radius;
}
