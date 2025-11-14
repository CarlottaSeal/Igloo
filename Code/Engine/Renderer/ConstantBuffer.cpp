#include "ConstantBuffer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"

//Add dx11
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include "RenderCommon.h"

//Link some libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

ConstantBuffer::ConstantBuffer(ID3D11Device* device, size_t size)
	:m_size(size)
	,m_device(device)
{
	Create();
}
#ifdef ENGINE_DX12_RENDERER
ConstantBuffer::ConstantBuffer(size_t size)
	:m_size(size)
{
//#ifdef ENGINE_DX12_RENDERER
	m_constantBufferView = new D3D12_CONSTANT_BUFFER_VIEW_DESC();
//#endif
}

ConstantBuffer::ConstantBuffer(size_t multiBufferSize, size_t originalSize)
	:m_maxSize(multiBufferSize)
	,m_size((originalSize))
	//,m_frameOffsets(multiBufferSize/originalSize, 0)
{
	m_constantBufferView = new D3D12_CONSTANT_BUFFER_VIEW_DESC();
}

void ConstantBuffer::AppendData(void const* data, size_t size, int currentDraw)
{
	size_t alignedSize = AlignUp(size, 256);
	size_t offset = currentDraw * alignedSize;
	GUARANTEE_OR_DIE(offset + alignedSize <= m_maxSize, "ConstantBuffer overflow!");

	memcpy(m_mappedPtr + offset, data, size);
	m_offset = offset;
}

size_t ConstantBuffer::GetFrameOffset(int frame)
{
	return frame * m_size;
}

void ConstantBuffer::ResetOffset()
{
	m_offset = 0;
}
#endif

ConstantBuffer::~ConstantBuffer()
{
	if(m_buffer)
		m_buffer->Release();

#ifdef ENGINE_DX12_RENDERER
	delete m_constantBufferView;
	m_constantBufferView = nullptr;
	DX_SAFE_RELEASE( m_dx12ConstantBuffer )
#endif
}

void ConstantBuffer::Create()
{
	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.ByteWidth = (UINT)m_size;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
	if (!SUCCEEDED(hr))
	{
		DebuggerPrintf("CreateBuffer failed with HRESULT: 0x%08X\n", hr);
		ERROR_AND_DIE("Could not create constant buffer.");
	}
}
