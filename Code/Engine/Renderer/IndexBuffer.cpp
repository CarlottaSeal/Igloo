#include "IndexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/EngineCommon.hpp"

//Add dx11
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi.h>

//Link some libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

IndexBuffer::IndexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride)
	: m_device(device)
	, m_size(size)
	, m_stride(stride)
	, m_buffer(nullptr)
{
	Create();
}

IndexBuffer::IndexBuffer(unsigned int size, unsigned int stride)
	: m_size(size)
	, m_stride(stride)
{
#ifdef ENGINE_DX12_RENDERER
	m_indexBufferView = new D3D12_INDEX_BUFFER_VIEW();
#endif
}

IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);

#ifdef ENGINE_DX12_RENDERER
	DX_SAFE_RELEASE( m_dx12IndexBuffer );
	delete m_indexBufferView;
#endif
}

void IndexBuffer::Create()
{
	//Create INDEX buffer
	UINT indexBufferSize = m_size;
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = indexBufferSize;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	//Call ID3D11Device::CreateBuffer
	HRESULT hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer.");
	}
}

void IndexBuffer::Resize(unsigned int size)
{
	if (size == 0 || size == m_size)
	{
		return;
	}

	DX_SAFE_RELEASE(m_buffer);

	m_size = size;

	Create();
}

unsigned int IndexBuffer::GetSize()
{
	return m_size;
}

unsigned int IndexBuffer::GetStride()
{
	return m_stride;
}
