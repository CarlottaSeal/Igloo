#pragma once
//#include <d3d12.h>

#include "Engine/Renderer/Renderer.hpp"

#include <string>

struct  ID3D11VertexShader;
struct	ID3D11PixelShader;
struct	ID3D11InputLayout;

struct ID3D10Blob;
struct D3D12_INPUT_LAYOUT_DESC;

struct ShaderConfig
{
	std::string m_name;
	std::string m_vertexEntryPoint = "VertexMain";
	std::string m_pixelEntryPoint = "PixelMain";
};

class Shader
{
	friend class Renderer;
	friend class DX11Renderer;
	friend class DX12Renderer;

private:
	Shader(const ShaderConfig& config);
	Shader(const Shader& copy) = delete;
	~Shader();

	const std::string& GetName() const;

private:
	ShaderConfig m_config;
	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader*  m_pixelShader = nullptr;
	ID3D11InputLayout*  m_inputLayout = nullptr;

#ifdef ENGINE_DX12_RENDERER
	int m_shaderIndex = -1;
	ID3D10Blob* m_dx12VertexShader = nullptr;
	ID3D10Blob* m_dx12PixelShader = nullptr;
	D3D12_INPUT_LAYOUT_DESC* m_inputLayoutForVertex = nullptr;
#endif
};