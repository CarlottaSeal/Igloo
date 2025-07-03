#pragma once
#include "Engine/Core/EngineCommon.hpp"

struct ID3D11Device;
struct ID3D11Buffer;

struct ID3D12Resource;
struct D3D12_CONSTANT_BUFFER_VIEW_DESC;

class ConstantBuffer
{
	friend class Renderer;
	friend class DX11Renderer;
	friend class DX12Renderer;

public:
	ConstantBuffer(ID3D11Device* device, size_t size);
	ConstantBuffer(size_t size);
	ConstantBuffer(const ConstantBuffer& copy) = delete;
	virtual ~ConstantBuffer();

	void Create();

private:
	ID3D11Device* m_device = nullptr;
	ID3D11Buffer* m_buffer = nullptr;
	size_t m_size = 0;

#ifdef ENGINE_DX12_RENDERER
	ID3D12Resource* m_dx12ConstantBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC* m_constantBufferView = nullptr;
#endif
};