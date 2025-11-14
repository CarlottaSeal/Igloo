#pragma once
#include <atomic>
#include <vector>

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Scene/BVH.h"

struct Vec3;
struct Vertex_PCUTBN;

class SDFGenerator
{
    friend class Scene;
public:
    SDFGenerator();
    ~SDFGenerator();
    
    std::vector<float>& GetSDFData() { return m_sdfData; }
    int GetResolution() const { return m_resolution; }  
    const AABB3& GetBounds() const { return m_bounds; }

    void SetResolution(int resolution) { m_resolution = resolution; }
    void ResizeSDFData(size_t size) { m_sdfData.resize(size); }
    void SetBoundsFromVerts( std::vector<Vertex_PCUTBN>& vertices) { m_bounds = GetVertexBounds3D(vertices); }
    
    //CPU端采样
    float Sample(const Vec3& localPos) const;
    Vec3 ComputeGradient(const Vec3& localPos) const;
    
    float GetVoxel(int x, int y, int z) const;

protected:
    int m_resolution;
    AABB3 m_bounds;
    std::vector<float> m_sdfData;

    BVH m_bvh;
    std::atomic<float> m_progress{0.0f}; 
};
