#include "RadianceCache.h"

#include <algorithm>

#include "Engine/Core/EngineCommon.hpp"
#include "ThirdParty/d3dx12/d3dx12.h"

void RadianceCache::Initialize(ID3D12Device* device, uint32_t maxProbes, uint32_t raysPerProbe)
{
    if (m_initialized)
        return;
    
    m_maxProbes = maxProbes;
    m_raysPerProbe = raysPerProbe;
    m_activeProbeCount = 0;
    m_updateProbeCount = 0;
    
    // 初始化 CPU 端数据
    m_probes.resize(maxProbes);
    m_freeIndices.reserve(maxProbes);
    for (uint32_t i = 0; i < maxProbes; i++)
    {
        m_probes[i].m_probeIndex = i;
        m_probes[i].m_isActive = false;
        m_freeIndices.push_back(i);
    }
    
    m_updateList.reserve(256);  // 预留空间
    
    // 创建 GPU 资源
    CreateProbeBuffers(device);
    CreateUpdateListBuffer(device);
    //CreateConstantBuffer(device);
    CreateReadbackBuffer(device);
    //CreateDescriptors(device);
    
    m_currentIndex = 0;
    m_initialized = true;
    
    DebuggerPrintf("[RadianceCache] Initialized with %u max probes, %u rays per probe\n", 
                   maxProbes, raysPerProbe);
}

void RadianceCache::Shutdown()
{
    if (!m_initialized)
        return;
    
    // 释放资源
    for (int i = 0; i < RADIANCE_CACHE_BUFFER_COUNT; i++)
    {
        DX_SAFE_RELEASE(m_probeBuffer[i]);
    }
    DX_SAFE_RELEASE(m_updateListBuffer);
    //DX_SAFE_RELEASE(m_constantBuffer);
    DX_SAFE_RELEASE(m_readbackBuffer);
    
    m_probes.clear();
    m_freeIndices.clear();
    m_updateList.clear();
    
    m_initialized = false;
}

void RadianceCache::BeginFrame()
{
    // 每帧开始时重置更新列表
    m_updateList.clear();
    m_updateProbeCount = 0;
}

void RadianceCache::EndFrame()
{
    // 可以在这里更新统计信息
}

void RadianceCache::SwapBuffers()
{
    m_currentIndex = 1 - m_currentIndex;
}

void RadianceCache::CreateProbeBuffers(ID3D12Device* device)
{
    // 每个 Probe 的大小
    uint32_t probeSize = sizeof(RadianceProbeGPU);
    uint32_t bufferSize = probeSize * m_maxProbes;
    
    // 创建双缓冲
    for (int i = 0; i < RADIANCE_CACHE_BUFFER_COUNT; i++)
    {
        // 使用 DEFAULT heap（GPU only）
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            bufferSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS  // 需要 UAV
        );
        
        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_probeBuffer[i])
        );
        
        if (FAILED(hr))
        {
            ERROR_AND_DIE("[RadianceCache] Failed to create probe buffer!");
        }
        
        // 设置调试名称
        wchar_t debugName[64];
        swprintf_s(debugName, L"RadianceCache_ProbeBuffer_%d", i);
        m_probeBuffer[i]->SetName(debugName);
    }
    
    DebuggerPrintf("[RadianceCache] Created probe buffers (%u probes, %u bytes each)\n", 
                   m_maxProbes, bufferSize);
}

void RadianceCache::CreateUpdateListBuffer(ID3D12Device* device)
{
    // Update List 存储 uint32_t 索引
    uint32_t maxUpdateCount = 256;  // 每帧最多更新 256 个 Probes
    uint32_t bufferSize = sizeof(uint32_t) * maxUpdateCount;
    
    // 使用 UPLOAD heap（CPU 可写）
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_updateListBuffer)
    );
    
    if (FAILED(hr))
    {
        ERROR_AND_DIE("[RadianceCache] Failed to create update list buffer!");
    }
    
    m_updateListBuffer->SetName(L"RadianceCache_UpdateList");
}

// void RadianceCache::CreateConstantBuffer(ID3D12Device* device)
// {
//     uint32_t bufferSize = sizeof(RadianceCacheConstants);
//     
//     // 对齐到 256 bytes（D3D12 要求）
//     bufferSize = (bufferSize + 255) & ~255;
//     
//     D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
//     D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
//     
//     HRESULT hr = device->CreateCommittedResource(
//         &heapProps,
//         D3D12_HEAP_FLAG_NONE,
//         &bufferDesc,
//         D3D12_RESOURCE_STATE_GENERIC_READ,
//         nullptr,
//         IID_PPV_ARGS(&m_constantBuffer)
//     );
//     
//     if (FAILED(hr))
//     {
//         ERROR_AND_DIE("[RadianceCache] Failed to create constant buffer!");
//     }
//     
//     m_constantBuffer->SetName(L"RadianceCache_Constants");
// }

void RadianceCache::CreateReadbackBuffer(ID3D12Device* device)
{
    uint32_t bufferSize = sizeof(RadianceProbeGPU) * m_maxProbes;
    
    D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_readbackBuffer)
    );
    
    if (FAILED(hr))
    {
        ERROR_AND_DIE("[RadianceCache] Failed to create readback buffer!");
    }
    
    m_readbackBuffer->SetName(L"RadianceCache_Readback");
}

void RadianceCache::BuildUpdateQueue(uint32_t maxProbesThisFrame)
{
    m_updateList.clear();
    
    // 🔴 这里是简化版：选择最高优先级的 Probes
    // 实际应该由 RadianceCacheManager 来管理优先级队列
    
    struct ProbeWithPriority
    {
        uint32_t index;
        float priority;
        
        bool operator<(const ProbeWithPriority& other) const
        {
            return priority > other.priority;  // 降序
        }
    };
    
    std::vector<ProbeWithPriority> priorityQueue;
    priorityQueue.reserve(m_activeProbeCount);
    
    for (uint32_t i = 0; i < m_maxProbes; i++)
    {
        if (m_probes[i].m_isActive)
        {
            ProbeWithPriority item;
            item.index = i;
            item.priority = m_probes[i].m_priority;
            priorityQueue.push_back(item);
        }
    }
    
    // 排序
    std::sort(priorityQueue.begin(), priorityQueue.end());
    
    // 选择前 N 个
    uint32_t updateCount = MinUint32(maxProbesThisFrame, (uint32_t)priorityQueue.size());
    for (uint32_t i = 0; i < updateCount; i++)
    {
        m_updateList.push_back(priorityQueue[i].index);
    }
    
    m_updateProbeCount = (uint32_t)m_updateList.size();
}

void RadianceCache::UploadProbeData()
{
    if (m_updateList.empty())
        return;
    
    // 上传 Update List 到 GPU
    void* mappedData = nullptr;
    HRESULT hr = m_updateListBuffer->Map(0, nullptr, &mappedData);
    
    if (SUCCEEDED(hr))
    {
        memcpy(mappedData, m_updateList.data(), m_updateList.size() * sizeof(uint32_t));
        m_updateListBuffer->Unmap(0, nullptr);
    }
}
//
// void RadianceCache::UploadConstants(const RadianceCacheConstants& constants)
// {
//     void* mappedData = nullptr;
//     HRESULT hr = m_constantBuffer->Map(0, nullptr, &mappedData);
//     
//     if (SUCCEEDED(hr))
//     {
//         memcpy(mappedData, &constants, sizeof(RadianceCacheConstants));
//         m_constantBuffer->Unmap(0, nullptr);
//     }
// }

uint32_t RadianceCache::AllocateProbe(const Vec3& worldPos, const Vec2& screenPos)
{
    if (m_freeIndices.empty())
    {
        DebuggerPrintf("[RadianceCache] Warning: No free probes available!\n");
        return UINT32_MAX;
    }
    
    uint32_t index = m_freeIndices.back();
    m_freeIndices.pop_back();
    
    RadianceProbe& probe = m_probes[index];
    probe.m_probeIndex = index;
    probe.m_worldPosition = worldPos;
    probe.m_screenPosition = screenPos;
    probe.m_isActive = true;
    probe.m_isDirty = true;
    probe.m_lastUpdateFrame = 0;
    probe.m_birthFrame = 0;  // 需要外部设置
    probe.m_priority = 1.0f;
    
    m_activeProbeCount++;
    
    return index;
}

void RadianceCache::FreeProbe(uint32_t probeIndex)
{
    if (probeIndex >= m_maxProbes)
        return;
    
    RadianceProbe& probe = m_probes[probeIndex];
    if (!probe.m_isActive)
        return;
    
    probe.m_isActive = false;
    m_freeIndices.push_back(probeIndex);
    m_activeProbeCount--;
}

RadianceProbe* RadianceCache::GetProbe(uint32_t probeIndex)
{
    if (probeIndex >= m_maxProbes)
        return nullptr;
    return &m_probes[probeIndex];
}

const RadianceProbe* RadianceCache::GetProbe(uint32_t probeIndex) const
{
    if (probeIndex >= m_maxProbes)
        return nullptr;
    return &m_probes[probeIndex];
}

ID3D12Resource* RadianceCache::GetCurrentProbeBuffer() const
{
    return m_probeBuffer[m_currentIndex];
}

ID3D12Resource* RadianceCache::GetPreviousProbeBuffer() const
{
    return m_probeBuffer[1 - m_currentIndex];
}

ID3D12Resource* RadianceCache::GetUpdateListBuffer() const
{
    return m_updateListBuffer;
}

void RadianceCache::ReadbackProbeData(ID3D12GraphicsCommandList* cmdList, std::vector<RadianceProbeGPU>& outProbes)
{
    // 从 GPU 读回数据（用于调试）
    // 需要先 Copy 到 Readback Buffer
    
    cmdList->CopyResource(m_readbackBuffer, GetCurrentProbeBuffer());
    
    // 🔴 注意：这里需要等待 GPU 完成，然后再 Map
    // 实际使用时需要在下一帧才能读取
    
    outProbes.resize(m_maxProbes);
    
    void* mappedData = nullptr;
    HRESULT hr = m_readbackBuffer->Map(0, nullptr, &mappedData);
    
    if (SUCCEEDED(hr))
    {
        memcpy(outProbes.data(), mappedData, sizeof(RadianceProbeGPU) * m_maxProbes);
        m_readbackBuffer->Unmap(0, nullptr);
    }
}