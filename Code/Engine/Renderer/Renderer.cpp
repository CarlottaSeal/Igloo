//#define WIN32_LEAN_AND_MEAN

#include "Engine/Renderer/DX11Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include <windows.h>
//#include <gl/gl.h>
#include <string.h>

#include "ThirdParty/stb/stb_image.h"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Window/Window.hpp"

//#pragma comment( lib, "opengl32" )

//Add dx11
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>

#include "DX12Renderer.hpp"

//Link some libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#if defined(ENGINE_DEBUG_RENDER)
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

//HDC g_displayDeviceContext = nullptr;				// ...becomes void* Window::m_displayContext
HGLRC g_openGLRenderingContext = nullptr;			// ...becomes void* Renderer::m_apiRenderingContext

// extern Window* g_theWindow;
//
#if defined(ENGINE_DEBUG_RENDER)
void* m_dxgiDebug = nullptr;
void* m_dxgiDebugModule = nullptr;
#endif

 #if defined(OPAQUE)
 #undef OPAQUE
 #endif

// struct LightConstants //DirectionalLight
// {
// 	float SunDirection[3];
// 	float SunIntensity;
// 	//float AmbientIntensity[4];
// 	float AmbientIntensity;
// 	float AmbientLightColor[3];
// };
// static const int k_lightConstantsSlot = 1;
//
// struct CameraConstants
// {
// 	Mat44 WorldToCameraTransform;
// 	Mat44 CameraToRenderTransform;
// 	Mat44 RenderToClipTransform;
//  };
// static const int k_cameraConstantsSlot = 2;
//
// struct ModelConstants
// {
// 	Mat44 ModelToWorldTransform;
// 	float ModelColor[4];
// };
// static const int k_modelConstantsSlot = 3;
//
// struct PointLightConstants
// {
// 	float PointLightPosition[3];
// 	float LightIntensity = 0.f;
// 	float LightColor[3];
// 	float LightRange;
// };
// static const int k_pointLightConstantsSlot = 4;
//
// struct SpotLightConstants
// {
// 	Vec3 SpotLightPosition;
// 	float SpotLightCutOff; // cos(45°)，表示光锥半角
// 	Vec3 SpotLightDirection; // Normalized
// 	float pad0;
// 	float SpotLightColor[3];
// 	float pad1;
// };
// static const int k_spotLightConstantsSlot = 5;
//
// struct ShadowConstants
// {
// 	Mat44 LightViewProjectionMatrix;
// };
// static const int k_shadowConstantsSlot = 6;

Renderer::Renderer(RendererConfig config)
    :m_config(config)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer = new DX11Renderer(config);
#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer = new DX12Renderer(config);
	#endif
}

void Renderer::Startup() 
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->Startup();
	#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->Startup();
	#endif
}

void Renderer::ShutDown()
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->ShutDown();
	#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->ShutDown();
	#endif
}

void Renderer::BeginFrame() 
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BeginFrame();
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->BeginFrame();
	#endif
}

void Renderer::EndFrame() 
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->EndFrame();
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->EndFrame();
	#endif
}

void Renderer::ClearScreen(const Rgba8 & clearColor)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->ClearScreen(clearColor);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->ClearScreen(clearColor);
	#endif
}

void Renderer::ClearScreen()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->ClearScreen(Rgba8::MAGENTA);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->ClearScreen(Rgba8::MAGENTA);
	#endif
}

void Renderer::BeginCamera(const Camera& camera)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BeginCamera(camera);
	#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->BeginCamera(camera);
	#endif
}

void Renderer::EndCamera(const Camera& camera)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->EndCamera(camera);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->EndCamera(camera);
	#endif
}

void Renderer::SetViewport(const AABB2& normalizedViewport)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetViewport(normalizedViewport);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->SetViewport(normalizedViewport);
	#endif
}

void Renderer::DrawVertexArray(int numVerts, const Vertex_PCU* verts)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexArray(numVerts, verts);
	#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->DrawVertexArray(numVerts, verts);
	#endif
}

void Renderer::DrawVertexArray(const std::vector<Vertex_PCUTBN>& verts)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexArray(verts);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->DrawVertexArray(verts);
	#endif
}

void Renderer::DrawVertexArray(int numVerts, const Vertex_PCUTBN* verts)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexArray(numVerts, verts);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->DrawVertexArray(numVerts, verts);
	#endif
}

void Renderer::DrawVertexIndexArray(int numVerts, const Vertex_PCUTBN* verts, int numIndices, const unsigned int* indices)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexIndexArray(numVerts, verts, numIndices, indices);
#endif
#ifdef ENGINE_DX12_RENDERER
	//m_dx12Renderer->DrawVertexIndexArray()
	UNUSED(numVerts);
	UNUSED(numIndices);
	UNUSED(indices);
	UNUSED(verts);
	ERROR_AND_DIE("Cannot use DX12 in this way!")
	#endif
}

void Renderer::DrawVertexIndexArray(int numVerts, const Vertex_PCUTBN* verts, int numIndices, const unsigned int* indices, VertexBuffer* vbo, IndexBuffer* ibo)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexIndexArray(numVerts, verts, numIndices, indices, vbo, ibo);
#endif
#ifdef ENGINE_DX12_RENDERER
	//m_dx12Renderer->DrawVertexIndexArray()
	UNUSED(numVerts);
	UNUSED(verts);
	UNUSED(numIndices);
	UNUSED(indices);
	UNUSED(vbo);
	UNUSED(ibo);
	ERROR_AND_DIE("Cannot use DX12 in this way!")
	#endif
}

void Renderer::DrawVertexIndexArray(const std::vector<Vertex_PCUTBN>& verts, const std::vector<unsigned int>& indices)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexIndexArray(verts, indices);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->DrawVertexIndexArray(verts, indices);
	#endif
}

void Renderer::DrawVertexIndexArray(const std::vector<Vertex_PCUTBN>& verts, const std::vector<unsigned int>& indices, VertexBuffer* vbo, IndexBuffer* ibo)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexIndexArray(verts, indices, vbo, ibo);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(verts);
	UNUSED(indices);
	UNUSED(vbo);
	UNUSED(ibo);
	ERROR_AND_DIE("Cannot use DX12 in this way!")
	#endif
}

void Renderer::DrawVertexArray(const std::vector<Vertex_PCU>& verts)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexArray(verts);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->DrawVertexArray(verts);
	#endif
}

void Renderer::DrawAABB2(const AABB2& bounds, const Rgba8& color)
{
	Vertex_PCU verts[6];

	Vec2 bottomLeft = bounds.m_mins;
	Vec2 topLeft = Vec2(bounds.m_mins.x, bounds.m_maxs.y);
	Vec2 topRight = bounds.m_maxs;
	Vec2 bottomRight = Vec2(bounds.m_maxs.x, bounds.m_mins.y);

	verts[0] = Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, Vec2(0.f, 0.f));
	verts[1] = Vertex_PCU(Vec3(topLeft.x, topLeft.y, 0.f), color, Vec2(0.f, 1.f));
	verts[2] = Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, Vec2(1.f, 1.f));

	verts[3] = Vertex_PCU(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, Vec2(0.f, 0.f));
	verts[4] = Vertex_PCU(Vec3(topRight.x, topRight.y, 0.f), color, Vec2(1.f, 1.f));
	verts[5] = Vertex_PCU(Vec3(bottomRight.x, bottomRight.y, 0.f), color, Vec2(1.f, 0.f));

	DrawVertexArray(6, verts);
}

Image* Renderer::CreateImageFromFile(char const* imageFilePath)
{
#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateImageFromFile(imageFilePath);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateImageFromFile(imageFilePath);
	#endif
}

Texture* Renderer::CreateTextureFromImage(const Image& image)
{
#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateTextureFromImage(image);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateTextureFromImage(image);
	#endif
}

Texture* Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateOrGetTextureFromFile(imageFilePath);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateOrGetTextureFromFile(imageFilePath);
	#endif
}

Texture* Renderer::CreateTextureFromFile(char const* imageFilePath)
{
	#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateTextureFromFile(imageFilePath);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateTextureFromFile(imageFilePath);
#endif
}

Texture* Renderer::GetTextureForFileName(const char* imageFilePath)
{
	#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->GetTextureForFileName(imageFilePath);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->GetTextureByFileName(imageFilePath);
	#endif
}

//------------------------------------------------------------------------------------------------
Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateTextureFromData(name, dimensions, bytesPerTexel, texelData);
#endif
 #ifdef ENGINE_DX12_RENDERER
	UNUSED(name);
	UNUSED(dimensions);
	UNUSED(bytesPerTexel);
	UNUSED(texelData);
 	ERROR_AND_DIE("Cannot use DX12 in this way!")
 	#endif
}

//-----------------------------------------------------------------------------------------------
void Renderer::BindTexture(const Texture* texture, int slot)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BindTexture(texture,slot);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->BindTexture(texture,slot);
	#endif
}

BitmapFont* Renderer::CreateOrGetBitmapFont(const char* bitmapFontFilePathWithNoExtension)
{
	#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateOrGetBitmapFont(bitmapFontFilePathWithNoExtension);
#endif
	#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateOrGetBitmapFont(bitmapFontFilePathWithNoExtension);
	#endif
}

Shader* Renderer::CreateShader(char const* shaderName, char const* shaderSource, VertexType vertexType)
{
	#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateShader(shaderName, shaderSource, vertexType);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateShader(shaderName, shaderSource, vertexType);
	#endif
}

Shader* Renderer::CreateShader(char const* shaderName, VertexType vertexType)
{
	#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateShader(shaderName, vertexType);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateShader(shaderName, vertexType);
	#endif
}

Shader* Renderer::CreateOrGetShader(char const* shaderName, VertexType vertexType)
{
	#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateOrGetShader(shaderName, vertexType);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateShader(shaderName, vertexType);
	#endif
}

bool Renderer::CompileShaderToByteCode(char const* name, char const* source, char const* entryPoint, char const* target, std::vector<unsigned char>& outByteCode, ID3DBlob** shaderByteCode)
{//只会在内部被call
#ifdef ENGINE_DX11_RENDERER
	UNUSED(shaderByteCode);
	return m_dx11Renderer->CompileShaderToByteCode(outByteCode, name, source, entryPoint, target);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(outByteCode);
	return m_dx12Renderer->CompileShaderToByteCode(shaderByteCode, name, source, entryPoint, target);
	#endif
}

void Renderer::BindShader(Shader* shader)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BindShader(shader);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->BindShader(shader);
	#endif
}

VertexBuffer* Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
	#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateVertexBuffer(size, stride);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateVertexBuffer(size, stride);
#endif
}

void Renderer::CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vbo)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CopyCPUToGPU(data, size, vbo);
#endif
	
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->CopyCPUToGPU(data, size, vbo);
	#endif
}

void Renderer::BindVertexBuffer(VertexBuffer* vbo)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BindVertexBuffer(vbo);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->BindVertexBuffer(vbo);
#endif
}

IndexBuffer* Renderer::CreateIndexBuffer(const unsigned int size, unsigned int stride)
{
#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateIndexBuffer(size, stride);
#endif
#ifdef ENGINE_DX12_RENDERER
	return m_dx12Renderer->CreateIndexBuffer(size, stride);
#endif
}

void Renderer::BindIndexBuffer(IndexBuffer* ibo)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BindIndexBuffer(ibo);
#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->BindIndexBuffer(ibo);
	#endif
}

void Renderer::DrawIndexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, unsigned int indexCount)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawIndexBuffer(vbo, ibo, indexCount);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(vbo);
	UNUSED(ibo);
	UNUSED(indexCount);
	#endif
}

void Renderer::CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer*& ibo)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CopyCPUToGPU(data, size, ibo);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(ibo);
	UNUSED(data);
	UNUSED(size);
	#endif
}

ConstantBuffer* Renderer::CreateConstantBuffer(const unsigned int size)
{
#ifdef ENGINE_DX11_RENDERER
	return m_dx11Renderer->CreateConstantBuffer(size);
#endif
	#ifdef ENGINE_DX12_RENDERER
	UNUSED(size);
	ERROR_AND_DIE("Cannot create a ConstantBuffer in DX12 interface!");
	#endif
}

void Renderer::CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* cbo)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CopyCPUToGPU(data, size, cbo);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->CopyCPUToGPU(data, size, cbo);
	#endif
}

void Renderer::BindConstantBuffer(int slot, ConstantBuffer* cbo)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BindConstantBuffer(slot, cbo);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->BindConstantBuffer(slot, cbo);
#endif
}

void Renderer::DrawVertexBuffer(VertexBuffer* vbo, unsigned int vertexCount)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->DrawVertexBuffer(vbo, vertexCount);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->DrawVertexBuffer(vbo, vertexCount);
#endif
}

void Renderer::SetBlendMode(BlendMode blendMode)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetBlendMode(blendMode);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->SetBlendMode(blendMode);
#endif
}

void Renderer::SetBlendModeIfChanged()
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetBlendModeIfChanged();
#endif
#ifdef ENGINE_DX12_RENDERER
	//m_dx12Renderer->SetBlendModeIfChanged();
	#endif
}

void Renderer::SetRasterizerModeIfChanged()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetRasterizerModeIfChanged();
#endif
}

void Renderer::SetRasterizerMode(RasterizerMode rasterizerMode)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetRasterizerMode(rasterizerMode);
#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->SetRasterizerMode(rasterizerMode);
	#endif
}

void Renderer::SetSamplerMode(SamplerMode samplerMode, int slot)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetSamplerMode(samplerMode, slot);
#endif
	#ifdef ENGINE_DX12_RENDERER
	UNUSED(slot);
	m_dx12Renderer->SetSamplerMode(samplerMode);
	#endif
}

void Renderer::SetSamplerModeIfChanged()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetSamplerModeIfChanged();
#endif
}

void Renderer::SetSamplerModeIfChanged(int slot)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetSamplerModeIfChanged(slot);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(slot);
#endif
}

void Renderer::SetDepthMode(DepthMode depthMode)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetDepthMode(depthMode);
#endif
	#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->SetDepthMode(depthMode);
	#endif
}

void Renderer::SetDepthModeIfChanged()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetDepthModeIfChanged();
#endif
}

void Renderer::SetGeneralLightConstants(const Rgba8 sunColor, const Vec3& sunNormal, int numLights,
                                        std::vector<Rgba8> colors, std::vector<Vec3> worldPositions, std::vector<Vec3> spotForwards,
                                        std::vector<float> ambiences, std::vector<float> innerRadii, std::vector<float> outerRadii,
                                        std::vector<float> innerDotThresholds, std::vector<float> outerDotThresholds)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetGeneralLightConstants(sunColor, sunNormal, numLights,
										colors, worldPositions, spotForwards,
										ambiences, innerRadii, outerRadii,
										innerDotThresholds, outerDotThresholds);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(sunColor);
	UNUSED(sunNormal);
	UNUSED(numLights);
	UNUSED(colors);
	UNUSED(worldPositions);
	UNUSED(spotForwards);
	UNUSED(ambiences);
	UNUSED(innerRadii);
	UNUSED(outerRadii);
	UNUSED(innerDotThresholds);
	UNUSED(outerDotThresholds);
	#endif
}


//void Renderer::SetLightConstants(const Vec3& sunDirection, const float sunIntensity, const Rgba8& ambientColor)
//{
//	LightConstants lightConstants = {};
//	lightConstants.SunDirection = sunDirection;
//	lightConstants.SunIntensity = sunIntensity;
//	ambientColor.GetAsFloats(lightConstants.AmbientIntensity);
//
//	CopyCPUToGPU(&lightConstants, sizeof(LightConstants), m_lightCBO);
//	BindConstantBuffer(k_lightConstantsSlot, m_lightCBO);
//}
#ifdef ENGINE_PAST_VERSION_LIGHTS
void Renderer::SetLightConstants(const Vec3& sunDirection, const float sunIntensity, const float ambientIntensity, const Rgba8& ambientColor)
{
	LightConstants lightConstants = {};
	lightConstants.SunDirection[0] = sunDirection.x;
	lightConstants.SunDirection[1] = sunDirection.y;
	lightConstants.SunDirection[2] = sunDirection.z;
	lightConstants.SunIntensity = sunIntensity;
	lightConstants.AmbientIntensity = ambientIntensity;
	/*lightConstants.AmbientLightColor[0] = ambientColor.r;
	lightConstants.AmbientLightColor[1] = ambientColor.g;
	lightConstants.AmbientLightColor[2] = ambientColor.b;*/
	float ambientColorAsFloats[4];
	ambientColor.GetAsFloats(ambientColorAsFloats);
	lightConstants.AmbientLightColor[0] = ambientColorAsFloats[0];
	lightConstants.AmbientLightColor[1] = ambientColorAsFloats[1];
	lightConstants.AmbientLightColor[2] = ambientColorAsFloats[2];
	//memcpy(lightConstants.AmbientIntensity, ambientIntensity, sizeof(float) * 4);

	CopyCPUToGPU(&lightConstants, sizeof(LightConstants), m_lightCBO);
	BindConstantBuffer(k_lightConstantsSlot, m_lightCBO);
}

void Renderer::SetPointLightConstants(const std::vector<Vec3>& pos, std::vector<float> intensity, const std::vector<Rgba8>& color, std::vector<float> range)
{
	std::vector<PointLightConstants> pointLightConstantsArray;
	for (int i = 0; i<10;i++)
	{
		PointLightConstants pointLightConstants = {};
		pointLightConstants.PointLightPosition[0] = pos[i].x;
		pointLightConstants.PointLightPosition[1] = pos[i].y;
		pointLightConstants.PointLightPosition[2] = pos[i].z;
		float pointColorAsFloats[4];
		color[i].GetAsFloats(pointColorAsFloats);
		pointLightConstants.LightColor[0] = pointColorAsFloats[0];
		pointLightConstants.LightColor[1] = pointColorAsFloats[1];
		pointLightConstants.LightColor[2] = pointColorAsFloats[2];
		pointLightConstants.LightIntensity = intensity[i];
		pointLightConstants.LightRange = range[i];
		pointLightConstantsArray.push_back(pointLightConstants);
		
	}
	CopyCPUToGPU(pointLightConstantsArray.data(), sizeof(PointLightConstants)*10, m_pointLightCBO);
	BindConstantBuffer(k_pointLightConstantsSlot, m_pointLightCBO);
}

void Renderer::SetSpotLightConstants(const std::vector<Vec3>& pos, std::vector<float> cutOff, const std::vector<Vec3>& dir, const std::vector<Rgba8>& color)
{
	std::vector<SpotLightConstants> spotLightConstantsArray = {};
	for (int i = 0; i<4;i++)
	{
		SpotLightConstants spotLightConstants = {};
		spotLightConstants.SpotLightPosition = pos[i];
		spotLightConstants.SpotLightCutOff = cutOff[i];
		spotLightConstants.SpotLightDirection = dir[i].GetNormalized();
		float spotColorAsFloats[4];
		color[i].GetAsFloats(spotColorAsFloats);
		spotLightConstants.SpotLightColor[0] = spotColorAsFloats[0];
		spotLightConstants.SpotLightColor[1] = spotColorAsFloats[1];
		spotLightConstants.SpotLightColor[2] = spotColorAsFloats[2];
		spotLightConstantsArray.push_back(spotLightConstants);
	}
	CopyCPUToGPU(spotLightConstantsArray.data(), sizeof(SpotLightConstants)*4, m_spotLightCBO);
	BindConstantBuffer(k_spotLightConstantsSlot, m_spotLightCBO);
}
#endif

void Renderer::SetModelConstants(const Mat44& modelToWorldTransform, const Rgba8& modelColor)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetModelConstants(modelToWorldTransform, modelColor);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->SetModelConstants(modelToWorldTransform, modelColor);
	#endif
}

void Renderer::SetShadowConstants(const Mat44& lightViewProjectionMatrix)
{
	#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetShadowConstants(lightViewProjectionMatrix);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(lightViewProjectionMatrix);
#endif
}

void Renderer::SetPerFrameConstants(const float time, const int debugInt, const float debugFloat)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetPerFrameConstants(time, debugInt, debugFloat);
#endif
#ifdef ENGINE_DX12_RENDERER
	m_dx12Renderer->SetPerFrameConstants(time, debugInt, debugFloat);
	#endif
}

//------------------------------------------------------
//Functions called in Startup()
void Renderer::CreateDeviceAndSwapChain(unsigned int deviceFlags)
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateDeviceAndSwapChain(deviceFlags);
#endif
#ifdef ENGINE_DX12_RENDERER
	UNUSED(deviceFlags);
#endif
}

void Renderer::GetBackBufferTexture()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->GetBackBufferTexture();
#endif
}

void Renderer::SetDefaultTexture()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->SetDefaultTexture();
#endif
}

void Renderer::CreateAndBindDefaultShader()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateAndBindDefaultShader();
#endif
}

void Renderer::CreateSampleStates()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateSampleStates();
#endif
}

void Renderer::CreateBlendStates()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateBlendStates();
#endif
}

void Renderer::CreateRasterizerStates()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateRasterizerStates();
#endif
}

void Renderer::CreateDepthStencilStates()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateDepthStencilStates();
#endif
}

void Renderer::CreateDepthStencilTextureAndView()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateDepthStencilTextureAndView();
#endif
}

void Renderer::CreateShadowMapResources()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateShadowMapResources();
#endif
}

void Renderer::BeginShadowPass()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BeginShadowPass();
#endif
}

void Renderer::EndShadowPass()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->EndShadowPass();
#endif
}

void Renderer::CreateShadowMapShader() //Only create
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->CreateShadowMapShader();
#endif
}

void Renderer::BindShadowMapTextureAndSampler()
{
#ifdef ENGINE_DX11_RENDERER
	m_dx11Renderer->BindShadowMapTextureAndSampler();
#endif
}
