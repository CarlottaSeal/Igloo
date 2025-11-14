#include "MeshBuffer.h"
#include "Engine/Core/EngineCommon.hpp"
#include "d3d12.h"

MeshBuffer::MeshBuffer(ID3D12Device* device,
    const std::vector<Vertex_PCUTBN>& vertices,
    const std::vector<unsigned int>& indices)
{
    if (vertices.empty() || indices.empty())
    {
        ERROR_AND_DIE("MeshBuffer::Initialize() - Empty vertices or indices!");
    }
    
    m_vertexCount = (uint32_t)vertices.size();
    m_indexCount = (uint32_t)indices.size();
    
    // 转换顶点格式
    std::vector<GPUVertex> gpuVertices;
    gpuVertices.reserve(vertices.size());
    for (const auto& v : vertices)
    {
        gpuVertices.push_back(GPUVertex(v));
    }
    
    // 生成Meshlets（可选，用于优化） TODO
    GenerateMeshlets(indices);
}

MeshBuffer::~MeshBuffer()
{
    DX_SAFE_RELEASE(m_vertexBuffer);
    DX_SAFE_RELEASE(m_indexBuffer);
    DX_SAFE_RELEASE(m_meshletBuffer);
    
    m_vertexCount = 0;
    m_indexCount = 0;
    m_meshletCount = 0;
}

void MeshBuffer::GenerateMeshlets(const std::vector<unsigned int>& indices)
{
    // 简单的Meshlet生成：每64个三角形一组
    const uint32_t MAX_TRIANGLES_PER_MESHLET = 64;
    const uint32_t MAX_VERTICES_PER_MESHLET = 64;
    
    uint32_t totalTriangles = (uint32_t)indices.size() / 3;
    m_meshletCount = (totalTriangles + MAX_TRIANGLES_PER_MESHLET - 1) / MAX_TRIANGLES_PER_MESHLET;
    
    // 这里简化处理，实际应该根据顶点复用情况优化
    // 参考UE5 Nanite或DirectX Mesh Shader Samples
    
    DebuggerPrintf("[MeshBuffer] Generated %u meshlets from %u triangles\n",
        m_meshletCount, totalTriangles);
}

void MeshBuffer::CreateMeshletBuffer(
    ID3D12Device* device,
    const std::vector<Meshlet>& meshlets)
{
    if (meshlets.empty())
        return;
    
    size_t bufferSize = meshlets.size() * sizeof(Meshlet);
    
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
        bufferSize,
        D3D12_RESOURCE_FLAG_NONE
    );
    
    HRESULT hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_meshletBuffer)
    );
    
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create meshlet buffer!");
    m_meshletBuffer->SetName(L"MeshBuffer_Meshlets");
    
    // TODO: 上传数据（类似顶点缓冲）
}

void MeshBuffer::CreateSRVs(
    ID3D12Device* device,
    D3D12_CPU_DESCRIPTOR_HANDLE vertexSRVHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE indexSRVHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE meshletSRVHandle)
{
    // Vertex Buffer SRV
    if (m_vertexBuffer)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_vertexCount;
        srvDesc.Buffer.StructureByteStride = sizeof(GPUVertex);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        
        device->CreateShaderResourceView(m_vertexBuffer, &srvDesc, vertexSRVHandle);
    }
    
    // Index Buffer SRV
    if (m_indexBuffer)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_indexCount;
        srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        
        device->CreateShaderResourceView(m_indexBuffer, &srvDesc, indexSRVHandle);
    }
    
    // Meshlet Buffer SRV（可选）
    if (m_meshletBuffer)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_meshletCount;
        srvDesc.Buffer.StructureByteStride = sizeof(Meshlet);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        
        device->CreateShaderResourceView(m_meshletBuffer, &srvDesc, meshletSRVHandle);
    }
}