#pragma once

#include "RadianceCache.h"
#include "Engine/Math/IntVec3.h"
#include <unordered_map>
#include <queue>

class Scene;
class Camera;
struct GBufferData;

// ========== 空间 Hash Grid ==========
struct ProbeHashGrid
{
    float m_cellSize;
    std::unordered_map<IntVec3, std::vector<uint32_t>> m_grid;
    
    IntVec3 WorldToCell(const Vec3& worldPos) const;
    void Insert(uint32_t probeIndex, const Vec3& worldPos);
    void Remove(uint32_t probeIndex, const Vec3& worldPos);
    std::vector<uint32_t> Query(const Vec3& worldPos, float radius) const;
    void Clear();
};

class RadianceCacheManager
{
public:
    RadianceCacheManager() = default;
    ~RadianceCacheManager() = default;
    
    void Initialize(RadianceCache* cache, Scene* scene);
    
    // 每帧调用
    void PlaceProbesScreenSpace(const Camera& camera,
                                const GBufferData& gbuffer);
    Vec3 ReconstructWorldPosition(const Vec2& ndc, float depth, const Mat44& viewProjInv);

    void BuildPriorityQueue(const Camera& camera, uint32_t currentFrame);
    
    void RecycleFarProbes(const Vec3& cameraPos, float maxDistance);
    
    // 查询
    std::vector<uint32_t> FindNearbyProbes(const Vec3& worldPos, uint32_t maxCount) const;

private:
    float CalculateProbePriority(const RadianceProbe* probe, const Vec3& cameraPos, uint32_t currentFrame) const;
    bool ShouldPlaceProbe(const Vec3& worldPos) const;
    
    RadianceCache* m_cache;
    Scene* m_scene;
    ProbeHashGrid m_spatialGrid;
    
    // 配置
    float m_minProbeSpacing;
    float m_maxProbeDistance;
    uint32_t m_probesPerFrame;
};