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

#include "DX12Renderer.hpp"

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
#ifdef ENGINE_DX12_RENDERER
IndexBuffer::IndexBuffer(ID3D12Device* device, unsigned int size, unsigned int stride)
{
	m_stride = stride;
	m_size = (unsigned int)size;
	m_isRingBuffer = true;

	CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	HRESULT hr = device->CreateCommittedResource(
		&props, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, IID_PPV_ARGS(&m_dx12IndexBuffer));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create ring vertex buffer");

	m_gpuBaseAddress = m_dx12IndexBuffer->GetGPUVirtualAddress();

	CD3DX12_RANGE readRange(0, 0);
	hr = m_dx12IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedPtr));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to map ring vertex buffer");
	
	m_indexBufferView.BufferLocation = m_gpuBaseAddress;
	m_indexBufferView.SizeInBytes = 0;  // 每帧调用 AppendData 后更新
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 必须设置，表示每个 index 是 uint32

	m_offset = 0; // 当前 ring buffer 偏移（字节）
}

void IndexBuffer::ResetRing()
{
	if (m_isRingBuffer)
	{
		m_offset = 0;
	}
}

unsigned int IndexBuffer::AppendData(const void* data, unsigned int size)
{
	if (m_offset + size > m_size)
		m_offset = 0; // wrap around

	// Map, copy, unmap
	void* dst = nullptr;
	CD3DX12_RANGE range(0, 0);
	m_dx12IndexBuffer->Map(0, &range, &dst);
	memcpy((BYTE*)dst + m_offset, data, size);
	m_dx12IndexBuffer->Unmap(0, nullptr);

	m_indexBufferView.BufferLocation = m_dx12IndexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = (UINT)m_offset + (UINT)size;

	m_dx12IndexBuffer->SetName(L"index upload");

	unsigned int oldOffset = (unsigned int)m_offset / sizeof(unsigned int);
	m_offset += size;
	return oldOffset;
}

IndexBuffer::IndexBuffer(unsigned int size, unsigned int stride)
	: m_size(size)
	, m_stride(stride)
{
//#ifdef ENGINE_DX12_RENDERER
	//m_indexBufferView = new D3D12_INDEX_BUFFER_VIEW();
//#endif
}
#endif

IndexBuffer::~IndexBuffer()
{
	DX_SAFE_RELEASE(m_buffer);

#ifdef ENGINE_DX12_RENDERER
	DX_SAFE_RELEASE( m_dx12IndexBuffer );
	//delete m_indexBufferView;
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
