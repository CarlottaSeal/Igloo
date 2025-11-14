#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/RenderCommon.h"

#include <d3d12.h>
#include <vector>
#include <cstdint>

enum RadianceCacheType
{
    RADIANCE_CACHE_NEAR,
    RADIANCE_CACHE_NORMAL,
    RADIANCE_CACHE_FAR,
    RADIANCE_CACHE_COUNT
};
struct RadianceProbeGPU
{
    float WorldPositionX;
    float WorldPositionY;
    float WorldPositionZ;
    float Pad0;
    
    // 2阶球谐系数 (9 coefficients × 3 channels = 27 floats)
    float SH_R[9];
    float SH_G[9];
    float SH_B[9];
    
    uint32_t LastUpdateFrame;
    float Validity;           // 0-1, 表示这个 Probe 的可信度
    float Weight;             // 用于混合
    float Pad1;
};

struct RadianceProbe
{
    uint32_t m_probeIndex;
    Vec3 m_worldPosition;
    Vec2 m_screenPosition;
    
    uint32_t m_lastUpdateFrame;
    uint32_t m_birthFrame;
    float m_priority;
    bool m_isDirty;
    bool m_isActive;
    
    AABB3 m_influenceBox;
    float m_influenceRadius;
    
    RadianceProbe()
        : m_probeIndex(0)
        , m_worldPosition(0, 0, 0)
        , m_screenPosition(0, 0)
        , m_lastUpdateFrame(0)
        , m_birthFrame(0)
        , m_priority(0.0f)
        , m_isDirty(true)
        , m_isActive(false)
        , m_influenceBox(Vec3(0,0,0), Vec3(0,0,0))
        , m_influenceRadius(5.0f)
    {}
};

class RadianceCache
{
    friend class DX12Renderer;
public:
    RadianceCache() = default;
    ~RadianceCache() = default;
    
    void Initialize(ID3D12Device* device, 
                   uint32_t maxProbes = 4096,
                   uint32_t raysPerProbe = 32);
    
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    void SwapBuffers();
    
    void BuildUpdateQueue(uint32_t maxProbesThisFrame);
    void UploadProbeData();
    void UploadConstants(const RadianceCacheConstants& constants);
    
    ID3D12Resource* GetCurrentProbeBuffer() const;
    ID3D12Resource* GetPreviousProbeBuffer() const;
    ID3D12Resource* GetUpdateListBuffer() const;
    
    uint32_t AllocateProbe(const Vec3& worldPos, const Vec2& screenPos);
    void FreeProbe(uint32_t probeIndex);
    RadianceProbe* GetProbe(uint32_t probeIndex);
    const RadianceProbe* GetProbe(uint32_t probeIndex) const;
    
    std::vector<uint32_t>& GetUpdateList() { return m_updateList; }
    const std::vector<uint32_t>& GetUpdateList() const { return m_updateList; }
    
    uint32_t GetMaxProbes() const { return m_maxProbes; }
    uint32_t GetActiveProbeCount() const { return m_activeProbeCount; }
    uint32_t GetUpdateProbeCount() const { return m_updateProbeCount; }
    uint32_t GetRaysPerProbe() const { return m_raysPerProbe; }
    int GetCurrentBufferIndex() const { return m_currentIndex; }
    
    void ReadbackProbeData(ID3D12GraphicsCommandList* cmdList, 
                          std::vector<RadianceProbeGPU>& outProbes);
    
protected:
    void CreateProbeBuffers(ID3D12Device* device);
    void CreateUpdateListBuffer(ID3D12Device* device);
    void CreateConstantBuffer(ID3D12Device* device);
    void CreateReadbackBuffer(ID3D12Device* device);
    //void CreateDescriptors(ID3D12Device* device);
    
    // GPU 资源
    ID3D12Resource* m_probeBuffer[RADIANCE_CACHE_BUFFER_COUNT];      // 双缓冲
    ID3D12Resource* m_updateListBuffer;                              // 本帧要更新的 Probe 索引
    ID3D12Resource* m_readbackBuffer;                                // 用于调试
    
    std::vector<RadianceProbe> m_probes;                             // CPU 端 Probe 列表
    std::vector<uint32_t> m_freeIndices;                             // 空闲索引池
    std::vector<uint32_t> m_updateList;                              // 本帧要更新的 Probe 索引
    
    uint32_t m_maxProbes;
    uint32_t m_activeProbeCount;
    uint32_t m_updateProbeCount;
    uint32_t m_raysPerProbe;
    
    int m_currentIndex;
    bool m_initialized;
};