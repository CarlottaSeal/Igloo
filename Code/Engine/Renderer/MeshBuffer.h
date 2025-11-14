#pragma once
#include <d3d12.h>
#include <vector>
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"

//==============================================================================
// MeshBuffer - GPU Mesh Data for Mesh Shader Access
//==============================================================================
// 
// 用途：将Mesh数据上传到GPU，供Mesh Shader使用
// 
// 传统渲染管线：
//   Vertex Buffer → Vertex Shader → Pixel Shader
// 
// Mesh Shader管线：
//   Structured Buffers (Vertices + Indices) → Mesh Shader → Pixel Shader
//   ↑
//   这就是MeshBuffer的作用！
//
// 为什么需要MeshBuffer？
// - Mesh Shader不使用传统的Input Assembler（IA）
// - 需要通过Structured Buffer直接访问顶点/索引数据
// - 支持更灵活的几何处理（如Meshlet优化）
//
//==============================================================================

//------------------------------------------------------------------------------
// GPU Vertex Data (用于Structured Buffer)
//------------------------------------------------------------------------------
struct GPUVertex
{
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Vec3 tangent;
    Vec3 bitangent;
    
    GPUVertex() = default;
    
    // 从Vertex_PCUTBN转换
    GPUVertex(const Vertex_PCUTBN& vertex)
        : position(vertex.m_position)
        , normal(vertex.m_normal)
        , uv(vertex.m_uvTexCoords)
        , tangent(vertex.m_tangent)
        , bitangent(vertex.m_bitangent)
    {}
};

//------------------------------------------------------------------------------
// Meshlet（可选，用于优化）
//------------------------------------------------------------------------------
// Meshlet是Mesh的小块，用于Mesh Shader优化
// 每个Meshlet包含一组顶点和三角形索引
// UE5 Nanite使用类似的结构
struct Meshlet
{
    uint32_t vertexOffset;      // 顶点起始索引
    uint32_t vertexCount;       // 顶点数量
    uint32_t triangleOffset;    // 三角形起始索引
    uint32_t triangleCount;     // 三角形数量
    
    Meshlet()
        : vertexOffset(0), vertexCount(0)
        , triangleOffset(0), triangleCount(0)
    {}
};

//------------------------------------------------------------------------------
// MeshBuffer Class
//------------------------------------------------------------------------------
class MeshBuffer
{
public:
    MeshBuffer(ID3D12Device* device,
                const std::vector<Vertex_PCUTBN>& vertices, const std::vector<unsigned int>& indices);
    ~MeshBuffer();
    
    ID3D12Resource* GetVertexBuffer() const { return m_vertexBuffer; }
    ID3D12Resource* GetIndexBuffer() const { return m_indexBuffer; }
    ID3D12Resource* GetMeshletBuffer() const { return m_meshletBuffer; }
    
    uint32_t GetVertexCount() const { return m_vertexCount; }
    uint32_t GetIndexCount() const { return m_indexCount; }
    uint32_t GetTriangleCount() const { return m_indexCount / 3; }
    uint32_t GetMeshletCount() const { return m_meshletCount; }
    
    //--------------------------------------------------------------------------
    // SRV创建（用于Mesh Shader访问）
    //--------------------------------------------------------------------------
    // 在Descriptor Heap中创建SRV
    void CreateSRVs(
        ID3D12Device* device,
        D3D12_CPU_DESCRIPTOR_HANDLE vertexSRVHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE indexSRVHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE meshletSRVHandle
    );
    
    bool IsValid() const
    {
        return m_vertexBuffer != nullptr &&
               m_indexBuffer != nullptr &&
               m_vertexCount > 0 &&
               m_indexCount > 0;
    }
    
private:
    ID3D12Resource* m_vertexBuffer = nullptr;   // Structured Buffer<GPUVertex>
    ID3D12Resource* m_indexBuffer = nullptr;    // Structured Buffer<uint>
    ID3D12Resource* m_meshletBuffer = nullptr;  // Structured Buffer<Meshlet> (可选)
    
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
    uint32_t m_meshletCount = 0;
    
    void GenerateMeshlets(
        const std::vector<unsigned int>& indices
    );
    
    void CreateMeshletBuffer(
        ID3D12Device* device,
        const std::vector<Meshlet>& meshlets
    );
};

//==============================================================================
// 使用示例
//==============================================================================
/*

// 1. 在StaticMesh中添加MeshBuffer成员
class StaticMesh
{
public:
    MeshBuffer* m_meshBuffer = nullptr;  // 用于Mesh Shader渲染
};

// 2. 在加载Mesh时创建MeshBuffer
void StaticMesh::LoadFromFile(const std::string& path)
{
    // ... 加载顶点和索引 ...
    
    // 创建MeshBuffer
    m_meshBuffer = new MeshBuffer();
    m_meshBuffer->Initialize(device, m_verts, m_indices);
}

// 3. 在Card捕获时使用
void DX12Renderer::CaptureCard(SurfaceCard* card)
{
    MeshObject* meshObj = GetMeshObject(card->m_meshID);
    StaticMesh* mesh = meshObj->GetMesh();
    MeshBuffer* buffer = mesh->m_meshBuffer;
    
    if (!buffer || !buffer->IsValid())
        return;
    
    // 绑定SRV
    // commandList->SetGraphicsRootShaderResourceView(0, buffer->GetVertexBuffer()->GetGPUVirtualAddress());
    // ...
    
    // 使用Mesh Shader渲染
    commandList->DispatchMesh(...);
}

*/