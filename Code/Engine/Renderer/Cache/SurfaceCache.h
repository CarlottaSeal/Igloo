#pragma once
#include <cstdint>
#include <d3d12.h>
#include "ThirdParty/d3dx12/d3dx12.h"

#include "Engine/Renderer/RenderCommon.h"
//#include "Engine/Renderer/DX12Renderer.hpp"

struct GBufferData;

struct TemporaryCaptureResources
{
	ID3D12Resource* m_tempTextures[4];      // 4个临时纹理
	D3D12_CPU_DESCRIPTOR_HANDLE m_tempRtvs[4];  // 对应的 RTV 在dx12中管理->我不知道好不好
	uint32_t maxResolution;               // 最大分辨率
};

class SurfaceCache
{
    friend class GISystem;
public:
    SurfaceCache() = default;
    ~SurfaceCache() = default;

    static constexpr uint32_t INVALID_TILE_INDEX = 0xFFFFFFFF;
    
    void Initialize(ID3D12Device* device, uint32_t atlasSize, uint32_t tileSize, SurfaceCacheType type = SURFACE_CACHE_TYPE_PRIMARY);
    void Shutdown();
	
	ID3D12Resource* GetCurrentAtlasTexture() const;
	ID3D12Resource* GetPreviousAtlasTexture() const;
	ID3D12Resource* GetCurrentMetadataBuffer() const;
	ID3D12Resource* GetPreviousMetadataBuffer() const;
	ID3D12Resource* GetCurrentMetadataUploadBuffer() const;
	ID3D12Resource* GetPreviousMetadataUploadBuffer() const;

	int GetCurrentBufferIndex() const { return m_currentIndex; }
	int GetPreviousBufferIndex() const { return 1 - m_currentIndex; }

	void SwapBuffers() { m_currentIndex = 1 - m_currentIndex; }

    const SurfaceCacheStats& GetStats() const { return m_stats; }
    void ResetStats() { m_stats = {}; }

private:
    void CreateAtlasTexture(ID3D12Device* device);
    void CreateCardMetadataBuffer(ID3D12Device* device);
    void CreateCardAllocator(ID3D12Device* device);
	
	void CreateTemporaryCaptureResources(ID3D12Device* device, uint32_t maxResolution);
	void ReleaseTemporaryCaptureResources();

public:
    SurfaceCacheType m_type;
    int m_currentIndex = 0;
	
	TemporaryCaptureResources m_tempCapture;
    ID3D12Resource* m_atlasTexture[SURFACE_CACHE_BUFFER_COUNT];
    ID3D12Resource* m_cardMetadataBuffer[SURFACE_CACHE_BUFFER_COUNT];
    ID3D12Resource* m_cardMetadataUploadBuffer[SURFACE_CACHE_BUFFER_COUNT]; // UPLOAD heap（CPU 写入的中转
    
    ID3D12Resource* m_tileAllocator[SURFACE_CACHE_BUFFER_COUNT];

    uint32_t m_atlasSize = 0;
    uint32_t m_tileSize = 0;
    uint32_t m_tilesPerRow = 0;
    uint32_t m_maxCards = 0;
    
    SurfaceCacheStats m_stats;
    
    bool m_initialized = false;
};

