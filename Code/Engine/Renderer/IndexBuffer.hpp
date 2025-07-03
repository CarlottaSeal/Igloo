#pragma once
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
	IndexBuffer(unsigned int size, unsigned int stride);
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
	D3D12_INDEX_BUFFER_VIEW* m_indexBufferView = nullptr;
#endif
};