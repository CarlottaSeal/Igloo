#include "SDFGenerator.h"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"

extern Renderer* g_theRenderer;

SDFGenerator::SDFGenerator()
{
}

SDFGenerator::~SDFGenerator()
{
}

float SDFGenerator::Sample(const Vec3& localPos) const
{
    Vec3 size = m_bounds.GetBoundsSize();
    Vec3 normalized;
    normalized.x = (localPos.x - m_bounds.m_mins.x) / size.x;
    normalized.y = (localPos.y - m_bounds.m_mins.y) / size.y;
    normalized.z = (localPos.z - m_bounds.m_mins.z) / size.z;
    
    if (normalized.x < 0.0f || normalized.x > 1.0f ||
        normalized.y < 0.0f || normalized.y > 1.0f ||
        normalized.z < 0.0f || normalized.z > 1.0f)
    {
        return FLT_MAX;
    }
    
    // 三线性插值
    float x = normalized.x * (m_resolution - 1);
    float y = normalized.y * (m_resolution - 1);
    float z = normalized.z * (m_resolution - 1);
    
    int x0 = (int)floorf(x), x1 = MinI(x0 + 1, m_resolution - 1);
    int y0 = (int)floorf(y), y1 = MinI(y0 + 1, m_resolution - 1);
    int z0 = (int)floorf(z), z1 = MinI(z0 + 1, m_resolution - 1);
    
    float fx = x - x0, fy = y - y0, fz = z - z0;
    
    float v000 = GetVoxel(x0, y0, z0);
    float v001 = GetVoxel(x0, y0, z1);
    float v010 = GetVoxel(x0, y1, z0);
    float v011 = GetVoxel(x0, y1, z1);
    float v100 = GetVoxel(x1, y0, z0);
    float v101 = GetVoxel(x1, y0, z1);
    float v110 = GetVoxel(x1, y1, z0);
    float v111 = GetVoxel(x1, y1, z1);
    
    return Interpolate(
        Interpolate(Interpolate(v000, v100, fx), Interpolate(v010, v110, fx), fy),
        Interpolate(Interpolate(v001, v101, fx), Interpolate(v011, v111, fx), fy),
        fz
    );
}

Vec3 SDFGenerator::ComputeGradient(const Vec3& localPos) const
{
    const float epsilon = 0.001f;
        
    float dx = Sample(localPos + Vec3(epsilon, 0, 0)) - Sample(localPos - Vec3(epsilon, 0, 0));
    float dy = Sample(localPos + Vec3(0, epsilon, 0)) - Sample(localPos - Vec3(0, epsilon, 0));
    float dz = Sample(localPos + Vec3(0, 0, epsilon)) - Sample(localPos - Vec3(0, 0, epsilon));
        
    return Vec3(dx, dy, dz).GetNormalized();
}

float SDFGenerator::GetVoxel(int x, int y, int z) const
{
    int index = x + y * m_resolution + z * m_resolution * m_resolution;
    return m_sdfData[index];
}
