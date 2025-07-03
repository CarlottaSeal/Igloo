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

ConstantBuffer::ConstantBuffer(size_t size)
	:m_size(size)
{
#ifdef ENGINE_DX12_RENDERER
	m_constantBufferView = new D3D12_CONSTANT_BUFFER_VIEW_DESC();
#endif
}

ConstantBuffer::~ConstantBuffer()
{
	if(m_buffer)
		m_buffer->Release();

#ifdef ENGINE_DX12_RENDERER
	DX_SAFE_RELEASE( m_dx12ConstantBuffer )
	delete m_constantBufferView;
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
		ERROR_AND_DIE("Could not create constant buffer.");
	}
}
