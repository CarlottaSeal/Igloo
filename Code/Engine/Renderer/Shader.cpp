#include "Shader.hpp"
#include "Engine/Renderer/Renderer.hpp"

//Add dx11 & dx12
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

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

Shader::Shader(const ShaderConfig& config)
	:m_config(config)
{
}

Shader::~Shader()
{
	/*m_vertexShader->Release();
	m_pixelShader->Release();
	m_inputLayout->Release();*/
	DX_SAFE_RELEASE(m_vertexShader);
	DX_SAFE_RELEASE(m_pixelShader);
	DX_SAFE_RELEASE(m_inputLayout);

#ifdef ENGINE_DX12_RENDERER
	DX_SAFE_RELEASE(m_dx12VertexShader);
	DX_SAFE_RELEASE(m_dx12PixelShader);
	if (m_inputLayout)
	{
		delete m_inputLayoutForVertex;
		m_inputLayoutForVertex = nullptr;
	}
	
#endif
}

const std::string& Shader::GetName() const
{
    return m_config.m_name;
}
