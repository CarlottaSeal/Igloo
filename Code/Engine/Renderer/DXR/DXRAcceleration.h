#pragma once
#include <cstdint>
#include <d3d12.h>
#include <vector>

#include "Engine/Math/Mat44.hpp"

struct Instance
{
    ID3D12Resource* m_vertexBuffer;
    ID3D12Resource* m_indexBuffer;
    uint32_t vertexCount;
    uint32_t indexCount;
    Mat44 transform;
};

class DXRAcceleration
{
public:
    DXRAcceleration() = default;
    ~DXRAcceleration() = default;

    void Shutdown();
    bool Initialize(ID3D12Device5* device);
    void PrepareBLAS(const Instance& instance);
    void PrepareTLAS( const std::vector<Instance>& instances);
    
    ID3D12Resource* GetTLAS() const { return m_tlasBuffer; }
    
private:
    ID3D12Device5* m_device = nullptr;
    
    // Bottom Level Acceleration Structures (per mesh)
    std::vector<ID3D12Resource*> m_blasBuffers;
    
    // Top Level Acceleration Structure (scene)
    ID3D12Resource* m_tlasBuffer = nullptr;
    ID3D12Resource* m_tlasInstanceDesc = nullptr;
    ID3D12Resource* m_scratchBuffer = nullptr;
};

