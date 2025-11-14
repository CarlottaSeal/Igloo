#pragma once
#include <string>
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Math/IntVec2.hpp"

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D12Resource;

class Texture
{
	friend class Renderer; // Only the Renderer can create new Texture objects!
	friend class DX11Renderer;
	friend class DX12Renderer;

private:
	Texture(); // can't instantiate directly; must ask Renderer to do it for you
	Texture(Texture const& copy) = delete; // No copying allowed!  This represents GPU memory.
	~Texture();

public:
	IntVec2				GetDimensions() const { return m_dimensions; }
	std::string const& GetImageFilePath() const { return m_name; }

protected:
	std::string			m_name;
	IntVec2				m_dimensions;

	//unsigned int		m_openglTextureID = 0xFFFFFFFF;

	ID3D11Texture2D* m_texture = nullptr;
	ID3D11ShaderResourceView* m_shaderResourceView = nullptr;

#ifdef ENGINE_DX12_RENDERER
	ID3D12Resource* m_dx12Texture = nullptr;
	ID3D12Resource* m_textureBufferUploadHeap = nullptr;
	int m_textureDescIndex = -1;
#endif
};