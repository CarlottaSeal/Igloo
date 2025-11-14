#include "VertexBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/EngineCommon.hpp"

//Add dx11
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include <d3d12.h>

#include "DX12Renderer.hpp"

//Link some libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

VertexBuffer::VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride)
	: m_device(device)
	, m_size(size)
	, m_stride(stride)
    , m_buffer(nullptr)
{
    Create();
}
#ifdef ENGINE_DX12_RENDERER
VertexBuffer::VertexBuffer(ID3D12Device* device, unsigned int size, unsigned int stride)
{
	m_stride = stride;
	m_size = (unsigned int)size;
	m_isRingBuffer = true;

	CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	HRESULT hr = device->CreateCommittedResource(
		&props, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_dx12VertexBuffer));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create vertex buffer");

	m_gpuBaseAddress = m_dx12VertexBuffer->GetGPUVirtualAddress();

	CD3DX12_RANGE readRange(0, 0);
	hr = m_dx12VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedPtr));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to map vertex buffer");

	std::wstring name = L"VertexUpload_" + std::to_wstring(reinterpret_cast<uintptr_t>(this));
	m_dx12VertexBuffer->SetName(name.c_str());
	
	m_vertexBufferView.BufferLocation = m_gpuBaseAddress;
	m_vertexBufferView.SizeInBytes = size;
	m_vertexBufferView.StrideInBytes = stride;

	m_offset = 0;
}

void VertexBuffer::ResetRing()
{
	if (m_isRingBuffer)
	{
		m_offset = 0;
	}
}

unsigned int VertexBuffer::AppendData(const void* data, unsigned int size)
{
	GUARANTEE_OR_DIE(m_isRingBuffer, "Only ring buffer supports append!");
	GUARANTEE_OR_DIE(m_offset + size <= m_size, "Ring buffer overflow!");

	memcpy(m_mappedPtr + m_offset, data, size);

	unsigned int startVertexOffset = (unsigned int)m_offset / m_stride;

	m_offset += size;
	return startVertexOffset;
}

VertexBuffer::VertexBuffer(unsigned int size, unsigned int stride)
	: m_size(size)
	, m_stride(stride)
	, m_buffer(nullptr)
{
}
#endif

VertexBuffer::~VertexBuffer()
{
    DX_SAFE_RELEASE(m_buffer);

#ifdef ENGINE_DX12_RENDERER
	m_dx12VertexBuffer->Unmap(0, nullptr); 
	DX_SAFE_RELEASE(m_dx12VertexBuffer);
	//delete m_vertexBufferView;
#endif
}

void VertexBuffer::Create()
{
	//Create vertex buffer
	UINT vertexBufferSize = m_size;
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = vertexBufferSize;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	//Call ID3D11Device::CreateBuffer
	HRESULT hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		ERROR_AND_DIE("Could not create vertex buffer.");
	}
}

void VertexBuffer::Resize(unsigned int size) //Resize should safe release m_buffer and create one of the new size.
{
	if (size == 0 || size == m_size)
	{
		return; 
	}

	DX_SAFE_RELEASE(m_buffer);

	m_size = size;

	Create();
}

unsigned int VertexBuffer::GetSize()
{
    return m_size;
}

unsigned int VertexBuffer::GetStride()
{
    return m_stride;
}
