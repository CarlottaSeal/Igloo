#include "DXRAcceleration.h"
#include "Engine/Core/EngineCommon.hpp"

void DXRAcceleration::Shutdown()
{
    for (ID3D12Resource* blasBuffer : m_blasBuffers)
    {
        DX_SAFE_RELEASE(blasBuffer)
    }
    m_blasBuffers.clear();
    DX_SAFE_RELEASE(m_tlasBuffer)
    DX_SAFE_RELEASE(m_tlasInstanceDesc)
    DX_SAFE_RELEASE(m_scratchBuffer)
    DX_SAFE_RELEASE(m_device)
}

bool DXRAcceleration::Initialize(ID3D12Device5* device)
{
    m_device = device;
    
    // 检查 DXR 支持
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    HRESULT hr = device->CheckFeatureSupport(
        D3D12_FEATURE_D3D12_OPTIONS5, 
        &options5, 
        sizeof(options5)
    );
    
    if (FAILED(hr) || options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0)
    {
        // 回退到软件光追
        DebuggerPrintf("DXR not supported, will use software raytracing\n");
        return false;
    }
    DebuggerPrintf("DXR supported\n");
    return true;
}

void DXRAcceleration::PrepareBLAS(const Instance& instance)
{
    // 构建 BLAS 的基础设置
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = instance.m_vertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex_PCUTBN);
    geometryDesc.Triangles.VertexCount = instance.vertexCount;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.IndexBuffer = instance.m_indexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = instance.indexCount;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    
    // 获取所需内存大小
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
    inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs.NumDescs = 1;
    inputs.pGeometryDescs = &geometryDesc;
    
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);
    
    // 分配 BLAS buffer (Milestone 1 只需要这一步)
    // 实际构建留到后续 milestone
}

void DXRAcceleration::PrepareTLAS( const std::vector<Instance>& instances)
{
    UNUSED(instances)
}
