#pragma once
#include <cstdint>
#include <d3d12.h>

#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/Mat44.hpp"

class SDFGenerator;

struct SDFInstance
{
    //SDFGenerator* m_sdf;
    const std::vector<float>* m_data = nullptr;
    int m_resolution = 0;
    AABB3 m_bounds;
    
    Mat44 m_worldTransform;
    Mat44 m_inverseTransform;
    
    float Sample(const Vec3& localPos) const
    {
        if (!m_data || m_resolution == 0)
            return FLT_MAX;
    
        Vec3 size = m_bounds.GetBoundsSize();
        Vec3 normalized;
        normalized.x = (localPos.x - m_bounds.m_mins.x) / size.x;
        normalized.y = (localPos.y - m_bounds.m_mins.y) / size.y;
        normalized.z = (localPos.z - m_bounds.m_mins.z) / size.z;
    
        // 边界检查
        if (normalized.x < 0.0f || normalized.x > 1.0f ||
            normalized.y < 0.0f || normalized.y > 1.0f ||
            normalized.z < 0.0f || normalized.z > 1.0f)
        {
            return FLT_MAX;
        }
    
        // SDFGenerator::Sample
        float x = normalized.x * (m_resolution - 1);
        float y = normalized.y * (m_resolution - 1);
        float z = normalized.z * (m_resolution - 1);
    
        int x0 = (int)floorf(x), x1 = MinI(x0 + 1, m_resolution - 1);
        int y0 = (int)floorf(y), y1 = MinI(y0 + 1, m_resolution - 1);
        int z0 = (int)floorf(z), z1 = MinI(z0 + 1, m_resolution - 1);
    
        float fx = x - x0, fy = y - y0, fz = z - z0;
    
        auto GetVoxel = [this](int x, int y, int z) -> float {
            int index = x + y * m_resolution + z * m_resolution * m_resolution;
            return (*m_data)[index];
        };
    
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
};

//struct SDFReference
//{
//    uint32_t m_srvIndex;
//    Mat44 m_worldTransform;
//};
//
//struct SDFResource
//{
//    ID3D12Resource* m_sdfTexture3D = nullptr;
//    D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle = {};
//    AABB3 m_bounds;
//    int m_resolution;
//    uint32_t m_srvHeapIndex;  // 在 descriptor heap 中的索引
//    ID3D12Resource* m_sdfUploadBuffer = nullptr;
//};
