#pragma once
#include <d3d12.h>

#include "Engine/Core/EngineCommon.hpp"

struct ID3D11Device;
struct ID3D11Buffer;

struct ID3D12Resource;
struct D3D12_INDEX_BUFFER_VIEW;

class IndexBuffer
{
	friend class Renderer;
	friend class DX11Renderer;
	friend class DX12Renderer;

public:
	IndexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride);

#ifdef ENGINE_DX12_RENDERER
	IndexBuffer(ID3D12Device* device, unsigned int size, unsigned int stride);
	void ResetRing();
	unsigned int AppendData(const void* data, unsigned int size);
	
	IndexBuffer(unsigned int size, unsigned int stride);
#endif
	
	IndexBuffer(const IndexBuffer& copy) = delete;
	virtual ~IndexBuffer();

	void Create();
	void Resize(unsigned int size);

	unsigned int GetSize();
	unsigned int GetStride();

protected:
	ID3D11Device* m_device = nullptr;
	ID3D11Buffer* m_buffer = nullptr;
	unsigned int m_size = 0;
	unsigned int m_stride = 0;

#ifdef ENGINE_DX12_RENDERER
	ID3D12Resource* m_dx12IndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};

	D3D12_GPU_VIRTUAL_ADDRESS m_gpuBaseAddress = 0; // 分配起始地址（可选）
	size_t m_offset = 0; // 当前分配偏移量（仅 ring 模式下使用）
	uint8_t* m_mappedPtr = nullptr; // 持久映射指针
	
	bool m_isRingBuffer = false; 
#endif
};