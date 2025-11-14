//#ifndef WIN32_LEAN_AND_MEAN

#include "Engine/Renderer/DX12Renderer.hpp"

#include "Cache/CardBVH.h"
#include "GI/GISystem.h"

#ifdef ENGINE_DX12_RENDERER

#include "Renderer.hpp"
#include "ConstantBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Shader.hpp"
#include "VertexBuffer.hpp"
#include "SDFTexture3D.h"
#include "Cache/RadianceCacheManager.h"
#include "Cache/SurfaceCard.h"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/RenderCommon.h"
#include "Engine/Renderer/GI/DefaultGBufferShader.h"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Object/Mesh/MeshObject.h"
#include "Engine/Scene/SDF/SDFCommon.h"

#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/implot.h"
#include "ThirdParty/ImGui/imgui_impl_dx12.h"
#include "ThirdParty/ImGui/imgui_impl_win32.h"
#include "ThirdParty/stb/stb_image.h"

// #include <wrl/client.h>
// using Microsoft::WRL::ComPtr;

extern GISystem* g_theGISystem;

DX12Renderer::DX12Renderer(RendererConfig config)
{
	m_config = config;

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_renderTargets[i] = nullptr;
	}
}

DX12Renderer::~DX12Renderer()
{

}

void DX12Renderer::Startup()
{
#ifdef ENGINE_DEBUG_RENDER
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	debugController->EnableDebugLayer();
#endif

	IDXGIFactory4* factory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot create D3D12 DXGI factory");

	IDXGIAdapter1* hardwareAdapter;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &hardwareAdapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		hardwareAdapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		if (SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device))))
		{
			break;
		}
	}

	//hr = D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device));
	//GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot create D3D12 device");
	hardwareAdapter->Release();

	// get the descriptor size
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_scuDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot create D3D12 command queue!");

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
	swapChainDesc.BufferDesc.Width = m_config.m_window->GetClientDimensions().x;
	swapChainDesc.BufferDesc.Height = m_config.m_window->GetClientDimensions().y;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = (HWND)m_config.m_window->GetHwnd();
	swapChainDesc.SampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)
	swapChainDesc.Windowed = TRUE;  //TODO: make full screen usable

	hr = factory->CreateSwapChain(
		m_commandQueue,
		&swapChainDesc,
		(IDXGISwapChain**)&m_swapChain
	);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot create D3D12 swap chain!");
	factory->Release();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		if (m_config.m_enableGI)
		{
			rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT + GBUFFER_COUNT + CARD_CAPTURE_RTV_COUNT;
		} //3层cardCapture
		else
		{
			rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT;
		}
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvDescHeap));

		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot create D3D12 descriptor heaps!");
	}

	// Create frame resources.
	{
		//二编：除非有特殊需求（比如调试低层句柄地址），建议统一使用 Offset() 方法，符合现代 C++ 封装风格
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			//Get the i buffer in the swap chain and store it in the i position of ID3D12Resource array
			//GetBuffer() really allocate memory in GPU, create the rtv
			hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
			GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot get D3D12 render target!");
			std::wstring rtName = L"BackBuffer_" + std::to_wstring(i);
			m_renderTargets[i]->SetName(rtName.c_str());
			
			//make a rtv 'Descriptor'
			//"create" a render target view which binds the swap chain buffer (ID3D12Resource[i]) to the rtv handle
			m_device->CreateRenderTargetView(m_renderTargets[i], nullptr, rtvHandle);

			//increment the rtv handle by offsetting the rtv descriptor size
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}
	//Create CA with 1st
	// -- Create the Command Allocators -- //
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i]));
		if (FAILED(hr))
		{
			GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot create all CA!");
		}
		std::wstring name = L"CA" + std::to_wstring(i);
		m_commandAllocator[i]->SetName(name.c_str());
	}

	CreateGraphicsRootSignature();

	m_constantBuffers.fill(nullptr);

	// Constant buffers: create descriptor heap and resource heap
//	+---------------------+
//  | Create CBV Heap     | -- D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV (Shader Visible)
//  +---------------------+
//           |
//           V
//  +---------------------+           +----------------------+
//  | Create Upload Heap  | --> -->   | Create ID3D12Resource|
//  +---------------------+           +----------------------+
//           |
//           V
//  +---------------------+
//  | Create CBV and write|
//  | to descriptor heap  |
//  +---------------------+
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	//heapDesc.NumDescriptors = NUM_CONSTANT_BUFFERS + MAX_TEXTURE_COUNT + GBUFFER_COUNT + SURFACE_CACHE_DESCRIPTOR_COUNT;  
	heapDesc.NumDescriptors = TOTAL_NUM_DESCRIPTORS;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvDescHeap));
	m_cbvSrvDescHeap->SetName(L"Default CBVSRV heap");
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create constant buffer description heap!");

	// create a resource heap, descriptor heap, and pointer to cbv for each frame
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(1024 * 64);
	for (int cbSlot = 0; cbSlot < NUM_CONSTANT_BUFFERS; ++cbSlot)
	{
		size_t alignedSize = AlignUp(GetConstantBufferSize(cbSlot), 256);
		size_t totalBufferSize = alignedSize * 256;
    
		m_constantBuffers[cbSlot] = new ConstantBuffer(totalBufferSize, alignedSize);
    
		// 创建一个大buffer
		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBufferSize);
		hr = m_device->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffers[cbSlot]->m_dx12ConstantBuffer)
		);
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create constant buffer!");

		CD3DX12_RANGE range(0, 0);
		hr = m_constantBuffers[cbSlot]->m_dx12ConstantBuffer->Map(0, &range,
			reinterpret_cast<void**>(&m_constantBuffers[cbSlot]->m_mappedPtr));

		std::wstring cbName = L"ConstantBuffer_" + std::to_wstring(cbSlot);
		m_constantBuffers[cbSlot]->m_dx12ConstantBuffer->SetName(cbName.c_str());
	}

	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvDescHeap));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot Create Depth Stencil Descriptor heap!");

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES defaultProperties(D3D12_HEAP_TYPE_DEFAULT);
	//CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_config.m_window->GetClientDimensions().x, m_config.m_window->GetClientDimensions().y, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, m_config.m_window->GetClientDimensions().x, m_config.m_window->GetClientDimensions().y, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	m_device->CreateCommittedResource(
		&defaultProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilTextureDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer)
	);
	m_depthStencilBuffer->SetName(L"DepthStencilBuffer");

	m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilDesc, m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	// Create the command list.
	//We need to create a direct command list so that we can execute our clear render target command. We do this by specifying D3D12_COMMAND_LIST_TYPE_DIRECT for the second parameter.
	//Since we only need one command list, which is reset each frame where we specify a command allocator, we just create this command list with the first command allocator.
	//When a command list is created, it is created in the "recording" state. We do not want to record to the command list yet, so we Close() the command list after we create it.
	hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[0], m_pipelineStateObject, IID_PPV_ARGS(&m_commandList));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 Cannot Create CommandList!");
	//hr = m_commandList->Close(); //第一帧end时close
	//GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 cannot close the new created CommandList!");
	//ID3D12CommandList* ppCommandLists[] = { m_commandList };
	//m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

	// Sets up root signature and descriptor heaps for the command list, prepare for binding resources
	m_commandList->SetGraphicsRootSignature(m_graphicsRootSignature);
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvSrvDescHeap };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	m_loadedTextures.clear();
	Image* whiteImage = new Image(IntVec2(4, 4), Rgba8::WHITE);
	m_defaultTexture = CreateTextureFromImage(*whiteImage);
	m_defaultTexture->m_name = "Default";
	m_loadedTextures.push_back(m_defaultTexture);
	
	Image* image1 = new Image(IntVec2(2, 2), Rgba8(127, 127, 255));
	m_defaultNormalTexture = CreateTextureFromImage(*image1);
	m_defaultNormalTexture->m_name = "DefaultNormal";
	m_loadedTextures.push_back(m_defaultNormalTexture);

	Image* image2 = new Image(IntVec2(2, 2), Rgba8(127, 127, 0));
	m_defaultSpecTexture = CreateTextureFromImage(*image2);
	m_defaultSpecTexture->m_name = "DefaultSpec";
	m_loadedTextures.push_back(m_defaultSpecTexture);

	// Create the pipeline state, which includes compiling and loading shaders.
	m_defaultShader = CreateShader("Default", m_shaderSource);
	m_desiredShader = m_defaultShader;

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		// create the fences
		for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i]));
			GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 Cannot Create Fence!");
			m_fenceValue[i] = 0; // set the initial fence value to 0

			m_fence[i]->SetName((L"Fence_" + std::to_wstring(i)).c_str());
		}
		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ERROR_AND_DIE("D3D12 Cannot Create Fence Event!");
		}
	}
	//Create Ring buffers
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_frameVertexBuffers[i] = new VertexBuffer(m_device, VERTEX_RING_BUFFER_SIZE, sizeof(Vertex_PCU));
		m_frameVertexBuffersForTBN[i] = new VertexBuffer(m_device, VERTEX_RING_BUFFER_SIZE, sizeof(Vertex_PCUTBN));
		m_frameIndexBuffers[i] = new IndexBuffer(m_device, VERTEX_RING_BUFFER_SIZE, sizeof(Vertex_PCU));
	}
	// Wait for the command list to execute; we are reusing the same command 
	// list in our main loop but for now, we just want to wait for setup to 
	// complete before continuing.
	// for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	// {
	// 	m_frameIndex = i;
	// }
	WaitForPreviousFrame();

	if (m_config.m_enableGI) //TODO: 这里的renderMode应该是一个组合管线，由多种搭配组合而成
	{
		CreateCardCaptureResources();
		CreateCardCapturePipelineStates();

		CreateGBufferResources();
		//m_gBuffer.m_depth = m_depthStencilBuffer;
		CreateDepthSRV();
		
		CreateComputeRootSignature();
		
		CreateRadianceCacheResources();
		CreateCompositePSO();

		CreateSDFGenerationRootSignature();
		CreateSDFGenerationPSO();

		CreateRadianceCacheUpdatePSO();
		CreateRadianceCacheResources();
	}

	ImGuiStartUp();
}

void DX12Renderer::BeginFrame()
{
	//WaitForPreviousFrame();
	UINT frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_frameIndex = (int)frameIndex;
	m_frameVertexBuffers[m_frameIndex]->ResetRing();
	m_frameVertexBuffersForTBN[m_frameIndex]->ResetRing();
	m_frameIndexBuffers[m_frameIndex]->ResetRing();

	m_currentDrawIndex = 0;
	
	for (int i = 0; i < NUM_CONSTANT_BUFFERS; ++i)
	{
		m_constantBuffers[i]->ResetOffset();
	}

	m_radianceCache.BeginFrame();
	g_theGISystem->BeginFrame(m_frameIndex);

	m_commandList->SetGraphicsRootSignature(m_graphicsRootSignature);
	
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbvSrvDescHeap };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// 显式绑定srv槽 如果改成bindless的话就只需要这里绑定一次了 //二编：已改！
	CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
	                                               NUM_CONSTANT_BUFFERS, m_scuDescriptorSize);
	m_commandList->SetGraphicsRootDescriptorTable(14, descriptorHandle);

	ImGuiBeginFrame();
}

void DX12Renderer::EndFrame()
{
	if (m_gBufferPassActive)
		EndGBufferPass();

	ImGuiEndFrame();

	if (m_currentActivePass == ActivePass::BACKBUFFER)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &barrier);
	}

	m_currentRenderMode = RenderMode::UNKNOWN;
	m_currentActivePass = ActivePass::UNKNOWN;
	m_forwardPassActive = false;

	HRESULT hr = m_commandList->Close();
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12: Failed to Close CommandList");

	ID3D12CommandList* listsToExecute[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(1, listsToExecute);

	const UINT currentFrameIndex = m_frameIndex;
	const UINT64 currentFenceValue = m_fenceValue[currentFrameIndex];

	hr = m_commandQueue->Signal(m_fence[currentFrameIndex], currentFenceValue);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12: Failed to signal fence");

	hr = m_swapChain->Present(1, 0);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12: Present failed");

	m_radianceCache.EndFrame(); //TODO: meishayong
	

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	if (m_fence[m_frameIndex]->GetCompletedValue() < m_fenceValue[m_frameIndex])
	{
		hr = m_fence[m_frameIndex]->SetEventOnCompletion(m_fenceValue[m_frameIndex], m_fenceEvent);
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12: Failed to SetEventOnCompletion");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	hr = m_commandAllocator[m_frameIndex]->Reset();
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12: Failed to reset CommandAllocator");

	hr = m_commandList->Reset(m_commandAllocator[m_frameIndex], m_pipelineStateObject);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12: Failed to reset CommandList");

	m_fenceValue[m_frameIndex]++;

	if (m_fence[currentFrameIndex]->GetCompletedValue() < currentFenceValue)
	{
		hr = m_fence[currentFrameIndex]->SetEventOnCompletion(currentFenceValue, m_fenceEvent);
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to set completion event");
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	for (auto& res : m_previousFrameTempResources)
	{
		DX_SAFE_RELEASE(res);
	}
	m_previousFrameTempResources.clear();

	m_previousFrameTempResources = m_currentFrameTempResources;
	m_currentFrameTempResources.clear();

	m_hasBackBufferCleared = false;

	if (m_giSystem)
		m_giSystem->EndFrame();
}

void DX12Renderer::ShutDown()
{
	ImGuiShutDown();
	if (m_giSystem)
		m_giSystem->Shutdown();

	for (int i = 0; i < FRAME_BUFFER_COUNT; ++i)
	{
		m_frameIndex = i;
		WaitForPreviousFrame();
	}

	for (int scIdx = 0; scIdx < SURFACE_CACHE_TYPE_COUNT; scIdx++)
	{
		m_surfaceCaches[scIdx].Shutdown();
	}

	for (BitmapFont* font : m_loadedFonts)
	{
		delete font;
		font = nullptr;
	}
	m_loadedFonts.clear();
	for (Texture* tex : m_loadedTextures)
	{
		delete tex;
		tex = nullptr;
	}
	m_loadedTextures.clear();
	for (Shader* shader : m_loadedShaders)
	{
		delete shader;
		shader = nullptr;
	}
	m_loadedShaders.clear();

	for (int j = 0; j < NUM_CONSTANT_BUFFERS; j++)
	{
		delete m_constantBuffers[j];
		m_constantBuffers[j] = nullptr;
	}

	for (int n = 0; n < FRAME_BUFFER_COUNT; n++)
	{
		delete m_frameVertexBuffers[n];
		m_frameVertexBuffers[n] = nullptr;
		delete m_frameIndexBuffers[n];
		m_frameIndexBuffers[n] = nullptr;
		delete m_frameVertexBuffersForTBN[n];
		m_frameVertexBuffersForTBN[n] = nullptr;
	}
	for (SDFTexture3D* sdf : m_loadedSDFs)
	{
		delete sdf;
	}
	m_loadedSDFs.clear();

	DX_SAFE_RELEASE(m_gBuffer.m_albedo);
	DX_SAFE_RELEASE(m_gBuffer.m_normal);
	DX_SAFE_RELEASE(m_gBuffer.m_motion);
	DX_SAFE_RELEASE(m_gBuffer.m_material);
	
	m_radianceCache.Shutdown();
    
	DX_SAFE_RELEASE(m_cardBVHNodeBuffer);
	DX_SAFE_RELEASE(m_cardBVHIndexBuffer);
	DX_SAFE_RELEASE(m_radianceCacheUpdatePSO);

	for (auto& pair : m_pipelineStatesConfiguration)
	{
		DX_SAFE_RELEASE(pair.second);
	}
	m_pipelineStatesConfiguration.clear();
	for (auto& gBufferPair : m_gBufferPipelineStatesConfiguration)
	{
		DX_SAFE_RELEASE(gBufferPair.second);
	}
	m_gBufferPipelineStatesConfiguration.clear();
	for (auto& cardCapturePair : m_cardCapturePSOConfiguration)
	{
		DX_SAFE_RELEASE(cardCapturePair.second);
	}
	m_cardCapturePSOConfiguration.clear();
	DX_SAFE_RELEASE(m_compositePSO);
	DX_SAFE_RELEASE(m_sdfGenerationPSO);

	DX_SAFE_RELEASE(m_graphicsRootSignature); //its bound in pso
	DX_SAFE_RELEASE(m_computeRootSignature);
	DX_SAFE_RELEASE(m_sdfGenerationRootSignature);

	DX_SAFE_RELEASE(m_swapChain);
	
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		DX_SAFE_RELEASE(m_renderTargets[i]);
	}
	DX_SAFE_RELEASE(m_depthStencilBuffer);

	DX_SAFE_RELEASE(m_imguiSrvDescHeap);
	DX_SAFE_RELEASE(m_rtvDescHeap);
	DX_SAFE_RELEASE(m_dsvDescHeap);
	DX_SAFE_RELEASE(m_cbvSrvDescHeap);

	DX_SAFE_RELEASE(m_commandList);
	for (ID3D12CommandAllocator* ca : m_commandAllocator)
	{
		DX_SAFE_RELEASE(ca);
	}
	DX_SAFE_RELEASE(m_commandQueue);

	CloseHandle(m_fenceEvent);
	for (ID3D12Fence* fence : m_fence)
	{
		DX_SAFE_RELEASE(fence);
		fence = nullptr;
	}
	
	//ComPtr<ID3D12DebugDevice> debugDevice;
	//if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
	//{
	//	debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	//}
	DX_SAFE_RELEASE(m_device);
}

void DX12Renderer::ClearScreen(Rgba8 const& clearColor)
{
	// // Indicate that the back buffer will be used as a render target.
	// CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// m_commandList->ResourceBarrier(1, &barrier);
	//
	// CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	// CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());
	// m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	//
	// float colorAsFloats[4];
	// clearColor.GetAsFloats(colorAsFloats);
	// m_commandList->ClearRenderTargetView(rtvHandle, colorAsFloats, 0, nullptr);
	// m_commandList->ClearDepthStencilView(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	//if (m_currentRenderMode == RenderMode::FORWARD)
	//{
	//BeginForwardPass();
	//ClearForwardPassRTV(clearColor);

	m_commandList->ClearDepthStencilView(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//}
	//if (m_currentRenderMode == RenderMode::GI)
	//{
	//	BeginGBufferPass(clearColor);
	//}
}

void DX12Renderer::ClearScreen()
{
	ClearScreen(Rgba8::MAGENTA);
}

void DX12Renderer::BeginCamera(Camera const& camera)
{
	m_camera = camera;
	m_previousCam = m_currentCam;
	//CameraConstants cam;
	m_currentCam.WorldToCameraTransform = camera.GetWorldToCameraTransform();
	m_currentCam.CameraToRenderTransform = camera.GetCameraToRenderTransform();
	m_currentCam.RenderToClipTransform = camera.GetRenderToClipTransform();
	m_currentCam.CameraWorldPosition = camera.GetPosition();

	//D3D12_VIEWPORT viewport = {};
	m_viewport.TopLeftX = 0.f;
	m_viewport.TopLeftY = 0.f;
	m_viewport.Width = (float)m_config.m_window->GetClientDimensions().x;
	m_viewport.Height = (float)m_config.m_window->GetClientDimensions().y;
	m_viewport.MinDepth = 0.f;
	m_viewport.MaxDepth = 1.f;

	//m_viewport = &viewport;

	D3D12_RECT scissorRect;
	scissorRect.left = 0;
	scissorRect.right = m_config.m_window->GetClientDimensions().x;
	scissorRect.bottom = m_config.m_window->GetClientDimensions().y;
	scissorRect.top = 0;


	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &scissorRect);

	m_scissorRect = scissorRect;

	// UINT8* cbGPUAddress;
	// CD3DX12_RANGE readRange(0, 0);
	// m_frameConstantBuffers[m_frameIndex][k_cameraConstantsSlot]
	// 	->m_dx12ConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&cbGPUAddress));
	// memcpy(cbGPUAddress, &cam, sizeof(CameraConstants));
	// m_frameConstantBuffers[m_frameIndex][k_cameraConstantsSlot]
	// 	->m_dx12ConstantBuffer->Unmap(0, nullptr);
	// m_constantBuffers[k_cameraConstantsSlot]->AppendData(&cam, sizeof(CameraConstants), m_frameIndex); 
	// BindConstantBuffer(k_cameraConstantsSlot,
	// 	m_constantBuffers[k_cameraConstantsSlot]);
	m_constantBuffers[k_cameraConstantsSlot]->AppendData(&m_currentCam, sizeof(CameraConstants), m_currentDrawIndex); 
	BindConstantBuffer(k_cameraConstantsSlot,
		m_constantBuffers[k_cameraConstantsSlot]);
}

void DX12Renderer::EndCamera(Camera const& camera)
{
	UNUSED(camera);
}

void DX12Renderer::BeginForwardPass()
{
	if (m_currentActivePass != ActivePass::BACKBUFFER)
	{
		// Indicate that the back buffer will be used as a render target.
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &barrier);
	}
	
	PassModeConstants modeConstants;
	modeConstants.RenderMode =static_cast<uint32_t>(RenderMode::FORWARD);
	m_constantBuffers[k_passConstantsSlot]->AppendData(&modeConstants, sizeof(PassModeConstants), m_currentDrawIndex);
	BindConstantBuffer(k_passConstantsSlot, m_constantBuffers[k_passConstantsSlot]);

	m_currentActivePass = ActivePass::BACKBUFFER;
	m_forwardPassActive = true;
}

void DX12Renderer::ClearForwardPassRTV(Rgba8 const& clearColor)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	float colorAsFloats[4];
	clearColor.GetAsFloats(colorAsFloats);
	m_commandList->ClearRenderTargetView(rtvHandle, colorAsFloats, 0, nullptr);

	m_hasBackBufferCleared = true;
}

void DX12Renderer::EndForwardPass()
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_renderTargets[m_frameIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
	m_commandList->ResourceBarrier(1, &barrier);

	m_forwardPassActive = false;
}

void DX12Renderer::SetViewport(const AABB2& normalizedViewport)
{
	UNUSED(normalizedViewport);
	//TODO: make it work
}

void DX12Renderer::SetModelConstants(Mat44 const& modelMatrix, Rgba8 const& modelColor)
{
	ModelConstants modelConstants;
	modelConstants.ModelToWorldTransform = modelMatrix;
	modelColor.GetAsFloats(modelConstants.ModelColor);

	m_constantBuffers[k_modelConstantsSlot]->AppendData(&modelConstants, sizeof(ModelConstants), m_currentDrawIndex); 
	BindConstantBuffer(k_modelConstantsSlot, m_constantBuffers[k_modelConstantsSlot]);
}

void DX12Renderer::SetLightConstants(Vec3 const& lightPosition, float ambient, Mat44 const& lightViewMatrix, Mat44 const& lightProjectionMatrix)
{
	UNUSED(lightPosition); UNUSED(ambient); UNUSED(lightViewMatrix); UNUSED(lightProjectionMatrix);
}

void DX12Renderer::SetPerFrameConstants(const float time, const int debugInt, const float debugFloat)
{
	PerFrameConstants perFrameConstants = {};
	perFrameConstants.Time = time;
	perFrameConstants.DebugInt = debugInt;
	perFrameConstants.DebugFloat = debugFloat;

	m_constantBuffers[k_perFrameConstantsSlot]->AppendData(&perFrameConstants, sizeof(perFrameConstants), m_currentDrawIndex);
	BindConstantBuffer(k_perFrameConstantsSlot, m_constantBuffers[k_perFrameConstantsSlot]);
}

void DX12Renderer::SetGeneralLightConstants(Rgba8 sunColor, const Vec3& sunNormal, int numLights,
	std::vector<Rgba8> colors, std::vector<Vec3> worldPositions, std::vector<Vec3> spotForwards,
	std::vector<float> ambiences, std::vector<float> innerRadii, std::vector<float> outerRadii,
	std::vector<float> innerDotThresholds, std::vector<float> outerDotThresholds)
{
	if (numLights > s_maxLights)
	{
		ERROR_AND_DIE("Cannot handle this many lights!")
	}
	GeneralLightConstants lightConstant = {};
	float sunColorAsFloats[4];
	sunColor.GetAsFloats(sunColorAsFloats);
	lightConstant.SunColor[0] = sunColorAsFloats[0];
	lightConstant.SunColor[1] = sunColorAsFloats[1];
	lightConstant.SunColor[2] = sunColorAsFloats[2];
	lightConstant.SunColor[3] = sunColorAsFloats[3];
	lightConstant.SunNormal[0] = sunNormal.x;
	lightConstant.SunNormal[1] = sunNormal.y;
	lightConstant.SunNormal[2] = sunNormal.z;
	lightConstant.NumLights = numLights;

	for (int i = 0; i < numLights; i++)
	{
		GeneralLight light;

		float lightColorAsFloats[4];
		colors[i].GetAsFloats(lightColorAsFloats);
		light.Color[0] = lightColorAsFloats[0];
		light.Color[1] = lightColorAsFloats[1];
		light.Color[2] = lightColorAsFloats[2];
		light.Color[3] = lightColorAsFloats[3];
		light.WorldPosition[0] = worldPositions[i].x;
		light.WorldPosition[1] = worldPositions[i].y;
		light.WorldPosition[2] = worldPositions[i].z;

		light.SpotForward[0] = spotForwards[i].x;
		light.SpotForward[1] = spotForwards[i].y;
		light.SpotForward[2] = spotForwards[i].z;
		light.Ambience = ambiences[i];
		light.InnerRadius = innerRadii[i];
		light.OuterRadius = outerRadii[i];
		light.InnerDotThreshold = innerDotThresholds[i];
		light.OuterDotThreshold = outerDotThresholds[i];

		lightConstant.LightsArray[i] = light;
	}
	
	m_constantBuffers[k_generalLightConstantsSlot]->AppendData(&lightConstant, sizeof(GeneralLightConstants), m_currentDrawIndex);
	BindConstantBuffer(k_generalLightConstantsSlot, m_constantBuffers[k_generalLightConstantsSlot]);
}

void DX12Renderer::SetMaterialConstants(const Texture* diffuseTex, const Texture* normalTex, const Texture* specTex)
{
	int index = k_materialConstantsSlot;
	ConstantBuffer* cb = m_constantBuffers[index];

	MaterialConstants materialConstant = {};
	if (diffuseTex)
		materialConstant.DiffuseId = diffuseTex->m_textureDescIndex;
	else
		materialConstant.DiffuseId = 0;
	if (normalTex)
		materialConstant.NormalId = normalTex->m_textureDescIndex;
	else
		materialConstant.NormalId = 1;

	if (specTex)
		materialConstant.SpecularId = specTex->m_textureDescIndex;
	else
		materialConstant.SpecularId = 2;
	
	// void* gpuPtr = nullptr;
	// CD3DX12_RANGE range(0, 0);
	// cb->m_dx12ConstantBuffer->Map(0, &range, &gpuPtr);
	// memcpy(gpuPtr, &materialConstant, sizeof(materialConstant));
	// cb->m_dx12ConstantBuffer->Unmap(0, nullptr);
	cb->AppendData(&materialConstant, sizeof(MaterialConstants), m_currentDrawIndex); 
	BindConstantBuffer(k_materialConstantsSlot, cb);
}

void DX12Renderer::SetComputeSurfaceCacheConstants(SurfaceCacheType type, size_t batchStart, int bindComputeSlot)
{
	SurfaceCacheConstants constants = {};

	switch (type)
	{
	case SURFACE_CACHE_TYPE_PRIMARY:
		constants.AtlasWidth = (float)m_giSystem->m_config.m_primaryAtlasSize;
		constants.AtlasHeight = (float)m_giSystem->m_config.m_primaryAtlasSize;
		constants.TileSize = m_giSystem->m_config.m_primaryTileSize;
		break;

	case SURFACE_CACHE_TYPE_GI:
		constants.AtlasWidth = (float)m_giSystem->m_config.m_giAtlasSize;
		constants.AtlasHeight = (float)m_giSystem->m_config.m_giAtlasSize;
		constants.TileSize = m_giSystem->m_config.m_giTileSize;
		break;
	}

	constants.ScreenWidth = (float)m_config.m_window->GetClientDimensions().x;
	constants.ScreenHeight = (float)m_config.m_window->GetClientDimensions().y;

	Mat44 worldToCamera = m_currentCam.WorldToCameraTransform;
	Mat44 cameraToRender = m_currentCam.CameraToRenderTransform;
	Mat44 renderToClip = m_currentCam.RenderToClipTransform;

	Mat44 viewProj = renderToClip;
	viewProj.Append(cameraToRender);   
	viewProj.Append(worldToCamera);    
	constants.ViewProj = viewProj;
	constants.ViewProjInverse = viewProj.GetInverse();

	Mat44 prevWorldToCamera = m_previousCam.WorldToCameraTransform;
	Mat44 prevCameraToRender = m_previousCam.CameraToRenderTransform;
	Mat44 prevRenderToClip = m_previousCam.RenderToClipTransform;

	Mat44 prevViewProj = prevWorldToCamera;
	prevViewProj.Append(prevCameraToRender);
	prevViewProj.Append(prevRenderToClip);
	constants.PrevViewProj = prevViewProj;

	constants.TilesPerRow = (uint32_t)(constants.AtlasWidth / constants.TileSize);
	constants.CurrentFrame = m_frameIndex;
	constants.CameraPosition = m_currentCam.CameraWorldPosition;

	constants.TemporalBlend = m_giSystem->m_config.m_enableTemporal ? 0.9f : 0.0f;

	float cameraMovement = GetDistanceSquared3D(m_currentCam.CameraWorldPosition,
		m_previousCam.CameraWorldPosition);
	if (cameraMovement > 1.0f)  
	{
		constants.TemporalBlend = 0.5f;
	}
	else
	{
		constants.TemporalBlend = 0.9f; 
	}

	size_t batchEnd = min(batchStart + s_maxCardsPerBatch, m_giSystem->m_dirtyCards.size());
	constants.ActiveCardCount = (uint32_t)(batchEnd - batchStart);
    
	// uint32_t* flatArray = (uint32_t*)constants.DirtyCardIndices;
	// for (size_t i = 0; i < constants.ActiveCardCount; ++i)
	// {
	// 	flatArray[i] = m_giSystem->m_dirtyCards[batchStart + i];
	// }
	//
	// for (size_t i = constants.ActiveCardCount; i < s_maxCardsPerBatch; ++i)
	// {
	// 	flatArray[i] = 0;
	// }
	m_constantBuffers[k_surfaceCacheConstantsSlot]->AppendData(&constants, 
			sizeof(SurfaceCacheConstants), m_currentDrawIndex);
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = 
		m_constantBuffers[k_surfaceCacheConstantsSlot]->m_dx12ConstantBuffer->GetGPUVirtualAddress()
		+ m_constantBuffers[k_surfaceCacheConstantsSlot]->m_offset;
	m_commandList->SetComputeRootConstantBufferView(bindComputeSlot, cbAddress);
}

void DX12Renderer::DrawVertexArray(int numVertexes, const Vertex_PCU* vertex)
{
	if (numVertexes == 0)
	{
		return;
	}
	
	const unsigned int vertexBufferSize = sizeof(Vertex_PCU) * numVertexes;

	VertexBuffer* ringBuffer = m_frameVertexBuffers[m_frameIndex];

	// 拷贝数据进 ring buffer（内部更新 view）
	unsigned int offset = ringBuffer->AppendData(vertex, vertexBufferSize);

	// 3. 调用 Draw：用当前 view + stride
	DrawVertexBuffer(ringBuffer, numVertexes, (int)offset);
}

void DX12Renderer::DrawVertexArray(int numVerts, const Vertex_PCUTBN* verts)
{
	UNUSED(numVerts);
	UNUSED(verts);
	ERROR_AND_DIE("Dont use this!")
}

void DX12Renderer::DrawVertexArray(std::vector<Vertex_PCU> const& verts)
{
	DrawVertexArray((int)verts.size(), verts.data());
}

void DX12Renderer::DrawVertexArray(std::vector<Vertex_PCUTBN> const& verts)
{
	if ((int)verts.size() < 3)
	{
		return;
	}

	const unsigned int vertexBufferSize = sizeof(Vertex_PCUTBN) * (int)verts.size();

	VertexBuffer* ringBuffer = m_frameVertexBuffers[m_frameIndex];

	unsigned int offset = ringBuffer->AppendData(verts.data(), vertexBufferSize);

	DrawVertexBuffer(ringBuffer, (int)verts.size(), (int)offset);
}

void DX12Renderer::DrawVertexIndexArray(std::vector<Vertex_PCU> const& verts, std::vector<unsigned int> const& indexes)
{
	int numOfIndexes = (int)indexes.size();
	if (numOfIndexes == 0)
	{
		return;
	}
	int numOfVerts = (int)verts.size();
	if (numOfVerts == 0)
	{
		return;
	}

	const unsigned int vertexBufferSize = sizeof(Vertex_PCU) * numOfVerts;
	const unsigned int indexBufferSize = sizeof(unsigned int) * numOfIndexes;

	VertexBuffer* ringVBO = m_frameVertexBuffers[m_frameIndex];
	IndexBuffer* ringIBO = m_frameIndexBuffers[m_frameIndex];

	ringVBO->AppendData(verts.data(), vertexBufferSize);       // 单位：字节
	unsigned int iboOffset = ringIBO->AppendData(indexes.data(), indexBufferSize);     // 单位：字节

	DrawIndexedVertexBuffer(ringVBO, ringIBO, numOfIndexes, iboOffset / sizeof(unsigned int)); //offset 以索引为单位
}

void DX12Renderer::DrawVertexIndexArray(std::vector<Vertex_PCUTBN> const& verts, std::vector<unsigned int> const& indexes)
{
	int numOfIndexes = (int)indexes.size();
	if (numOfIndexes == 0)
	{
		return;
	}
	int numOfVerts = (int)verts.size();
	if (numOfVerts == 0)
	{
		return;
	}

	const unsigned int vertexBufferSize = sizeof(Vertex_PCUTBN) * numOfVerts;
	const unsigned int indexBufferSize = sizeof(unsigned int) * numOfIndexes;

	VertexBuffer* ringVBO = m_frameVertexBuffers[m_frameIndex];
	IndexBuffer* ringIBO = m_frameIndexBuffers[m_frameIndex];

	ringVBO->AppendData(verts.data(), vertexBufferSize);     
	unsigned int iboOffset = ringIBO->AppendData(indexes.data(), indexBufferSize);    

	DrawIndexedVertexBuffer(ringVBO, ringIBO, numOfIndexes, iboOffset / sizeof(unsigned int)); //offset 以索引为单位
}

void DX12Renderer::DrawVertexBuffer(VertexBuffer* vbo, int vertexCount, int vertexOffset /*= 0 */)
{
	// m_constantBuffers[k_cameraConstantsSlot]->AppendData(&m_currentCam, sizeof(CameraConstants), m_currentDrawIndex); 
	// BindConstantBuffer(k_cameraConstantsSlot,
	// 	m_constantBuffers[k_cameraConstantsSlot]);
	
	SetGraphicsStatesIfChanged();
	BindVertexBuffer(vbo);
	
	// ✅ offset 正确传入，单位是“顶点数”
	m_commandList->DrawInstanced(vertexCount, 1, vertexOffset, 0);
	m_currentDrawIndex ++;
}

void DX12Renderer::DrawIndexedVertexBuffer(VertexBuffer* vbo, IndexBuffer* ibo, int indexCount, int indexOffset)
{
	// m_constantBuffers[k_cameraConstantsSlot]->AppendData(&m_currentCam, sizeof(CameraConstants), m_frameIndex); 
	// BindConstantBuffer(k_cameraConstantsSlot,
	// 	m_constantBuffers[k_cameraConstantsSlot]);

	SetGraphicsStatesIfChanged();
	BindVertexBuffer(vbo);
	BindIndexBuffer(ibo);
	m_commandList->DrawIndexedInstanced(indexCount, 1, indexOffset, 0, 0);
	m_currentDrawIndex ++;
}

Shader* DX12Renderer::CreateShader(char const* shaderName, VertexType type)
{
	Shader* test = GetShaderByName(shaderName);
	if (test)
	{
		return test;
	}
	std::string shaderHLSLName = std::string(shaderName) + ".hlsl";

	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderHLSLName);
	if (result < 0)
	{
		ERROR_AND_DIE("Fail to create shader from this shaderName");
	}
	return CreateShader(shaderName, shaderSource.c_str(), type);
}

Shader* DX12Renderer::CreateShader(char const* shaderName, char const* shaderSource, VertexType type /*= VertexType::PCU */)
{
	Shader* test = GetShaderByName(shaderName);
	if (test)
	{
		return test;
	}

	ShaderConfig sConfig;
	sConfig.m_name = std::string(shaderName);
	Shader* shader = new Shader(sConfig);

	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;

	CompileShaderToByteCode(&vertexShader, "VertexShader", shaderSource, sConfig.m_vertexEntryPoint.c_str(), "vs_5_0");
	CompileShaderToByteCode(&pixelShader, "PixelShader", shaderSource, sConfig.m_pixelEntryPoint.c_str(), "ps_5_1");

	static D3D12_INPUT_ELEMENT_DESC inputElementDescsForPCU[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	static D3D12_INPUT_ELEMENT_DESC inputElementDescsForPCUTBN[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	shader->m_dx12VertexShader = vertexShader;
	shader->m_dx12PixelShader = pixelShader;
	shader->m_inputLayoutForVertex = new D3D12_INPUT_LAYOUT_DESC();
	if (type == VertexType::VERTEX_PCU)
	{
		shader->m_inputLayoutForVertex->pInputElementDescs = inputElementDescsForPCU;
		shader->m_inputLayoutForVertex->NumElements = _countof(inputElementDescsForPCU);
	}
	else if (type == VertexType::VERTEX_PCUTBN)
	{
		shader->m_inputLayoutForVertex->pInputElementDescs = inputElementDescsForPCUTBN;
		shader->m_inputLayoutForVertex->NumElements = _countof(inputElementDescsForPCUTBN);
	}

	shader->m_shaderIndex = (int)m_loadedShaders.size();

	GUARANTEE_OR_DIE(shader->m_shaderIndex <= 65535, "Cannot create more than 65535 shaders!");
	m_loadedShaders.push_back(shader);
	return shader;
}

bool DX12Renderer::CompileShaderToByteCode(ID3DBlob** shaderByteCode, char const* name, char const* source, char const* entryPoint, char const* target)
{
#if defined ENGINE_DEBUG_RENDER
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | 
		D3DCOMPILE_PREFER_FLOW_CONTROL | D3DCOMPILE_ENABLE_STRICTNESS	|
		D3DCOMPILE_DEBUG_NAME_FOR_SOURCE;
#else
	UINT compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	ID3DBlob* shaderBlob = NULL;
	ID3DBlob* errorBlob = NULL;

	// Compile shader
	HRESULT hr = D3DCompile(
		source, strlen(source),
		name, nullptr, nullptr,
		entryPoint, target, compileFlags, 0,
		&shaderBlob, &errorBlob
	);
	if (SUCCEEDED(hr))
	{
		*shaderByteCode = shaderBlob;
		if (errorBlob != NULL)
		{
			errorBlob->Release();
		}
		return true;
	}
	else
	{
		if (errorBlob != NULL)
		{
			DebuggerPrintf((char*)errorBlob->GetBufferPointer());
		}
		if (errorBlob != NULL)
		{
			errorBlob->Release();
		}
		ERROR_AND_DIE(Stringf("Could not compile %s.", name));
	}
}

void DX12Renderer::BindShader(Shader* shader)
{
	if (shader == nullptr)
	{
		//m_desiredShader = m_defaultShader;
		if (m_currentRenderMode == RenderMode::GI)
			m_desiredShader = m_defaultGBufferShader;
		if(m_currentRenderMode == RenderMode::FORWARD)
			m_desiredShader = m_defaultShader;
	}
	else
	{
		m_desiredShader = shader;
	}
}

Shader* DX12Renderer::GetShaderByName(char const* shaderName)
{
	for (Shader* shader : m_loadedShaders)
	{
		std::string thisName = shader->GetName();
		if (thisName == shaderName)
		{
			return shader;
		}
	}
	return nullptr;
}

VertexBuffer* DX12Renderer::CreateVertexBuffer(unsigned int const size, unsigned int stride)
{
	// 仅创建upload heap，后续对于大的资产和需求应添加default heap
	// VertexBuffer* vertBuffer = new VertexBuffer(size, stride);
	//
	// CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	// CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	// HRESULT hr = m_device->CreateCommittedResource(
	// 	&heapProps,
	// 	D3D12_HEAP_FLAG_NONE,
	// 	&desc,
	// 	D3D12_RESOURCE_STATE_GENERIC_READ,
	// 	nullptr,
	// 	IID_PPV_ARGS(&vertBuffer->m_dx12VertexBuffer));
	// GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot Create Vertex Buffer!");
	VertexBuffer* vertBuffer = new VertexBuffer(m_device, size, stride);

	return vertBuffer;
}

void DX12Renderer::CopyCPUToGPU(void const* data, unsigned int size, VertexBuffer* vbo)
{
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);
	vbo->m_dx12VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
	memcpy(pVertexDataBegin, data, size);
	vbo->m_dx12VertexBuffer->Unmap(0, nullptr);

	vbo->m_vertexBufferView.BufferLocation = vbo->m_dx12VertexBuffer->GetGPUVirtualAddress();
	vbo->m_vertexBufferView.StrideInBytes = sizeof(Vertex_PCUTBN);
	vbo->m_vertexBufferView.SizeInBytes = (UINT)size;
}

void DX12Renderer::CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* ibo)
{
	UINT8* pIndexDataBegin;
	CD3DX12_RANGE readRange(0, 0);       
	ibo->m_dx12IndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
	memcpy(pIndexDataBegin, data, size);
	ibo->m_dx12IndexBuffer->Unmap(0, nullptr);

	ibo->m_indexBufferView.BufferLocation = ibo->m_dx12IndexBuffer->GetGPUVirtualAddress();
	ibo->m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	ibo->m_indexBufferView.SizeInBytes = (UINT)size;
}

void DX12Renderer::CopyCPUToGPU(const void* data, unsigned int size, ID3D12Resource* buffer)
{
	void* mappedData = nullptr;
	D3D12_RANGE readRange = {0, 0};
	buffer->Map(0, &readRange, &mappedData);
	memcpy(mappedData, data, size);
	buffer->Unmap(0, nullptr);
}

void DX12Renderer::CopyUploadHeapToDefaultHeap(ID3D12Resource* defaultHeap, ID3D12Resource* uploadHeap)
{
	CD3DX12_RESOURCE_BARRIER before = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultHeap,  // DEFAULT heap
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,  
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	m_commandList->ResourceBarrier(1, &before);
    
	m_commandList->CopyResource(
		defaultHeap,      // DEFAULT heap（目标）
		uploadHeap // UPLOAD heap（源）
	);
}

void DX12Renderer::BindIndexBuffer(IndexBuffer* ibo)
{
	m_commandList->IASetIndexBuffer(&ibo->m_indexBufferView);
}

Texture* DX12Renderer::CreateOrGetTextureFromFile(char const* imageFilePath)
{
	Texture* existingTexture = GetTextureByFileName(imageFilePath);
	if (existingTexture)
	{
		return existingTexture;
	}

	Texture* newTexture = CreateTextureFromFile(imageFilePath);
	return newTexture;
}

BitmapFont* DX12Renderer::CreateOrGetBitmapFont(char const* bitmapFontFilePathWithNoExtension)
{
	std::string fontTextureFilePath = std::string(bitmapFontFilePathWithNoExtension) + ".png";

	Texture* bitmapTexture = CreateOrGetTextureFromFile(fontTextureFilePath.c_str());
	if (!bitmapTexture)
	{
		return nullptr;
	}

	BitmapFont* bitmapFont = new BitmapFont(bitmapFontFilePathWithNoExtension, *bitmapTexture);
	if (!bitmapFont)
	{
		ERROR_AND_DIE(Stringf("Unkown bitmapFont"));
	}

	m_loadedFonts.push_back(bitmapFont);

	return bitmapFont;
}

Texture* DX12Renderer::GetTextureByFileName(char const* imageFilePath)
{
	for (int i = 0; i < m_loadedTextures.size(); i++)
	{
		if (m_loadedTextures[i] && m_loadedTextures[i]->GetImageFilePath() == imageFilePath)
		{
			return m_loadedTextures[i];
		}
	}
	return nullptr;
}

Image* DX12Renderer::CreateImageFromFile(char const* filePath)
{
	return new Image(filePath);
}

Texture* DX12Renderer::CreateTextureFromImage(Image const& image)
{
	Texture* newTexture = new Texture();
	newTexture->m_dimensions = image.GetDimensions();
	
	newTexture->m_textureDescIndex = (int)m_loadedTextures.size();

	GUARANTEE_OR_DIE(newTexture->m_textureDescIndex < 200, Stringf("Cannot create more than %d textures!", 200));

	IntVec2 dims = image.GetDimensions();
	int rowPitch = dims.x * 4; // 4 bytes per pixel (RGBA8)
	std::vector<unsigned char> flippedData(rowPitch * dims.y);
	unsigned char const* src = static_cast<unsigned char const*>(image.GetRawData());
	for (int y = 0; y < dims.y; ++y)
	{
		memcpy(&flippedData[y * rowPitch], &src[(dims.y - 1 - y) * rowPitch], rowPitch);
	}
	
	D3D12_RESOURCE_DESC resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	resourceDescription.Width = newTexture->m_dimensions.x; // width of the texture
	resourceDescription.Height = newTexture->m_dimensions.y; // height of the texture
	resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	resourceDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // This is the dxgi format of the image (format of the pixels)
	resourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	resourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

	D3D12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
	HRESULT hr = m_device->CreateCommittedResource(
		&properties, // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&resourceDescription, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&newTexture->m_dx12Texture));

	std::wstring texName = L"Texture_" + std::to_wstring(newTexture->m_textureDescIndex);
	newTexture->m_dx12Texture->SetName(texName.c_str());

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Cannot create texture committed resource!");

	// upload the texture to GPU; store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = flippedData.data();
	textureData.RowPitch = image.GetDimensions().x * 32 / 8; // size of all our triangle vertex data
	textureData.SlicePitch = image.GetDimensions().x * image.GetDimensions().y; // also the size of our triangle vertex data

	GUARANTEE_OR_DIE(newTexture->m_dx12Texture != nullptr, "Cannot create texture!");
	
	UINT64 textureUploadBufferSize; 
	m_device->GetCopyableFootprints(&resourceDescription, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
	// now we create an upload heap to upload our texture to the GPU
	hr = m_device->CreateCommittedResource(
		&properties, // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&resourceDesc, // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&newTexture->m_textureBufferUploadHeap));
	std::wstring upName = L"TextureUpload_" + std::to_wstring(newTexture->m_textureDescIndex);
	newTexture->m_textureBufferUploadHeap->SetName(upName.c_str());

	UpdateSubresources(m_commandList, newTexture->m_dx12Texture, newTexture->m_textureBufferUploadHeap, 0, 0, 1, &textureData);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(newTexture->m_dx12Texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	m_commandList->ResourceBarrier(1, &barrier);
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
	                                               NUM_CONSTANT_BUFFERS +
	                                               newTexture->m_textureDescIndex, m_scuDescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_device->CreateShaderResourceView(newTexture->m_dx12Texture, &srvDesc, descriptorHandle);

	//m_loadedTextures.push_back(newTexture);
	return newTexture;
}

void DX12Renderer::PushBackNewTextureManually(Texture* const tex)
{
	m_loadedTextures.push_back(tex);
}

void DX12Renderer::BindTexture(const Texture* texture, int slot)
{
	if (texture == nullptr)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
		                                               NUM_CONSTANT_BUFFERS + m_defaultTexture->
		                                               m_textureDescIndex, m_scuDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(slot+NUM_CONSTANT_BUFFERS, descriptorHandle);
	}
	else
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
		                                               NUM_CONSTANT_BUFFERS + texture->
		                                               m_textureDescIndex, m_scuDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(slot+NUM_CONSTANT_BUFFERS, descriptorHandle);
	}
	m_currentTexture = texture;

	//  Descriptor Heap Layout(CBV + SRV) :
	//	+-------------------------- + ------------------------------ - +
	//	| Constant Buffers(CBVs)	| Textures(SRVs)				  |
	//	| 0 1 2 ... N - 1			| [0] = white[1] = diffuseMap ... |
	//	+-------------------------- + ------------------------------ - +
	//				^							^
	//				|                           |
	//	m_numCBs * drawCall			+	texture->m_textureIndex
}

void DX12Renderer::SetBlendMode(BlendMode blendMode)
{
	m_desiredBlendMode = blendMode;
}

D3D12_BLEND_DESC MakeBlendDesc(BlendMode mode)
{
	D3D12_BLEND_DESC blendDesc = {};
	if (mode == BlendMode::ADDITIVE)
	{
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
		blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
		blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	else if (mode == BlendMode::ALPHA)
	{
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		//blendDesc.RenderTarget[0].LogicOpEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
		blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
		blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	else //if (mode == BlendMode::OPAQUE)
	{
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = blendDesc.RenderTarget[0].SrcBlend;
		blendDesc.RenderTarget[0].DestBlendAlpha = blendDesc.RenderTarget[0].DestBlend;
		blendDesc.RenderTarget[0].BlendOpAlpha = blendDesc.RenderTarget[0].BlendOp;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	return blendDesc;
}

D3D12_RASTERIZER_DESC MakeRasterizerDesc(RasterizerMode mode)
{
	D3D12_RASTERIZER_DESC rasterizerDesc = {};
	if (mode == RasterizerMode::SOLID_CULL_NONE)
	{
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizerDesc.FrontCounterClockwise = true;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.f;
		rasterizerDesc.SlopeScaledDepthBias = 0.f;
		rasterizerDesc.DepthClipEnable = true;
		rasterizerDesc.MultisampleEnable = false;
		rasterizerDesc.AntialiasedLineEnable = true;
	}
	else if (mode == RasterizerMode::SOLID_CULL_BACK)
	{
		rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = true;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.f;
		rasterizerDesc.SlopeScaledDepthBias = 0.f;
		rasterizerDesc.DepthClipEnable = true;
		rasterizerDesc.MultisampleEnable = false;
		rasterizerDesc.AntialiasedLineEnable = true;
	}
	else if (mode == RasterizerMode::WIREFRAME_CULL_BACK)
	{
		rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = true;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.f;
		rasterizerDesc.SlopeScaledDepthBias = 0.f;
		rasterizerDesc.DepthClipEnable = true;
		rasterizerDesc.MultisampleEnable = false;
		rasterizerDesc.AntialiasedLineEnable = true;
	}
	else if (mode == RasterizerMode::WIREFRAME_CULL_NONE)
	{
		rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterizerDesc.FrontCounterClockwise = true;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.f;
		rasterizerDesc.SlopeScaledDepthBias = 0.f;
		rasterizerDesc.DepthClipEnable = true;
		rasterizerDesc.MultisampleEnable = false;
		rasterizerDesc.AntialiasedLineEnable = true;
	}
	return rasterizerDesc;
}

D3D12_DEPTH_STENCIL_DESC MakeDepthStencilDesc(DepthMode mode)
{
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};

	if (mode == DepthMode::DISABLED)
	{
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	}
	else
	{
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	}

	return depthStencilDesc;
}

void DX12Renderer::SetGraphicsStatesIfChanged()
{
	if (m_currentRasterizerMode == m_desiredRasterizerMode && m_currentBlendMode == m_desiredBlendMode
		&& m_currentDepthMode == m_desiredDepthMode && m_currentSamplerMode == m_desiredSamplerMode
		&& m_currentShader == m_desiredShader)
	{
		m_commandList->SetPipelineState(m_pipelineStateObject);
		return;
	}
	m_currentRasterizerMode = m_desiredRasterizerMode;
	m_currentBlendMode = m_desiredBlendMode;
	m_currentDepthMode = m_desiredDepthMode;
	m_currentSamplerMode = m_desiredSamplerMode;
	m_currentShader = m_desiredShader;

	int indexNum = (m_desiredShader->m_shaderIndex << 16) | ((int)m_desiredDepthMode << 12)
		| ((int)m_desiredSamplerMode << 8) | ((int)m_desiredRasterizerMode << 4) | ((int)m_desiredBlendMode);

	if (m_currentRenderMode == RenderMode::FORWARD)
	{
		auto iter = m_pipelineStatesConfiguration.find(indexNum);
		if (iter != m_pipelineStatesConfiguration.end())
		{
			m_commandList->SetPipelineState(iter->second);
			m_pipelineStateObject = iter->second;
			return;
		}
	}
	if (m_currentRenderMode == RenderMode::GI)
	{
		auto iter = m_gBufferPipelineStatesConfiguration.find(indexNum);
		if (iter != m_gBufferPipelineStatesConfiguration.end())
		{
			m_commandList->SetPipelineState(iter->second);
			m_pipelineStateObject = iter->second;
			return;
		}
	}
	
	ID3D12PipelineState* pipelineState = nullptr;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = *m_desiredShader->m_inputLayoutForVertex;
	psoDesc.pRootSignature = m_graphicsRootSignature;
	psoDesc.VS =
	{
		reinterpret_cast<UINT8*>(m_desiredShader->m_dx12VertexShader->GetBufferPointer()),
		m_desiredShader->m_dx12VertexShader->GetBufferSize()
	};
	psoDesc.PS =
	{
		reinterpret_cast<UINT8*>(m_desiredShader->m_dx12PixelShader->GetBufferPointer()),
		m_desiredShader->m_dx12PixelShader->GetBufferSize()
	};
	psoDesc.RasterizerState = MakeRasterizerDesc(m_desiredRasterizerMode);
	psoDesc.BlendState = MakeBlendDesc(m_desiredBlendMode);
	psoDesc.DepthStencilState = MakeDepthStencilDesc(m_desiredDepthMode);
	//psoDesc.SampleDesc 因为没有multi sample，不用设置，sample count在创建swapchain时设置为1了
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; //需要深度
	psoDesc.SampleDesc.Count = 1;
	if (m_currentRenderMode ==RenderMode::FORWARD)
	{
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	if (m_currentRenderMode == RenderMode::GI) //4个RTV+1个DSV（m_depth）
	{
		psoDesc.NumRenderTargets = 4;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  
		psoDesc.RTVFormats[1] = DXGI_FORMAT_R10G10B10A2_UNORM;
		psoDesc.RTVFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.RTVFormats[3] = DXGI_FORMAT_R16G16_FLOAT;
	}
	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 Cannot Create Graphics Pipeline State!");
	std::wstring debugName = L"PSO_" 
    + std::wstring(m_desiredShader->m_config.m_name.begin(), m_desiredShader->m_config.m_name.end())
    + L"_Rs" + std::to_wstring((int)m_desiredRasterizerMode)
    + L"_Bl" + std::to_wstring((int)m_desiredBlendMode)
    + L"_Dp" + std::to_wstring((int)m_desiredDepthMode)
    + L"_Sp" + std::to_wstring((int)m_desiredSamplerMode)
	+ L"_NUMRTV" + std::to_wstring((int)psoDesc.NumRenderTargets);

	pipelineState->SetName(debugName.c_str());

	m_commandList->SetPipelineState(pipelineState);
	if (m_currentRenderMode == RenderMode::FORWARD)
		m_pipelineStatesConfiguration[indexNum] = pipelineState;
	if (m_currentRenderMode == RenderMode::GI)
		m_gBufferPipelineStatesConfiguration[indexNum] = pipelineState;
	
	m_pipelineStateObject = pipelineState;
}

void DX12Renderer::CreateDepthSRV()
{
	// 用depthBuffer创建
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;		//SRV格式与DSV格式不同
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
		m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		NUM_CONSTANT_BUFFERS + MAX_TEXTURE_COUNT + GBUFFER_COUNT,  // 在GBuffer SRV之后
		m_scuDescriptorSize
	);

	m_device->CreateShaderResourceView(
		m_depthStencilBuffer, 
		&srvDesc,
		srvHandle
	);
}

void DX12Renderer::CreateGBufferResources()
{
	D3D12_CLEAR_VALUE albedoClearValue = {};
	albedoClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	albedoClearValue.Color[0] = 0.0f; // R
	albedoClearValue.Color[1] = 0.0f; // G
	albedoClearValue.Color[2] = 0.0f; // B
	albedoClearValue.Color[3] = 1.0f; // A

	D3D12_CLEAR_VALUE normalClearValue = {};
	normalClearValue.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	normalClearValue.Color[0] = 0.5f; // X
	normalClearValue.Color[1] = 0.5f; // Y
	normalClearValue.Color[2] = 1.0f; // Z
	normalClearValue.Color[3] = 1.0f; // A

	D3D12_CLEAR_VALUE materialClearValue = {};
	materialClearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	materialClearValue.Color[0] = 1.0f; // R
	materialClearValue.Color[1] = 0.0f; // G
	materialClearValue.Color[2] = 1.0f; // B
	materialClearValue.Color[3] = 0.0f; // A

	D3D12_CLEAR_VALUE motionClearValue = {};
	motionClearValue.Format = DXGI_FORMAT_R16G16_FLOAT;
	motionClearValue.Color[0] = 0.0f; // X
	motionClearValue.Color[1] = 0.0f; // Y
	motionClearValue.Color[2] = 0.0f; // Z
	motionClearValue.Color[3] = 0.0f; // A

	m_gBuffer.m_gBufferClearValues[0] = albedoClearValue;
	m_gBuffer.m_gBufferClearValues[1] = normalClearValue;
	m_gBuffer.m_gBufferClearValues[2] = materialClearValue;
	m_gBuffer.m_gBufferClearValues[3] = motionClearValue;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = m_config.m_window->GetClientDimensions().x;
	desc.Height = m_config.m_window->GetClientDimensions().y;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    
	HRESULT hr;
	// Albedo
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	hr = m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &albedoClearValue,
		IID_PPV_ARGS(&m_gBuffer.m_albedo));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create GBuffer albedo!")
	
		m_gBuffer.m_albedo->SetName(L"GBufferAlbedo");
    
	// Normal
	desc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
	hr = m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &normalClearValue,
		IID_PPV_ARGS(&m_gBuffer.m_normal));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create GBuffer normal!");
	m_gBuffer.m_normal->SetName(L"GBufferNormal");
    
	// Material
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	hr = m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &materialClearValue,
		IID_PPV_ARGS(&m_gBuffer.m_material));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create GBuffer material!");
	m_gBuffer.m_material->SetName(L"GBufferMaterial");
    
	// Motion Vectors
	desc.Format = DXGI_FORMAT_R16G16_FLOAT;
	hr = m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &motionClearValue,
		IID_PPV_ARGS(&m_gBuffer.m_motion));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create GBuffer normal!");
	m_gBuffer.m_motion->SetName(L"GBufferMotion");

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
		m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		NUM_CONSTANT_BUFFERS + MAX_TEXTURE_COUNT,  // cbvsrv描述符堆中的实际位置
		m_scuDescriptorSize
	);

	// Create srv for GBuffer
	for (int i = 0; i < GBUFFER_COUNT; ++i)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		switch (i)
		{
		case 0: // Albedo
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case 1: // Normal  
			srvDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
			break;
		case 2: // Material
			srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		case 3: // Motion
			srvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
			break;
		}

		m_device->CreateShaderResourceView(
			m_gBuffer.GetResource(i),
			&srvDesc,
			srvHandle
		);

		srvHandle.Offset(1, m_scuDescriptorSize);
	}
	//创建完GBuffer的ID3D12Resources后补上它们的rtv
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.Offset(FRAME_BUFFER_COUNT, m_rtvDescriptorSize);
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	// 建议为每个 GBuffer 明确 RTV 格式，避免误用 sRGB（GBuffer 通常线性）
	// Albedo
	{
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		m_device->CreateRenderTargetView(m_gBuffer.m_albedo, &rtvDesc, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// Normal
	{
		rtvDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		m_device->CreateRenderTargetView(m_gBuffer.m_normal, &rtvDesc, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// Material
	{
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		m_device->CreateRenderTargetView(m_gBuffer.m_material, &rtvDesc, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	// Motion
	{
		rtvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		m_device->CreateRenderTargetView(m_gBuffer.m_motion, &rtvDesc, rtvHandle);
	}

	ShaderConfig cfg;
	cfg.m_name = "DefaultGBufferShader";
	cfg.m_vertexEntryPoint = "GBufferVS";
	cfg.m_pixelEntryPoint = "GBufferPS";

	m_defaultGBufferShader = new Shader(cfg);

	ID3DBlob* vs = nullptr;
	ID3DBlob* ps = nullptr;

	CompileShaderToByteCode(&vs, "GBufferVS", m_gBufferShaderSource, cfg.m_vertexEntryPoint.c_str(), "vs_5_0");  //TODO: æ”¹GBuffer
	CompileShaderToByteCode(&ps, "GBufferPS", m_gBufferShaderSource, cfg.m_pixelEntryPoint.c_str(), "ps_5_1");

	static D3D12_INPUT_ELEMENT_DESC kPCUTBN[] = {
		{ "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,                           D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",     0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	m_defaultGBufferShader->m_dx12VertexShader = vs;
	m_defaultGBufferShader->m_dx12PixelShader = ps;
	m_defaultGBufferShader->m_inputLayoutForVertex = new D3D12_INPUT_LAYOUT_DESC();
	m_defaultGBufferShader->m_inputLayoutForVertex->pInputElementDescs = kPCUTBN;
	m_defaultGBufferShader->m_inputLayoutForVertex->NumElements = _countof(kPCUTBN);

	m_defaultGBufferShader->m_shaderIndex = (int)m_loadedShaders.size();

	GUARANTEE_OR_DIE(m_defaultGBufferShader->m_shaderIndex <= 65535, "Cannot create more than 65535 shaders!");
	m_loadedShaders.push_back(m_defaultGBufferShader);
}

void DX12Renderer::BeginGBufferPass()
{
	if (m_currentActivePass != ActivePass::GBUFFER)
	{
		CD3DX12_RESOURCE_BARRIER barriers[GBUFFER_COUNT];
		for (int i = 0; i < GBUFFER_COUNT; ++i)
		{
			barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
				m_gBuffer.GetResource(i),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);
		}
		/*barriers[GBUFFER_COUNT] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_depthStencilBuffer,
			D3D12_RESOURCE_STATE_DEPTH_READ |
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);*/
		m_commandList->ResourceBarrier(GBUFFER_COUNT, barriers);
	}

	PassModeConstants modeConstants;
	modeConstants.RenderMode =static_cast<uint32_t>(RenderMode::GI);
	m_constantBuffers[k_passConstantsSlot]->AppendData(&modeConstants, sizeof(PassModeConstants), m_currentDrawIndex);
	BindConstantBuffer(k_passConstantsSlot, m_constantBuffers[k_passConstantsSlot]);

	m_currentActivePass = ActivePass::GBUFFER;
	m_gBufferPassActive = true;
}

void DX12Renderer::ClearGBufferPassRTV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[GBUFFER_COUNT];
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE it(
			m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			FRAME_BUFFER_COUNT,
			m_rtvDescriptorSize
		);
		for (int i = 0; i < GBUFFER_COUNT; ++i)
		{
			rtvHandles[i] = it;
			it.Offset(1, m_rtvDescriptorSize);
		}
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	m_commandList->OMSetRenderTargets(GBUFFER_COUNT, rtvHandles, FALSE, &dsvHandle);

	for (int i = 0; i < GBUFFER_COUNT; ++i)
	{
		m_commandList->ClearRenderTargetView(rtvHandles[i], m_gBuffer.m_gBufferClearValues[i].Color, 0, nullptr);
	}
	m_commandList->ClearDepthStencilView(
		dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.0f, 0, 0, nullptr
	);
}

void DX12Renderer::EndGBufferPass()
{
	//ExtractGBufferToSurfaceCache();
	BeginCardCapturePass();
	EndCardCapturePass();
	//BeginRadianceCachePass();

	CompositeFinalImage();

	if (m_debugVisualizeSurfaceCache)
	{
		VisualizeSurfaceCache();
	}

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthStencilBuffer,
		D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_WRITE  // 恢复为可写
	);
	m_commandList->ResourceBarrier(1, &barrier);
	m_gBufferPassActive = false;

	for (int i = 0; i < SURFACE_CACHE_TYPE_COUNT; i++)
	{
		m_surfaceCaches[i].SwapBuffers();
	}

	m_radianceCache.SwapBuffers();  //TODO 暂时还只有一层
}

void DX12Renderer::UnbindGBufferPassRT()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_frameIndex,
		m_rtvDescriptorSize
	);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
}

void DX12Renderer::CreateSDFGenerationRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE ranges[2];
    
	// [0] = Input SRVs (t0-t3): Vertex, Index, BVHNode, BVHTriIndices
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);
    
	// [1] = Output UAV (u0): SDF Texture3D
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
    
	CD3DX12_ROOT_PARAMETER params[3];
	params[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);
	params[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
	params[2].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init(3, params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    
	ID3DBlob* signature = nullptr;
	ID3DBlob* error = nullptr;
    
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error
	);
    
	if (FAILED(hr))
	{
		if (error)
		{
			DebuggerPrintf("[DX12] SDF Root Signature error: %s\n", 
						  (char*)error->GetBufferPointer());
			error->Release();
		}
		GUARANTEE_OR_DIE(false, "Failed to serialize SDF root signature!");
	}
    
	hr = m_device->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&m_sdfGenerationRootSignature)
	);
    
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create SDF root signature!");
	m_sdfGenerationRootSignature->SetName(L"SDFGenerationRootSig");
    
	if (signature) signature->Release();
}

void DX12Renderer::CreateSDFGenerationPSO()
{
	std::string shaderSource;
	int result = FileReadToString(shaderSource, "Data/Shaders/SDFGeneration.hlsl");
	GUARANTEE_OR_DIE(result >= 0, "Failed to load SDFGeneration.hlsl");
    
	ID3DBlob* cs = nullptr;
	CompileShaderToByteCode(&cs, "SDFGenerate", shaderSource.c_str(), "main", "cs_5_1");
    
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	desc.pRootSignature = m_sdfGenerationRootSignature;
	desc.CS = { cs->GetBufferPointer(), cs->GetBufferSize() };
    
	HRESULT hr = m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_sdfGenerationPSO));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create SDF generation PSO!");
    
	m_sdfGenerationPSO->SetName(L"SDFGenerationPSO");
	cs->Release();
}

SDFTexture3D* DX12Renderer::GenerateSDFOnGPU(const std::vector<Vertex_PCUTBN>& vertices,
	const std::vector<uint32_t>& indices, const BVH& bvh, const AABB3& bounds, int resolution)
{
	SDFTexture3D* sdf = new SDFTexture3D(resolution);
	DebuggerPrintf("[DX12] Starting GPU SDF generation (resolution=%d)\n", resolution);
    
    std::vector<GPUBVHNode> bvhNodes;
    std::vector<uint32_t> bvhTriIndices;
    bvh.FlattenForGPU(bvhNodes, bvhTriIndices);
    
    if (bvhNodes.empty())
    {
        DebuggerPrintf("[DX12] BVH is empty, cannot generate SDF\n");
        return nullptr;
    }
    
    DebuggerPrintf("[DX12] SDF Gen: %zu verts, %zu indices, %zu BVH nodes\n",
                   vertices.size(), indices.size(), bvhNodes.size());
    
    ID3D12Resource* vertexBuffer = CreateStructuredBuffer(sdf, 
        vertices.data(), 
        vertices.size(),
        sizeof(Vertex_PCUTBN)
    ,
        L"SDF_VertexBuffer"
    );
    ID3D12Resource* indexBuffer = CreateStructuredBuffer(sdf,
        indices.data(),
        indices.size(),
        sizeof(uint32_t)
    ,
        L"SDF_IndexBuffer"
    );
    ID3D12Resource* bvhNodeBuffer = CreateStructuredBuffer(sdf,
        bvhNodes.data(),
        bvhNodes.size(),
        sizeof(GPUBVHNode)
    ,
        L"SDF_BVHNodeBuffer"
    );
    ID3D12Resource* bvhTriBuffer = CreateStructuredBuffer(sdf,
        bvhTriIndices.data(),
        bvhTriIndices.size(),
        sizeof(uint32_t)
    ,
        L"SDF_BVHTriIndicesBuffer"
    );
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(
        m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        SDF_GEN_VERTEX_SRV,
        m_scuDescriptorSize
    );
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.StructureByteStride = sizeof(Vertex_PCUTBN);
    srvDesc.Buffer.NumElements = (UINT)vertices.size();
    m_device->CreateShaderResourceView(vertexBuffer, &srvDesc, srvHandle);
    
    srvHandle.Offset(1, m_scuDescriptorSize);
    srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
    srvDesc.Buffer.NumElements = (UINT)indices.size();
    m_device->CreateShaderResourceView(indexBuffer, &srvDesc, srvHandle);
    
    srvHandle.Offset(1, m_scuDescriptorSize);
    srvDesc.Buffer.StructureByteStride = sizeof(GPUBVHNode);
    srvDesc.Buffer.NumElements = (UINT)bvhNodes.size();
    m_device->CreateShaderResourceView(bvhNodeBuffer, &srvDesc, srvHandle);
    
    srvHandle.Offset(1, m_scuDescriptorSize);
    srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
    srvDesc.Buffer.NumElements = (UINT)bvhTriIndices.size();
    m_device->CreateShaderResourceView(bvhTriBuffer, &srvDesc, srvHandle);
    
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    texDesc.Width = resolution;
    texDesc.Height = resolution;
    texDesc.DepthOrArraySize = (UINT16)resolution;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;  // ✅ UAV
    
    CD3DX12_HEAP_PROPERTIES defaultProps(D3D12_HEAP_TYPE_DEFAULT);
    
    HRESULT hr = m_device->CreateCommittedResource(
        &defaultProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,  // ✅ 初始状态
        nullptr,
        IID_PPV_ARGS(&sdf->m_sdfTexture3D)
    );
    
    GUARANTEE_OR_DIE(SUCCEEDED(hr), "Create SDF Texture3D failed");
    sdf->m_sdfTexture3D->SetName(L"SDFTexture3D");
    
    // 创建临时UAV（固定slot，所有SDF生成共用）
    CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(
        m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        SDF_GEN_OUTPUT_UAV,
        m_scuDescriptorSize
    );
    
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice = 0;
    uavDesc.Texture3D.FirstWSlice = 0;
    uavDesc.Texture3D.WSize = resolution;
    m_device->CreateUnorderedAccessView(sdf->m_sdfTexture3D, nullptr, &uavDesc, uavHandle);
    
    // 6. Dispatch Compute Shader
    m_commandList->SetPipelineState(m_sdfGenerationPSO);
    m_commandList->SetComputeRootSignature(m_sdfGenerationRootSignature);
    
    ID3D12DescriptorHeap* heaps[] = { m_cbvSrvDescHeap };
    m_commandList->SetDescriptorHeaps(1, heaps);
    
    // 绑定SRVs (t0-t3)
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle(
        m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
        SDF_GEN_VERTEX_SRV,
        m_scuDescriptorSize
    );
    m_commandList->SetComputeRootDescriptorTable(0, gpuSrvHandle);
    
    // 绑定UAV (u0)
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuUavHandle(
        m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
        SDF_GEN_OUTPUT_UAV,
        m_scuDescriptorSize
    );
    m_commandList->SetComputeRootDescriptorTable(1, gpuUavHandle);
    
    // 绑定Constants
    SDFGenerationConstants constants;
    constants.BoundsMin = bounds.m_mins;
    constants.Resolution = resolution;
    constants.BoundsMax = bounds.m_maxs;
    constants.NumTriangles = (uint32_t)(indices.size() / 3);
	
	m_constantBuffers[k_sdfGenerationConstantsSlot]->AppendData(&constants, sizeof(SDFGenerationConstants), m_currentDrawIndex);
	D3D12_GPU_VIRTUAL_ADDRESS constantsBufferAddress = m_constantBuffers[k_sdfGenerationConstantsSlot]->m_dx12ConstantBuffer->GetGPUVirtualAddress()
	+ m_constantBuffers[k_sdfGenerationConstantsSlot]->m_offset;
    m_commandList->SetComputeRootConstantBufferView(2, constantsBufferAddress);
    
    // Dispatch
    int numGroups = (resolution + 7) / 8;
    m_commandList->Dispatch(numGroups, numGroups, numGroups);
    
    DebuggerPrintf("[DX12] Dispatched SDF generation: %dx%dx%d groups\n", 
                   numGroups, numGroups, numGroups);
    
    // 7. UAV Barrier
    CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(sdf->m_sdfTexture3D);
    m_commandList->ResourceBarrier(1, &uavBarrier);
    
    // 8. Transition到SRV
    auto toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
        sdf->m_sdfTexture3D,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    m_commandList->ResourceBarrier(1, &toSRV);
    
	m_currentFrameTempResources.push_back(vertexBuffer);
	m_currentFrameTempResources.push_back(indexBuffer);
	m_currentFrameTempResources.push_back(bvhNodeBuffer);
	m_currentFrameTempResources.push_back(bvhTriBuffer);
    
    if (m_nextSDFDescriptorIndex >= SDF_TEXTURE_SRV_BASE + MAX_SDF_TEXTURE_COUNT)
    {
        DebuggerPrintf("[DX12] SDF descriptor pool exhausted!\n");
        delete sdf;
        return nullptr;
    }
    
    sdf->m_srvHeapIndex = m_nextSDFDescriptorIndex++;
    
    CD3DX12_CPU_DESCRIPTOR_HANDLE finalSrvHandle(
        m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        sdf->m_srvHeapIndex,
        m_scuDescriptorSize
    );
    
    D3D12_SHADER_RESOURCE_VIEW_DESC finalSrvDesc = {};
    finalSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    finalSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    finalSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    finalSrvDesc.Texture3D.MipLevels = 1;
    finalSrvDesc.Texture3D.MostDetailedMip = 0;
    
    m_device->CreateShaderResourceView(sdf->m_sdfTexture3D, &finalSrvDesc, finalSrvHandle);
    
    m_loadedSDFs.push_back(sdf);
    DebuggerPrintf("[DX12] SDF registered, SRV index: %d\n", sdf->m_srvHeapIndex);
    return sdf;
}

ID3D12Resource* DX12Renderer::CreateStructuredBuffer(SDFTexture3D* sdfOwner, const void* data, size_t numElements, size_t elementSize,
	const wchar_t* debugName)
{
	UINT64 bufferSize = numElements * elementSize;
    
	// 1. 创建Default Heap buffer
	CD3DX12_HEAP_PROPERTIES defaultProps(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_NONE);
    
	ID3D12Resource* buffer = nullptr;
	HRESULT hr = m_device->CreateCommittedResource(
		&defaultProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&buffer)
	);
    
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Create structured buffer failed");
	buffer->SetName(debugName);
    
	// 2. 创建Upload buffer
	CD3DX12_HEAP_PROPERTIES uploadProps(D3D12_HEAP_TYPE_UPLOAD);
	auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
    
	ID3D12Resource* uploadBuffer = nullptr;
	hr = m_device->CreateCommittedResource(
		&uploadProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)
	);
    
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Create upload buffer failed");
    
	// 3. 上传数据
	void* mapped = nullptr;
	uploadBuffer->Map(0, nullptr, &mapped);
	memcpy(mapped, data, bufferSize);
	uploadBuffer->Unmap(0, nullptr);
    
	// 4. 拷贝到Default heap
	m_commandList->CopyResource(buffer, uploadBuffer);
    
	// 5. Transition到SRV可读
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		buffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(1, &barrier);
	uploadBuffer->SetName(L"SDFStructureUploadBuffer");
    
	m_currentFrameTempResources.push_back(uploadBuffer);
	if (sdfOwner)
	{
		sdfOwner->m_tempBuffers.push_back(uploadBuffer);
		sdfOwner->m_tempBuffers.push_back(buffer);  // default heap buffer也存
	}

	return buffer;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Renderer::GetSRVHandle(uint32_t index)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(
		m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
		index,
		m_scuDescriptorSize
	);
}

void DX12Renderer::CreateGraphicsRootSignature()
{
	HRESULT hr;
	// Create a root signature. 
	{
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		//共 15 个 Root Parameter（14个CBV + 1个SRV Descriptor Table） ->Thesis extension: 32 parameters
		CD3DX12_ROOT_PARAMETER rootParameters[32] = {};

		// [0~13] = Root CBVs -> b0 ~ b13
		for (UINT i = 0; i < 14; ++i)
		{
			rootParameters[i].InitAsConstantBufferView(i, 0, D3D12_SHADER_VISIBILITY_ALL); // register(b#), space 0
		}
		// [14] = SRV Descriptor Table for t0~
		CD3DX12_DESCRIPTOR_RANGE textureSRV(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, MAX_TEXTURE_COUNT, 0); // up to t0~t199
		rootParameters[14].InitAsDescriptorTable(1, &textureSRV, D3D12_SHADER_VISIBILITY_PIXEL);
		// [15] = GBuffer SRVs
		CD3DX12_DESCRIPTOR_RANGE gbufferSRVs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GBUFFER_COUNT + DEPTH_SRV_COUNT, GBUFFER_SRV_START); 
		rootParameters[15].InitAsDescriptorTable(1, &gbufferSRVs, D3D12_SHADER_VISIBILITY_ALL);
		// [16] = Surface Cache SRVs
		static CD3DX12_DESCRIPTOR_RANGE surfaceCacheSRVs;
		surfaceCacheSRVs.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		                      SURFACE_CACHE_BUFFER_COUNT * (DESCRIPTORS_PER_SURFACE_BUFFER/2), SURFACE_CACHE_SRV, 0);
		rootParameters[16].InitAsDescriptorTable(1, &surfaceCacheSRVs, D3D12_SHADER_VISIBILITY_ALL);

		// [17] = Surface Cache UAVs (u0 + u1)
		static CD3DX12_DESCRIPTOR_RANGE surfaceCacheUAVs;
		surfaceCacheUAVs.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
		                      SURFACE_CACHE_BUFFER_COUNT * (DESCRIPTORS_PER_SURFACE_BUFFER/2), 0, 0);  // u0, u1
		rootParameters[17].InitAsDescriptorTable(1, &surfaceCacheUAVs, D3D12_SHADER_VISIBILITY_ALL);
		// u0, u1, u2, u3, u4, u5（如果包含 Buffer 1 的 UAVs） 那就是6
		
		// [18-19] = Radiance Cache 
		rootParameters[18].InitAsShaderResourceView(RADIANCE_CACHE_SRV, 0); 
		rootParameters[19].InitAsUnorderedAccessView(5, 0);  // u5
		// [20-21] = Probe Grid 
		rootParameters[20].InitAsShaderResourceView(PROBE_GRID_SRV_INDEX, 0); 
		rootParameters[21].InitAsConstantBufferView(16, 0);  // b16 - Probe settings TODO:改
		// [22-23] = SSR
		rootParameters[22].InitAsShaderResourceView(SSR_SRV_INDEX, 0); 
		rootParameters[23].InitAsConstantBufferView(17, 0);  // b17 - SSR settings TODO:改
		//[24-25] = Temporal
		rootParameters[24].InitAsShaderResourceView(TEMPORAL_PREV_SRV_INDEX, 0); 
		rootParameters[25].InitAsShaderResourceView(TEMPORAL_MOTION_SRV_INDEX, 0); 
		// // [26-31] = reserved
		for (UINT i = 26; i < 32; ++i)
		{
			rootParameters[i].InitAsConstantBufferView(18 + (i-26), 0);
		}
        
		D3D12_STATIC_SAMPLER_DESC samplers[2] = {};
    
		// Sampler 0: Point Sampler (s0) - 用于 GBuffer、精确采样
		samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;  // 改为 CLAMP，避免边界问题
		samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[0].MipLODBias = 0;
		samplers[0].MaxAnisotropy = 0;
		samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplers[0].MinLOD = 0.0f;
		samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
		samplers[0].ShaderRegister = 0;  // s0
		samplers[0].RegisterSpace = 0;
		samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
		// Sampler 1: Linear Sampler (s1) - 用于 Surface Cache、Radiance Cache
		samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		samplers[1].MipLODBias = 0;
		samplers[1].MaxAnisotropy = 0;
		samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplers[1].MinLOD = 0.0f;
		samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
		samplers[1].ShaderRegister = 1;  
		samplers[1].RegisterSpace = 0;
		samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    
		CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};
		rsigDesc.Init(
			_countof(rootParameters),
			rootParameters,
			2,  // 2 samplers
			samplers, 
			rootSignatureFlags
		);
		
		ID3DBlob* signature;
		ID3DBlob* error;
		hr = D3D12SerializeRootSignature(&rsigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
		if (!SUCCEEDED(hr))
		{
			if (error != NULL)
			{
				DebuggerPrintf((char*)error->GetBufferPointer());
			}
		}
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 Cannot Serialize Root Signature!");

		hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
		                                   IID_PPV_ARGS(&m_graphicsRootSignature));
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 Cannot Create Root Signature!");
		m_graphicsRootSignature->SetName(L"OnlyRS");

		if (signature) signature->Release();
		if (error) error->Release();
	}
}

void DX12Renderer::CreateComputeRootSignature()
{
    static CD3DX12_DESCRIPTOR_RANGE descriptorRanges[11];

    // [0] GBuffer SRVs (t0-t4)
    descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, GBUFFER_COUNT + DEPTH_SRV_COUNT, 0, 0); // t0-t4

    // [1] Surface Cache Atlas UAV (u0-u1) - PRIMARY + GI
    descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, SURFACE_CACHE_TYPE_COUNT, 0, 0); // u0-u1

    // [2] Surface Cache Metadata UAV (u2-u3)
    descriptorRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, SURFACE_CACHE_TYPE_COUNT, SURFACE_CACHE_TYPE_COUNT, 0); // u2-u3

    // [3] Radiance Cache Probe UAV (u4)
    descriptorRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, SURFACE_CACHE_TYPE_COUNT * 2, 0); // u4

    // [4] Additional UAVs (u5-u6) - 预留
    descriptorRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, SURFACE_CACHE_TYPE_COUNT * 2 + 1, 0); // u5-u6

    // [5] Previous Surface Cache Atlas SRV (t5)
    descriptorRanges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, PREVIOUS_SURFACE_CACHE_RESOURCE_COUNT,
        GBUFFER_COUNT + DEPTH_SRV_COUNT, 0); // t5

    // [6] Card Metadata SRV (t6)
    descriptorRanges[6].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1,
        GBUFFER_COUNT + DEPTH_SRV_COUNT + 1, 0); // t6

    // [7] SDF Volume SRV (t10)
    descriptorRanges[7].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 10, 0); // t10

    // [8] Surface Cache Atlas SRV for Radiance Cache (t8-t9)
    // 这是给 Radiance Cache Update 读取 Surface Cache 用的
    descriptorRanges[8].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SURFACE_CACHE_TYPE_COUNT, 8, 0); 

    // [9] Radiance Cache Probe SRV (Previous frame, t7)
    descriptorRanges[9].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 7, 0);

    // [10] Card BVH SRVs (t11 - t12)
    descriptorRanges[10].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 11, 0); // t240: BVH Nodes, t241: BVH Indices

    CD3DX12_ROOT_PARAMETER computeRootParameters[15] = {};

    // [0] = GBuffer SRVs descriptor table
    computeRootParameters[0].InitAsDescriptorTable(1, &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

    // [1] = Surface Cache Atlas UAV descriptor table
    computeRootParameters[1].InitAsDescriptorTable(1, &descriptorRanges[1], D3D12_SHADER_VISIBILITY_ALL);

    // [2] = Surface Cache Metadata UAV descriptor table
    computeRootParameters[2].InitAsDescriptorTable(1, &descriptorRanges[2], D3D12_SHADER_VISIBILITY_ALL);

    // [3] = Surface Cache Constants CBV (b0)
    computeRootParameters[3].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    // [4] = Light Constants CBV (b1)
    computeRootParameters[4].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

    // [5] = Previous Surface Cache SRV (t5)
    computeRootParameters[5].InitAsDescriptorTable(1, &descriptorRanges[5], D3D12_SHADER_VISIBILITY_ALL);

    // [6] = Card Metadata SRV (t6)
    computeRootParameters[6].InitAsDescriptorTable(1, &descriptorRanges[6], D3D12_SHADER_VISIBILITY_ALL);

    // [7] = SDF Volume SRV descriptor table (t10)
    computeRootParameters[7].InitAsDescriptorTable(1, &descriptorRanges[7], D3D12_SHADER_VISIBILITY_ALL);

    // [8] = Additional UAVs descriptor table (u5-u6)
    computeRootParameters[8].InitAsDescriptorTable(1, &descriptorRanges[4], D3D12_SHADER_VISIBILITY_ALL);

    // [9] = Radiance Cache Probe UAV (u4)
    computeRootParameters[9].InitAsDescriptorTable(1, &descriptorRanges[3], D3D12_SHADER_VISIBILITY_ALL);

    // [10] = Surface Cache Atlas SRV for Radiance Cache (t200-t201)
    computeRootParameters[10].InitAsDescriptorTable(1, &descriptorRanges[8], D3D12_SHADER_VISIBILITY_ALL);

    // [11] = Radiance Cache Probe SRV (Previous, t208)
    computeRootParameters[11].InitAsDescriptorTable(1, &descriptorRanges[9], D3D12_SHADER_VISIBILITY_ALL);

    // [12] = Card BVH Nodes SRV + Indices SRV (t240-t241)
    computeRootParameters[12].InitAsDescriptorTable(1, &descriptorRanges[10], D3D12_SHADER_VISIBILITY_ALL);

    // [13] = Radiance Cache Constants CBV (b13)
    computeRootParameters[13].InitAsConstantBufferView(13, 0, D3D12_SHADER_VISIBILITY_ALL);

    // [14] = Surface Cache Metadata SRV for Radiance Cache (t202-t203)
    // 用于 Radiance Cache Update 读取 Card Metadata
    static CD3DX12_DESCRIPTOR_RANGE surfCacheMetaSrvRange;
    surfCacheMetaSrvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SURFACE_CACHE_TYPE_COUNT, 202, 0); // t202-t203
    computeRootParameters[14].InitAsDescriptorTable(1, &surfCacheMetaSrvRange, D3D12_SHADER_VISIBILITY_ALL);

    D3D12_STATIC_SAMPLER_DESC samplers[2] = {};

    samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[0].MipLODBias = 0;
    samplers[0].MaxAnisotropy = 0;
    samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    samplers[0].MinLOD = 0.0f;
    samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[0].ShaderRegister = 0;
    samplers[0].RegisterSpace = 0;
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Linear sampler (s1) - Surface Cache 采样
    samplers[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplers[1].MipLODBias = 0;
    samplers[1].MaxAnisotropy = 0;
    samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    samplers[1].MinLOD = 0.0f;
    samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
    samplers[1].ShaderRegister = 1;
    samplers[1].RegisterSpace = 0;
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    CD3DX12_ROOT_SIGNATURE_DESC computeRootSigDesc = {};
    computeRootSigDesc.Init(
        _countof(computeRootParameters),
        computeRootParameters,
        _countof(samplers),
        samplers,
        D3D12_ROOT_SIGNATURE_FLAG_NONE
    );
    ID3DBlob* signature = nullptr;
    ID3DBlob* error = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(
        &computeRootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error
    );

    if (FAILED(hr))
    {
        if (error != nullptr)
        {
            DebuggerPrintf("Root signature serialization error: %s\n", (char*)error->GetBufferPointer());
            error->Release();
        }
        GUARANTEE_OR_DIE(false, "Failed to serialize compute root signature!");
    }

    hr = m_device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_computeRootSignature)
    );

    GUARANTEE_OR_DIE(SUCCEEDED(hr), "Failed to create compute root signature!");
    m_computeRootSignature->SetName(L"ComputeRootSignature");

    if (signature) signature->Release();
    if (error) error->Release();

    DebuggerPrintf("[Renderer] Created Compute Root Signature with Radiance Cache support\n");
}

void DX12Renderer::CreateSurfaceCacheDescriptorsAndTransitionStates(SurfaceCache* cache, int bufferIndex)
{
	if (cache == nullptr)
		return;

	const D3D12_CPU_DESCRIPTOR_HANDLE cpuStart = m_cbvSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
	const UINT inc = m_scuDescriptorSize;

	int idxSrvAtlas = 0;
	int idxSrvMeta = 0;
	int idxUavRadiance = 0; 
	int idxUavMeta = 0;

	if (cache->m_type == SURFACE_CACHE_TYPE_PRIMARY)
	{
		idxSrvAtlas = PrimaryAtlasSrvIndex(bufferIndex);
		idxSrvMeta = PrimaryMetaSrvIndex(bufferIndex);
		idxUavRadiance = PrimaryAtlasUavIndex(bufferIndex);  
		idxUavMeta = PrimaryMetaUavIndex(bufferIndex);
	}
	else // SURFACE_CACHE_TYPE_GI
	{
		idxSrvAtlas = GIAtlasSrvIndex(bufferIndex);
		idxSrvMeta = GIMetaSrvIndex(bufferIndex);
		idxUavRadiance = GIAtlasUavIndex(bufferIndex); 
		idxUavMeta = GIMetaUavIndex(bufferIndex);
	}

	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = SURFACE_CACHE_LAYER_COUNT;  // SRV可以访问所有layers

		m_device->CreateShaderResourceView(
			cache->m_atlasTexture[bufferIndex],
			&srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, idxSrvAtlas, inc)
		);
	}

	// Radiance Layer UAV（按理来说应该只写入这一个layer -> LIGHT）
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = 0;
		//uavDesc.Texture2DArray.FirstArraySlice = SURFACE_CACHE_LAYER_RADIANCE;
		uavDesc.Texture2DArray.FirstArraySlice = 0;
		//uavDesc.Texture2DArray.ArraySize = 1; 
		uavDesc.Texture2DArray.ArraySize = SURFACE_CACHE_LAYER_COUNT;

		m_device->CreateUnorderedAccessView(
			cache->m_atlasTexture[bufferIndex],
			nullptr,
			&uavDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, idxUavRadiance, inc)
		);
	}

	// Metadata SRV
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.NumElements = cache->m_maxCards;
		srvDesc.Buffer.StructureByteStride = sizeof(SurfaceCardMetadata);

		m_device->CreateShaderResourceView(
			cache->m_cardMetadataBuffer[bufferIndex],
			&srvDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, idxSrvMeta, inc)
		);
	}

	// Metadata UAV
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = cache->m_maxCards;
		uavDesc.Buffer.StructureByteStride = sizeof(SurfaceCardMetadata);

		m_device->CreateUnorderedAccessView(
			cache->m_cardMetadataBuffer[bufferIndex],
			nullptr,
			&uavDesc,
			CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuStart, idxUavMeta, inc)
		);
	}

	CD3DX12_RESOURCE_BARRIER barriers[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(
			cache->m_atlasTexture[bufferIndex],
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		),
		CD3DX12_RESOURCE_BARRIER::Transition(
			cache->m_cardMetadataBuffer[bufferIndex],
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		)
	};
	m_commandList->ResourceBarrier(2, barriers);
}

void DX12Renderer::CompositeFinalImage() //TODO: 想做RenderNode型渲染器就要改名，现在这个实际上只是GBuffer+SurfaceCache管线的
{
	if (!m_giSystem || !m_gBufferPassActive)
		return;
	
	BeginForwardPass();
	if (!m_hasBackBufferCleared)
	{
		ClearForwardPassRTV(Rgba8::AQUA);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
		m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_frameIndex,
		m_rtvDescriptorSize
	);
	//CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());
	//m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	m_commandList->SetGraphicsRootSignature(m_graphicsRootSignature);

	ID3D12DescriptorHeap* heaps[] = { m_cbvSrvDescHeap };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	
	//绑定GBuffer SRV表 -> 看是否需要访问原GBuffer数据进行更复杂的合成等
	// Transition GBuffer resources to SRV state 
	CD3DX12_RESOURCE_BARRIER barriers[GBUFFER_COUNT + DEPTH_SRV_COUNT];
	for (int i = 0; i < GBUFFER_COUNT; ++i)
	{
		barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_gBuffer.GetResource(i),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
	} 
	// 深度缓冲：保持深度读取，添加像素着色器资源
	barriers[GBUFFER_COUNT] = CD3DX12_RESOURCE_BARRIER::Transition(
		m_depthStencilBuffer,
		D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_DEPTH_READ |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(GBUFFER_COUNT+DEPTH_SRV_COUNT, barriers);
	// 绑定GBuffer（已经在SRV状态）和Surface Cache用于读取
	CD3DX12_GPU_DESCRIPTOR_HANDLE gbufferHandle(
		m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
		NUM_CONSTANT_BUFFERS + MAX_TEXTURE_COUNT,
		m_scuDescriptorSize
	);
	m_commandList->SetGraphicsRootDescriptorTable(15, gbufferHandle);
    
	SurfaceCache* cache = GetCurrentCache(SURFACE_CACHE_TYPE_PRIMARY);
	BindSurfaceCacheForGraphics(cache);

	BindRadianceCacheForGraphics(&m_radianceCache);

	// //Rebind surfacecache const
	// D3D12_GPU_VIRTUAL_ADDRESS cbAddress =
	// 	m_constantBuffers[k_surfaceCacheConstantsSlot]->m_dx12ConstantBuffer->GetGPUVirtualAddress()
	// 	+ m_constantBuffers[k_surfaceCacheConstantsSlot]->m_offset;
	// m_commandList->SetGraphicsRootConstantBufferView(k_surfaceCacheConstantsSlot, cbAddress);
	//rebind camera
	D3D12_GPU_VIRTUAL_ADDRESS cameraCBAddress =
		m_constantBuffers[k_cameraConstantsSlot]->m_dx12ConstantBuffer->GetGPUVirtualAddress()
		+ m_constantBuffers[k_cameraConstantsSlot]->m_offset;
	m_commandList->SetGraphicsRootConstantBufferView(k_cameraConstantsSlot, cameraCBAddress);
	//Rebind radiancecache const
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress =
		m_constantBuffers[k_radianceCacheConstantsSlot]->m_dx12ConstantBuffer->GetGPUVirtualAddress()
		+ m_constantBuffers[k_radianceCacheConstantsSlot]->m_offset;
	m_commandList->SetGraphicsRootConstantBufferView(k_radianceCacheConstantsSlot, cbAddress);

	m_commandList->SetPipelineState(m_compositePSO);

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_commandList->IASetVertexBuffers(0, 0, nullptr);
	m_commandList->IASetIndexBuffer(nullptr);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_commandList->DrawInstanced(3, 1, 0, 0);
}

void DX12Renderer::BeginCardCapturePass()
{
	if (m_currentActivePass == ActivePass::CARDCAPTURE)
    {
        DebuggerPrintf("[CardCapture] Warning: Already in Card Capture Pass\n");
        return;
    }
	const std::vector<uint32_t>& dirtyCards = m_giSystem->GetDirtyCards();  
	if (dirtyCards.empty())
		return;

	m_currentCaptureIndex = 0;
	//ID3D12Resource* atlasTexture = m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].GetCurrentAtlasTexture();

	// // Transition atlas to RTV state
	// CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
	// 	atlasTexture,
	// 	D3D12_RESOURCE_STATE_COMMON,
	// 	D3D12_RESOURCE_STATE_RENDER_TARGET
	// );
	// m_commandList->ResourceBarrier(1, &barrier);
	// 7. 转换状态：Atlas COMMON -> COPY_DEST
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].GetCurrentAtlasTexture(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	m_commandList->ResourceBarrier(1, &barrier);
	
    m_currentActivePass = ActivePass::CARDCAPTURE;

	UploadCardMetadataToGPU();
	CaptureDirtySurfaceCards(s_maxCardsPerBatch);
}

void DX12Renderer::EndCardCapturePass()
{
	if (m_currentActivePass != ActivePass::CARDCAPTURE)
	{
		return;
	}

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].GetCurrentAtlasTexture(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(1, &barrier);
 
	DebuggerPrintf("[Renderer] End Card Capture Pass\n");
	m_currentActivePass = ActivePass::UNKNOWN;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::GetCardCaptureRTVHandle(int layer)
{
	const int baseIndex = GBUFFER_COUNT + FRAME_BUFFER_COUNT;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
		m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		baseIndex + layer,
		m_rtvDescriptorSize);

	return handle;
}

void DX12Renderer::UploadCardMetadataToGPU()
{
	if (m_giSystem->m_cardMetadataCPU.empty())
		return;
	
	for (int type = 0; type < SURFACE_CACHE_TYPE_COUNT; type++)
	{
		SurfaceCache cache = m_surfaceCaches[type];
		if (!cache.m_initialized)
			continue;
        
		ID3D12Resource* uploadBuffer = cache.GetCurrentMetadataUploadBuffer();
		if (!uploadBuffer)
			continue;
        
		void* mappedData = nullptr;
		HRESULT hr = uploadBuffer->Map(0, nullptr, &mappedData);
        
		if (SUCCEEDED(hr))
		{
			size_t dataSize = m_giSystem->m_cardMetadataCPU.size() * 
							 sizeof(SurfaceCardMetadata);
			memcpy(mappedData, m_giSystem->m_cardMetadataCPU.data(), dataSize);
			uploadBuffer->Unmap(0, nullptr);
            
			ID3D12Resource* defaultBuffer = cache.GetCurrentMetadataBuffer();
			if (defaultBuffer)
			{
                TransitionResource(uploadBuffer, 
					D3D12_RESOURCE_STATE_GENERIC_READ, 
					D3D12_RESOURCE_STATE_COPY_SOURCE);
                TransitionResource(defaultBuffer, 
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, 
					D3D12_RESOURCE_STATE_COPY_DEST);
                
				m_commandList->CopyResource(defaultBuffer, uploadBuffer);
                
				TransitionResource(defaultBuffer, 
					D3D12_RESOURCE_STATE_COPY_DEST, 
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
		}
	}
}

void DX12Renderer::CaptureDirtySurfaceCards(uint32_t maxCardsPerFrame)
{
	std::vector<uint32_t> cardsToUpdate = m_giSystem->BuildUpdateList(maxCardsPerFrame);

	for (uint32_t cardID : cardsToUpdate)
	{
		SurfaceCard* card = m_giSystem->m_scene->GetSurfaceCardByID(cardID);
		if (!card)
			continue;
                
		MeshObject* obj = static_cast<MeshObject*>(m_giSystem->m_scene->GetSceneObject(card->m_meshObjectID));
		if (!obj)
			continue;
                
		CardInstanceData* instance = obj->GetCardInstance(card->m_templateIndex);
		if (!instance)
			continue;
                
		const SurfaceCardTemplate& templ = obj->GetMesh()->m_cardTemplates[card->m_templateIndex];
                
		CaptureSingleCard(obj, card, instance, templ);
                
		card->m_pendingUpdate = false;
		card->m_lastTouchedFrame = m_frameIndex;
		instance->m_isDirty = false;

		if (card->m_pendingRealloc)
		{
			FinalizeCardCapture(card);
		}
	}
	m_giSystem->RemoveProcessedDirtyCards(cardsToUpdate.size());
}

void DX12Renderer::CaptureSingleCard(MeshObject* object, SurfaceCard* card, CardInstanceData* instance, const SurfaceCardTemplate& templ)
{
	uint32_t width = card->m_pixelResolution.x;   
	uint32_t height = card->m_pixelResolution.y;  
    
	D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 0.0f;       
	viewport.TopLeftY = 0.0f;     
	viewport.Width = (float)width;  
	viewport.Height = (float)height; 
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
    
	m_commandList->RSSetViewports(1, &viewport);
    
	D3D12_RECT scissorRect = {};
	scissorRect.left = 0;            
	scissorRect.top = 0;            
	scissorRect.right = (LONG)width; 
	scissorRect.bottom = (LONG)height;
    
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[SURFACE_CACHE_LAYER_COUNT];
	// for (int layer = 0; layer < SURFACE_CACHE_LAYER_COUNT; ++layer)
	// {
	// 	rtvHandles[layer] = GetCardCaptureRTVHandle(layer);
	// }
	//    
	// m_commandList->OMSetRenderTargets(
	// 	SURFACE_CACHE_LAYER_COUNT,
	// 	rtvHandles,
	// 	FALSE,
	// 	nullptr  // 可选：如果需要 depth，传入 DSV handle
	// );
	m_commandList->OMSetRenderTargets(SURFACE_CACHE_LAYER_COUNT,
		m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].m_tempCapture.m_tempRtvs, FALSE, nullptr);
	float clearColor[4] = { 0, 0, 0, 0 };
	for (int i = 0; i < SURFACE_CACHE_LAYER_COUNT; i++)
	{
		m_commandList->ClearRenderTargetView(m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].m_tempCapture.m_tempRtvs[i],
		                                     clearColor, 0, nullptr);
	}
	//SetupCardCaptureCamera(card, instance, templ);
	CardCaptureConstants captureConsts = {};
    
	captureConsts.CardOrigin = instance->m_worldOrigin;
	captureConsts.CardAxisX = instance->m_worldAxisX;
	captureConsts.CardAxisY = instance->m_worldAxisY;
	captureConsts.CardNormal = instance->m_worldNormal;
    
	captureConsts.CardSize = instance->m_worldSize;
	captureConsts.CaptureDirection = templ.m_direction;
	captureConsts.Resolution = card->m_pixelResolution.x;
	captureConsts.CaptureDepth = object->CalculateCaptureDepth(instance, templ.m_direction);
	memcpy(captureConsts.LightMask, instance->m_lightMask, sizeof(captureConsts.LightMask));
    
	m_constantBuffers[k_cardCaptureConstantsSlot]->AppendData(&captureConsts, sizeof(CardCaptureConstants), m_currentCaptureIndex);
	BindConstantBuffer(k_cardCaptureConstantsSlot, m_constantBuffers[k_cardCaptureConstantsSlot]);

	uint64_t psoKey = ((uint64_t)width << 32) | height;
	auto it = m_cardCapturePSOConfiguration.find(psoKey);
	ID3D12PipelineState* pso = (it != m_cardCapturePSOConfiguration.end()) ? it->second : m_cardCapturePSO;
	m_commandList->SetPipelineState(pso);
	
	DrawObjectsForCardCapture(card);

	for (int i = 0; i < SURFACE_CACHE_LAYER_COUNT; i++)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].m_tempCapture.m_tempTextures[i],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_COPY_SOURCE
		);
		m_commandList->ResourceBarrier(1, &barrier);
	}
    
	// 8. 复制到 Atlas 的不同层
	for (int layer = 0; layer < SURFACE_CACHE_LAYER_COUNT; layer++)
	{
		CopyCardToAtlasLayer(
			m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].m_tempCapture.m_tempTextures[layer],
			m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].GetCurrentAtlasTexture(),
			layer,
			card->m_atlasPixelCoord.x,
			card->m_atlasPixelCoord.y,
			card->m_pixelResolution.x,  // ⚠️ 实际分辨率，不是最大分辨率
			card->m_pixelResolution.y
		);
	}
    
	// 9. 转换状态：恢复temp textures-> RTV
	for (int i = 0; i < SURFACE_CACHE_LAYER_COUNT; i++)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].m_tempCapture.m_tempTextures[i],
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		m_commandList->ResourceBarrier(1, &barrier);
	}
	
	m_currentCaptureIndex ++;

	DebuggerPrintf("[Renderer] Captured card %u at atlas (%u,%u) size %ux%u\n",
				   card->m_globalCardID, card->m_atlasPixelCoord.x, card->m_atlasPixelCoord.y, width, height);
}

void DX12Renderer::DrawObjectsForCardCapture(SurfaceCard* card)
{
	MeshObject* obj = static_cast<MeshObject*>(m_giSystem->m_scene->GetSceneObject(card->m_meshObjectID));
	if (!obj || !obj->GetMesh())
		return;

	StaticMesh* mesh = obj->GetMesh();

	ModelConstants modelConstants;
	modelConstants.ModelToWorldTransform = obj->m_cachedWorldMatrix;
	obj->m_color.GetAsFloats(modelConstants.ModelColor);
	m_constantBuffers[k_cardCaptureModelConstantsSlot]->AppendData(&modelConstants, sizeof(ModelConstants), m_currentCaptureIndex); 
	BindConstantBuffer(k_cardCaptureModelConstantsSlot, m_constantBuffers[k_cardCaptureModelConstantsSlot]);

	ConstantBuffer* cb = m_constantBuffers[k_cardCaptureMaterialConstantsSlot];
	MaterialConstants materialConstant = {};
	if (mesh->m_diffuseTexture)
		materialConstant.DiffuseId = mesh->m_diffuseTexture->m_textureDescIndex;
	else
		materialConstant.DiffuseId = 0;
	if (mesh->m_normalTexture)
		materialConstant.NormalId = mesh->m_normalTexture->m_textureDescIndex;
	else
		materialConstant.NormalId = 1;
	if (mesh->m_specularTexture)
		materialConstant.SpecularId = mesh->m_specularTexture->m_textureDescIndex;
	else
		materialConstant.SpecularId = 2;
	cb->AppendData(&materialConstant, sizeof(MaterialConstants), m_currentCaptureIndex); 
	BindConstantBuffer(k_cardCaptureMaterialConstantsSlot, cb);
	
	BindVertexBuffer(mesh->m_vertexBuffer);
	BindIndexBuffer(mesh->m_indexBuffer);

	// DrawIndexed
	m_commandList->DrawIndexedInstanced((UINT)mesh->m_indices.size(), 1, 0, 0, 0);
}

void DX12Renderer::FinalizeCardCapture(SurfaceCard* card)
{
	if (card->m_oldAtlasCoord.x >= 0 && card->m_oldAtlasCoord.y >= 0)
	{
		m_giSystem->FreeCardSpace(card->m_oldAtlasCoord, card->m_oldTileSpan);
    
		DebuggerPrintf("[Renderer] Released old tile for card %u at (%d,%d)\n",
					   card->m_globalCardID, card->m_oldAtlasCoord.x, card->m_oldAtlasCoord.y);
    
		card->m_oldAtlasCoord = IntVec2(-1, -1);
		card->m_oldTileSpan = IntVec2(0, 0);
	}

	card->m_resident = true;
	card->m_pendingRealloc = false;
}

void DX12Renderer::CopyCardToAtlasLayer(ID3D12Resource* srcTexture, ID3D12Resource* dstAtlasArray, uint32_t dstLayer,
	uint32_t dstX, uint32_t dstY, uint32_t width, uint32_t height)
{
	D3D12_BOX srcBox = {};
	srcBox.left = 0;
	srcBox.top = 0;
	srcBox.front = 0;
	srcBox.right = width;
	srcBox.bottom = height;
	srcBox.back = 1;
    
	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.pResource = dstAtlasArray;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = dstLayer;  // Layer index
    
	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.pResource = srcTexture;
	src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src.SubresourceIndex = 0;
	
	m_commandList->CopyTextureRegion(
		&dst,
		dstX, dstY, 0,  // 目标位置
		&src,
		&srcBox         // 源区域
	);
}

void DX12Renderer::CreateCardCapturePSO(const IntVec2& resolution)
{
	std::string shaderSource;
    int result = FileReadToString(shaderSource, "Data/Shaders/CardCapture.hlsl");
    if (result < 0)
    {
        DebuggerPrintf("[CardCapture] Failed to load CardCapture.hlsl\n");
        return;
    }

    ID3DBlob* vs = nullptr;
    ID3DBlob* ps = nullptr;

    CompileShaderToByteCode(&vs, "CardCaptureVS",
        shaderSource.c_str(), "CardCaptureVS", "vs_5_0");
    CompileShaderToByteCode(&ps, "CardCapturePS",
        shaderSource.c_str(), "CardCapturePS", "ps_5_1");

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",     0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // PSO description
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = m_graphicsRootSignature;
    psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = true;

    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.IndependentBlendEnable = false;

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 4;  
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; 
    psoDesc.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT; 
    psoDesc.RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT; 
    psoDesc.RTVFormats[3] = DXGI_FORMAT_R16G16B16A16_FLOAT;  // DirectLight

    psoDesc.SampleDesc.Count = 1;

    ID3D12PipelineState* pso = nullptr;
    HRESULT hr = m_device->CreateGraphicsPipelineState(
        &psoDesc, IID_PPV_ARGS(&pso)
    );

    if (FAILED(hr))
    {
        DebuggerPrintf("[CardCapture] Failed to create PSO for %dx%d\n",
            resolution.x, resolution.y);
        DX_SAFE_RELEASE(pso);
        vs->Release();
        ps->Release();
        return;
    }

    std::wstring name = L"CardCapturePSO_" + std::to_wstring(resolution.x) +
        L"x" + std::to_wstring(resolution.y);
    pso->SetName(name.c_str());

    vs->Release();
    ps->Release();

    uint64_t key = ((uint64_t)resolution.x << 32) | (uint64_t)resolution.y;
    
    auto it = m_cardCapturePSOConfiguration.find(key);
    if (it != m_cardCapturePSOConfiguration.end() && it->second != nullptr)
    {
        it->second->Release();
    }
    
    m_cardCapturePSOConfiguration[key] = pso;
    
    if (m_cardCapturePSO == nullptr)
    {
        m_cardCapturePSO = pso;
    }
    
    DebuggerPrintf("[CardCapture] Successfully created PSO for %dx%d\n",
        resolution.x, resolution.y);
}

void DX12Renderer::InitializeSurfaceCaches()
{
	m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].Initialize(
		m_device,
		m_giSystem->m_config.m_primaryAtlasSize,
		m_giSystem->m_config.m_primaryTileSize,
		SURFACE_CACHE_TYPE_PRIMARY
	);

	for (int i = 0; i < SURFACE_CACHE_BUFFER_COUNT; i++)
	{
		CreateSurfaceCacheDescriptorsAndTransitionStates(&m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY], i);
	}

	if (m_giSystem->m_config.m_enableMultipleTypes)
	{
		m_surfaceCaches[SURFACE_CACHE_TYPE_GI].Initialize(m_device,
			m_giSystem->m_config.m_primaryAtlasSize,
			m_giSystem->m_config.m_primaryTileSize,
			SURFACE_CACHE_TYPE_GI); 
		for (int i = 0; i < SURFACE_CACHE_BUFFER_COUNT; i++)
		{
			CreateSurfaceCacheDescriptorsAndTransitionStates(&m_surfaceCaches[SURFACE_CACHE_TYPE_GI], i);
		}
	}
}

SurfaceCache* DX12Renderer::GetCurrentCache(SurfaceCacheType type)
{
	return &m_surfaceCaches[type];
}

SurfaceCache* DX12Renderer::GetPreviousCache(SurfaceCacheType type)
{
	// 同一个对象，通过GetPreviousAtlasTexture()访问previous buffer
	return &m_surfaceCaches[type];
}

void DX12Renderer::CreateCardCaptureResources()
{
	InitializeSurfaceCaches();

	for (int layer = 0; layer < SURFACE_CACHE_LAYER_COUNT; ++layer)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart());
		//ID3D12Resource* atlasTexture = m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].GetCurrentAtlasTexture();
        
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		//rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2DArray.MipSlice = 0;
		//rtvDesc.Texture2DArray.FirstArraySlice = layer;
		//rtvDesc.Texture2DArray.ArraySize = 1;

		rtvHandle.Offset(GBUFFER_COUNT+FRAME_BUFFER_COUNT + layer, m_rtvDescriptorSize);
		m_device->CreateRenderTargetView(m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].m_tempCapture.m_tempTextures[layer],
		                                 &rtvDesc, rtvHandle);
		m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].m_tempCapture.m_tempRtvs[layer] = rtvHandle;
	}

	CreateCardCapturePipelineStates();
}

void DX12Renderer::CreateCardCapturePipelineStates()
{
	std::vector<IntVec2> resolutions = {
		IntVec2(32, 32),
		IntVec2(64, 64),
		IntVec2(128, 128),
		IntVec2(512, 512)
	};

	for (const IntVec2& res : resolutions)
	{
		CreateCardCapturePSO(res); 
	}
}

void DX12Renderer::CreateCompositePSO()
{
	const char* shaderName = "Data/Shaders/CompositeShader";
	Shader* test = GetShaderByName(shaderName);
	if (test)
	{
		return;
	}
	std::string shaderHLSLName = std::string(shaderName) + ".hlsl";

	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderHLSLName);
	if (result < 0)
	{
		ERROR_AND_DIE("Fail to create shader from this shaderName");
	}

	ShaderConfig sConfig;
	sConfig.m_name = std::string(shaderName);
	sConfig.m_pixelEntryPoint = "CompositePS";
	sConfig.m_vertexEntryPoint = "CompositeVS";
	//Shader* shader = new Shader(sConfig);

	ID3DBlob* vertexShader;
	ID3DBlob* pixelShader;

	CompileShaderToByteCode(&vertexShader, "VertexShader", shaderSource.c_str(), sConfig.m_vertexEntryPoint.c_str(), "vs_5_0");
	CompileShaderToByteCode(&pixelShader, "PixelShader", shaderSource.c_str(), sConfig.m_pixelEntryPoint.c_str(), "ps_5_1");

	D3D12_INPUT_LAYOUT_DESC emptyInputLayout = { nullptr, 0 };
    
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    
    // Root Signature（必须包含所有需要的资源）
    psoDesc.pRootSignature = m_graphicsRootSignature;
    
    psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
    psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
    
    psoDesc.InputLayout = emptyInputLayout;
    
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    
    // Render Target 格式（必须与 BackBuffer 一致）
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = UINT_MAX;
    
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;  // 不剔除
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
    
    // 深度/模板状态（禁用深度测试）
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;  // 不使用深度缓冲
    
    HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_compositePSO));
    
    if (FAILED(hr))
    {
        DX_SAFE_RELEASE(vertexShader);
        DX_SAFE_RELEASE(pixelShader);
        ERROR_AND_DIE("Failed to create Composite PSO!");
    }
    
    m_compositePSO->SetName(L"CompositePSO");
    
    DebuggerPrintf("[CreateCompositePSO] Successfully created Composite PSO\n");
    
    // 释放 Shader 字节码
    DX_SAFE_RELEASE(vertexShader);
    DX_SAFE_RELEASE(pixelShader);
}

void DX12Renderer::BindSurfaceCacheForCompute(SurfaceCache* cache) //暂时用不到咧 <-还是要复用起来
{
	if (!cache) return;
    
	// 转换Surface Cache资源到UAV状态
	CD3DX12_RESOURCE_BARRIER barriers[] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(
			cache->GetCurrentAtlasTexture(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		),
		CD3DX12_RESOURCE_BARRIER::Transition(
			cache->GetCurrentMetadataBuffer(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		)
	};
	m_commandList->ResourceBarrier(2, barriers);
    
	// 确保使用正确的描述符堆
	ID3D12DescriptorHeap* heaps[] = { m_cbvSrvDescHeap };
	m_commandList->SetDescriptorHeaps(1, heaps);

	// 绑定 Surface Cache UAVs（Atlas + Metadata）到参数 1
	// 注意：这里绑定从 Atlas UAV 开始的连续 descriptor table
	//CD3DX12_GPU_DESCRIPTOR_HANDLE surfaceCacheUAVHandle(
	//	m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
	//	SURFACE_CACHE_PRIMARY_ATLAS_UAV_INDEX,  // 从 Atlas UAV 开始，包含 Atlas + Metadata
	//	m_scuDescriptorSize
	//);
	//m_commandList->SetComputeRootDescriptorTable(1, surfaceCacheUAVHandle);
}

void DX12Renderer::BindSurfaceCacheForGraphics(SurfaceCache* cache)
{
	if (!cache) 
		return;
    
	// 转换Surface Cache到SRV状态（从UAV转换）
	CD3DX12_RESOURCE_BARRIER barriers[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(
			cache->GetCurrentAtlasTexture(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,  // 从UAV
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE  // 到SRV
		),
		CD3DX12_RESOURCE_BARRIER::Transition(
			cache->GetCurrentMetadataBuffer(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, //暂时是这样，之后有了radiance cache它俩会统一~
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		)
	};
	m_commandList->ResourceBarrier(2, barriers);

	int t = cache->m_type;

	int baseSlot = (t == 0) ? PrimaryAtlasSrvIndex(m_frameIndex)
		: GIAtlasSrvIndex(m_frameIndex);

	// 绑定 Surface Cache SRVs（Atlas + Metadata）到 16
	CD3DX12_GPU_DESCRIPTOR_HANDLE surfaceCacheSRVHandle(
		m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart(),
		baseSlot,  // 从 Atlas SRV 开始，包含 Atlas + Metadata
		m_scuDescriptorSize
	);
	m_commandList->SetGraphicsRootDescriptorTable(16, surfaceCacheSRVHandle);
}

void DX12Renderer::VisualizeSurfaceCache()
{
	 // Simple visualization - copy atlas to back buffer
    //if (!m_giSystem)
    //    return;
    //
    //SurfaceCache* cache = m_giSystem->GetCurrentCache(SURFACE_CACHE_TYPE_PRIMARY);
    //if (!cache || !cache->GetAtlasTexture())
    //    return;
    //
    //// Transition resources
    //CD3DX12_RESOURCE_BARRIER barriers[] = {
    //    CD3DX12_RESOURCE_BARRIER::Transition(
    //        m_renderTargets[m_frameIndex],
    //        D3D12_RESOURCE_STATE_RENDER_TARGET,
    //        D3D12_RESOURCE_STATE_COPY_DEST
    //    ),
    //    CD3DX12_RESOURCE_BARRIER::Transition(
    //        cache->GetAtlasTexture(),
    //        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //        D3D12_RESOURCE_STATE_COPY_SOURCE
    //    )
    //};
    //m_commandList->ResourceBarrier(2, barriers);
    //
    //// Copy first layer of atlas to back buffer (for visualization)
    //D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    //srcLocation.pResource = cache->GetAtlasTexture();
    //srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    //srcLocation.SubresourceIndex = 0; // First layer (albedo)
    //
    //D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    //dstLocation.pResource = m_renderTargets[m_frameIndex];
    //dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    //dstLocation.SubresourceIndex = 0;
    //
    //// Copy region (scale down if atlas is larger than screen)
    //uint32_t copyWidth = min(m_config.m_window->GetClientDimensions().x, 
    //                         (int)m_giSystem->m_config.m_primaryAtlasSize);
    //uint32_t copyHeight = min(m_config.m_window->GetClientDimensions().y,
    //                         (int)m_giSystem->m_config.m_primaryAtlasSize);
    //
    //D3D12_BOX srcBox = {0, 0, 0, copyWidth, copyHeight, 1};
    //m_commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, &srcBox);
    //
    //// Transition back
    //barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
    //    m_renderTargets[m_frameIndex],
    //    D3D12_RESOURCE_STATE_COPY_DEST,
    //    D3D12_RESOURCE_STATE_RENDER_TARGET
    //);
    //barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
    //    cache->GetAtlasTexture(),
    //    D3D12_RESOURCE_STATE_COPY_SOURCE,
    //    D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    //);
    //m_commandList->ResourceBarrier(2, barriers);
}

void DX12Renderer::InitializeRadianceCache()
{
	if (!m_device)
	{
		ERROR_AND_DIE("[DX12Renderer] Cannot initialize Radiance Cache: device is null!");
	}
    
	// 配置参数
	uint32_t maxProbes = 4096;      // 最多 4096 个 Probes
	uint32_t raysPerProbe = 32;     // 每个 Probe 发射 32 条射线
    
	m_radianceCache.Initialize(m_device, maxProbes, raysPerProbe);
    
	DebuggerPrintf("[DX12Renderer] Radiance Cache initialized: %u max probes, %u rays/probe\n",
				   maxProbes, raysPerProbe);
}

void DX12Renderer::CreateRadianceCacheResources()
{
	InitializeRadianceCache();
	
	for (int i = 0; i < RADIANCE_CACHE_BUFFER_COUNT; i++)
    {
        ID3D12Resource* probeBuffer = m_radianceCache.m_probeBuffer[i];
        if (!probeBuffer)
            continue;
        
        // === SRV ===
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_radianceCache.GetMaxProbes();
        srvDesc.Buffer.StructureByteStride = sizeof(RadianceProbeGPU);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        
        int srvIndex = RadianceCacheProbeSrvIndex(i);
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = GetCPUDescriptorHandle(
            m_cbvSrvDescHeap,
            srvIndex
        );
        m_device->CreateShaderResourceView(probeBuffer, &srvDesc, srvHandle);
        
        // === UAV ===
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = m_radianceCache.GetMaxProbes();
        uavDesc.Buffer.StructureByteStride = sizeof(RadianceProbeGPU);
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        
        int uavIndex = RadianceCacheProbeUavIndex(i);
        D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = GetCPUDescriptorHandle(
            m_cbvSrvDescHeap,
            uavIndex
        );
        m_device->CreateUnorderedAccessView(probeBuffer, nullptr, &uavDesc, uavHandle);
    }
    
    // ========== Update List SRV ==========
    {
        ID3D12Resource* updateListBuffer = m_radianceCache.GetUpdateListBuffer();
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_UINT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = 256;  // 最多 256 个更新索引
        srvDesc.Buffer.StructureByteStride = 0;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        
        int srvIndex = RadianceCacheUpdateListSrvIndex();
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = GetCPUDescriptorHandle(
            m_cbvSrvDescHeap,
            srvIndex
        );
        m_device->CreateShaderResourceView(updateListBuffer, &srvDesc, srvHandle);
    }
    
    DebuggerPrintf("[DX12Renderer] Created Radiance Cache descriptors\n");
}

void DX12Renderer::CreateRadianceCacheUpdatePSO()
{
	const char* shaderName = "Data/Shaders/RadianceCacheUpdate";
	Shader* test = GetShaderByName(shaderName);
	if (test)
	{
		return;
	}
	std::string shaderHLSLName = std::string(shaderName) + ".hlsl";

	std::string shaderSource;
	int result = FileReadToString(shaderSource, shaderHLSLName);
	if (result < 0)
	{
		ERROR_AND_DIE("Fail to create shader from this shaderName");
	}

	ID3DBlob* computeShaderBlob = nullptr;
	bool success = CompileShaderToByteCode(
		&computeShaderBlob,
		"RadianceCacheUpdate",
		shaderSource.c_str(),
		"CSMain",
		"cs_5_1"
	);
    
	if (!success || !computeShaderBlob)
	{
		ERROR_AND_DIE("[DX12Renderer] Failed to compile RadianceCacheUpdate.hlsl!");
	}
    
	// 创建 Compute PSO
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_computeRootSignature;
	psoDesc.CS = {
		computeShaderBlob->GetBufferPointer(),
		computeShaderBlob->GetBufferSize()
	};
	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    
	HRESULT hr = m_device->CreateComputePipelineState(
		&psoDesc,
		IID_PPV_ARGS(&m_radianceCacheUpdatePSO)
	);
    
	if (FAILED(hr))
	{
		ERROR_AND_DIE("[DX12Renderer] Failed to create Radiance Cache Update PSO!");
	}
    
	DX_SAFE_RELEASE(computeShaderBlob);
    
	DebuggerPrintf("[DX12Renderer] Created Radiance Cache Update PSO\n");
}

void DX12Renderer::BeginRadianceCachePass()
{
	RadianceCacheManager* manager = m_giSystem->GetRadianceCacheManager();
    if (!manager)
        return;
    
    manager->PlaceProbesScreenSpace(m_camera, m_gBuffer);
    
    manager->BuildPriorityQueue(m_camera, m_frameIndex);
    
    m_radianceCache.BuildUpdateQueue(s_maxProbesPerUpdate); //256
    
    uint32_t updateCount = m_radianceCache.GetUpdateProbeCount();
    if (updateCount == 0)
    {
        return;
    }
    
    // ========== 4. 上传数据到 GPU ==========
    m_radianceCache.UploadProbeData();
    
    RadianceCacheConstants constants = {};
    constants.MaxProbes = m_radianceCache.GetMaxProbes();
    constants.ActiveProbeCount = m_radianceCache.GetActiveProbeCount();
    constants.UpdateProbeCount = updateCount;
    constants.RaysPerProbe = m_radianceCache.GetRaysPerProbe();
    
    Vec3 camPos = m_currentCam.CameraWorldPosition;
    constants.CameraPositionX = camPos.x;
    constants.CameraPositionY = camPos.y;
    constants.CameraPositionZ = camPos.z;
    
    constants.ViewProj = m_camera.GetProjectionMatrix(); //有问题
    constants.ViewProjInverse = constants.ViewProj.GetOrthonormalInverse();
    
    constants.ScreenWidth = (float)m_config.m_window->GetClientDimensions().x;
    constants.ScreenHeight = (float)m_config.m_window->GetClientDimensions().y;
    constants.CurrentFrame = m_frameIndex;
    constants.TemporalBlend = 0.9f;  // 90% 保留旧数据
    
    constants.MaxTraceDistance = 50.0f;
    constants.ProbeSpacing = 2.0f;
    
    // Surface Cache 信息
    SurfaceCache* surfaceCache = GetCurrentCache(SURFACE_CACHE_TYPE_PRIMARY);
    constants.ActiveCardCount = (uint32_t)m_giSystem->GetCurrentSurfaceCardMetadataCPU().size();
    constants.AtlasWidth = surfaceCache->m_atlasSize;
    constants.AtlasHeight = surfaceCache->m_atlasSize;
    constants.TileSize = surfaceCache->m_tileSize;
    constants.BVHNodeCount = m_cardBVHNodeCount;

	m_constantBuffers[k_radianceCacheConstantsSlot]->AppendData(&constants, sizeof(RadianceCacheConstants), m_currentDrawIndex); 
	BindConstantBuffer(k_radianceCacheConstantsSlot,
		m_constantBuffers[k_radianceCacheConstantsSlot]);
	 
    // Surface Cache Atlas → SRV
    TransitionResource(
        surfaceCache->GetCurrentAtlasTexture(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );
    
    // Previous Probe Buffer → SRV
    TransitionResource(
        m_radianceCache.GetPreviousProbeBuffer(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );
    
    // Current Probe Buffer → UAV
    TransitionResource(
        m_radianceCache.GetCurrentProbeBuffer(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    
    // Card BVH → SRV
    if (m_cardBVHNodeBuffer)
    {
        TransitionResource(
            m_cardBVHNodeBuffer,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        );
    }
    
    if (m_cardBVHIndexBuffer)
    {
        TransitionResource(
            m_cardBVHIndexBuffer,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        );
    }
    
    // ========== 6. 绑定资源 ==========
    BindRadianceCacheForCompute(&m_radianceCache);
    
    // ========== 7. 设置 PSO ==========
    m_commandList->SetPipelineState(m_radianceCacheUpdatePSO);
    m_commandList->SetComputeRootSignature(m_computeRootSignature);
    
    // ========== 8. Dispatch ==========
    uint32_t groupCount = (updateCount + 63) / 64;  // 每个 thread group 64 个线程
    m_commandList->Dispatch(groupCount, 1, 1);
    
    
    DebuggerPrintf("[DX12Renderer] Updated %u radiance probes\n", updateCount);
}

void DX12Renderer::EndRadianceCachePass()
{
	// ========== 9. UAV Barrier ==========
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_radianceCache.GetCurrentProbeBuffer();
	m_commandList->ResourceBarrier(1, &uavBarrier);
    
	TransitionResource(
		m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY].GetCurrentAtlasTexture(),
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
    
	TransitionResource(
		m_radianceCache.GetCurrentProbeBuffer(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COMMON
	);
}

void DX12Renderer::BindRadianceCacheForCompute(RadianceCache* cache)
{
	if (!cache || !cache->m_initialized)
    {
        DebuggerPrintf("[Renderer] Warning: Cannot bind uninitialized Radiance Cache\n");
        return;
    }
    
    m_commandList->SetComputeRootSignature(m_computeRootSignature);
    
    DebuggerPrintf("[Renderer] Binding Radiance Cache for Compute...\n");
    
    D3D12_GPU_DESCRIPTOR_HANDLE gpuStart = m_cbvSrvDescHeap->GetGPUDescriptorHandleForHeapStart();
    UINT inc = m_scuDescriptorSize;
    
    // ========================================
    // Root Parameter [9]: Radiance Cache Probe UAV (Current, u4)
    // ========================================
    {
        int currentIndex = cache->GetCurrentBufferIndex();
        int uavIndex = RadianceCacheProbeUavIndex(currentIndex);  
        
        D3D12_GPU_DESCRIPTOR_HANDLE probeUavHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            gpuStart,
            uavIndex,
            inc
        );
        
        m_commandList->SetComputeRootDescriptorTable(9, probeUavHandle);
        
        DebuggerPrintf("  [9] Probe UAV (Current): descriptor index %d\n", uavIndex);
    }
    
    // ========================================
    // Root Parameter [11]: Radiance Cache Probe SRV (Previous, t208)
    // ========================================
    {
        int previousIndex = 1 - cache->GetCurrentBufferIndex();
        int srvIndex = RadianceCacheProbeSrvIndex(previousIndex);
        
        D3D12_GPU_DESCRIPTOR_HANDLE probeSrvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            gpuStart,
            srvIndex,
            inc
        );
        
        m_commandList->SetComputeRootDescriptorTable(11, probeSrvHandle);
        
        DebuggerPrintf("  [11] Probe SRV (Previous): descriptor index %d\n", srvIndex);
    }
    
    // ========================================
    // Root Parameter [10]: Surface Cache Atlas SRV (t200-t201)
    // ========================================
    {
        // 绑定 PRIMARY Surface Cache Atlas（用于采样直接光照）
        SurfaceCache* primaryCache = &m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY];
        int currentIndex = primaryCache->GetCurrentBufferIndex();
        int atlasSrvIndex = PrimaryAtlasSrvIndex(currentIndex);  // 例如 219
        
        D3D12_GPU_DESCRIPTOR_HANDLE atlasSrvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            gpuStart,
            atlasSrvIndex,
            inc
        );
        
        m_commandList->SetComputeRootDescriptorTable(10, atlasSrvHandle);
        
        DebuggerPrintf("  [10] Surface Cache Atlas SRV: descriptor index %d\n", atlasSrvIndex);
    }
    
    // ========================================
    // Root Parameter [6]: Card Metadata SRV (t6)
    // ========================================
    {
        // 使用 PRIMARY Surface Cache 的 Metadata
        SurfaceCache* primaryCache = &m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY];
        int currentIndex = primaryCache->GetCurrentBufferIndex();
        int metaSrvIndex = PrimaryMetaSrvIndex(currentIndex); 
        D3D12_GPU_DESCRIPTOR_HANDLE metaSrvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            gpuStart,
            metaSrvIndex,
            inc
        );
        
        m_commandList->SetComputeRootDescriptorTable(6, metaSrvHandle);
        
        DebuggerPrintf("  [6] Card Metadata SRV: descriptor index %d\n", metaSrvIndex);
    }
    
    // ========================================
    // Root Parameter [12]: Card BVH SRVs (t240-t241)
    // ========================================
    {
        // 注意：这里绑定的是 BVH Nodes 的起始位置
        // Descriptor Range 会自动覆盖 t240 (Nodes) 和 t241 (Indices)
        int bvhNodeSrvIndex = CARD_BVH_NODE_SRV;  // 240
        
        D3D12_GPU_DESCRIPTOR_HANDLE bvhHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
            gpuStart,
            bvhNodeSrvIndex,
            inc
        );
        
        m_commandList->SetComputeRootDescriptorTable(12, bvhHandle);
        
        DebuggerPrintf("  [12] Card BVH: descriptor index %d (Nodes) + %d (Indices)\n", 
            bvhNodeSrvIndex, bvhNodeSrvIndex + 1);
    }
    
    // ========================================
    // Root Parameter [13]: Radiance Cache Constants (b13)
    // ========================================
    {
        RadianceCacheConstants rcConstants = {};
        
        rcConstants.MaxProbes = cache->GetMaxProbes();
        rcConstants.ActiveProbeCount = cache->GetActiveProbeCount();
        rcConstants.UpdateProbeCount = cache->GetUpdateProbeCount();
        rcConstants.RaysPerProbe = cache->GetRaysPerProbe();
        
        rcConstants.CameraPositionX = m_camera.GetPosition().x;
        rcConstants.CameraPositionY = m_camera.GetPosition().y;
        rcConstants.CameraPositionZ = m_camera.GetPosition().z;
        
        Mat44 viewProj = m_camera.GetProjectionMatrix(); //TO CHECK
        viewProj.Append(m_camera.GetCameraToRenderTransform());
        rcConstants.ViewProj = viewProj;
        
        Mat44 viewProjInverse = viewProj;
        rcConstants.ViewProjInverse = viewProjInverse.GetOrthonormalInverse();
        
        IntVec2 screenSize = m_config.m_window->GetClientDimensions();
        rcConstants.ScreenWidth = (float)screenSize.x;
        rcConstants.ScreenHeight = (float)screenSize.y;
        
        rcConstants.CurrentFrame = m_frameIndex;
        rcConstants.TemporalBlend = 0.9f;  // Lumen 默认值
        
        rcConstants.MaxTraceDistance = 100.0f;  // 可配置
        rcConstants.ProbeSpacing = 2.0f;        // 可配置
        
        SurfaceCache* primaryCache = &m_surfaceCaches[SURFACE_CACHE_TYPE_PRIMARY];
        rcConstants.ActiveCardCount = (uint32_t)m_giSystem->GetCurrentSurfaceCardMetadataCPU().size();
        rcConstants.AtlasWidth = primaryCache->m_atlasSize;
        rcConstants.AtlasHeight = primaryCache->m_atlasSize;
        rcConstants.TileSize = primaryCache->m_tileSize;
        
        rcConstants.BVHNodeCount = m_cardBVHNodeCount;
        
        m_constantBuffers[k_radianceCacheConstantsSlot]->AppendData(
            &rcConstants,
            sizeof(RadianceCacheConstants),
            m_currentDrawIndex
        );
        D3D12_GPU_VIRTUAL_ADDRESS cbvAddress = 
            m_constantBuffers[k_radianceCacheConstantsSlot]->m_dx12ConstantBuffer->GetGPUVirtualAddress();
        m_commandList->SetComputeRootConstantBufferView(k_radianceCacheConstantsSlot, cbvAddress);
        
        DebuggerPrintf("  [13] Radiance Cache Constants: %u probes, %u active, %u updating\n",
            rcConstants.MaxProbes, rcConstants.ActiveProbeCount, rcConstants.UpdateProbeCount);
    }
    
    // ========================================
    // Root Parameter [1]: Update List Buffer (已在其他地方处理)
    // ========================================
    // Update List 在 cache->UploadProbeData() 中上传
    // 这里不需要额外绑定
    
    DebuggerPrintf("[Renderer] ✅ Radiance Cache bound for Compute\n");
}

void DX12Renderer::BindRadianceCacheForGraphics(RadianceCache* cache)
{
	if (!cache || !cache->m_initialized)
	{
		DebuggerPrintf("[Renderer] Warning: Cannot bind uninitialized Radiance Cache\n");
		return;
	}
	ID3D12Resource* currentProbeBuffer = cache->GetCurrentProbeBuffer();
    
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		currentProbeBuffer,
		D3D12_RESOURCE_STATE_COMMON,  // 或 D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);
	m_commandList->ResourceBarrier(1, &barrier);
    
	D3D12_GPU_VIRTUAL_ADDRESS probeBufferAddress = currentProbeBuffer->GetGPUVirtualAddress();
	m_commandList->SetGraphicsRootShaderResourceView(18, probeBufferAddress);
    
	// DebuggerPrintf("[Renderer] Bound Radiance Cache SRV at GPU address 0x%llx\n", 
	// 			   probeBufferAddress);
}

void DX12Renderer::UploadBufferData(ID3D12Resource* dstBuffer, const void* srcData, uint32_t dataSize) //TODO
{
	if (!dstBuffer || !srcData || dataSize == 0)
		return;
    
	// 创建 Upload Heap
	D3D12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
    
	ID3D12Resource* uploadBuffer = nullptr;
	HRESULT hr = m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)
	);
    
	if (FAILED(hr))
	{
		ERROR_AND_DIE("[DX12Renderer] Failed to create upload buffer!");
	}
    
	// Map and copy
	void* mappedData = nullptr;
	hr = uploadBuffer->Map(0, nullptr, &mappedData);
	if (SUCCEEDED(hr))
	{
		memcpy(mappedData, srcData, dataSize);
		uploadBuffer->Unmap(0, nullptr);
	}
    
	// Transition dst to COPY_DEST
	TransitionResource(dstBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    
	// Copy
	m_commandList->CopyResource(dstBuffer, uploadBuffer);
    
	// Transition back to SRV
	TransitionResource(dstBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    
	// 暂时保存，在帧结束后释放
	m_currentFrameTempResources.push_back(uploadBuffer);
}

void DX12Renderer::CreateCardBVHBuffers(const std::vector<GPUCardBVHNode>& nodes,
                                        const std::vector<uint32_t>& cardIndices)
{
	if (nodes.empty() || cardIndices.empty())
    {
        DebuggerPrintf("[DX12Renderer] Warning: Empty BVH data\n");
        return;
    }
    
    // ========== 1. 创建 BVH Node Buffer ==========
    {
        uint32_t bufferSize = static_cast<uint32_t>(nodes.size() * sizeof(GPUCardBVHNode));
        
        // 释放旧资源
        DX_SAFE_RELEASE(m_cardBVHNodeBuffer);
        
        // 创建 DEFAULT heap
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        
        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_cardBVHNodeBuffer)
        );
        
        if (FAILED(hr))
        {
            ERROR_AND_DIE("[DX12Renderer] Failed to create Card BVH node buffer!");
        }
        
        m_cardBVHNodeBuffer->SetName(L"CardBVH_NodeBuffer");
        m_cardBVHNodeCount = static_cast<uint32_t>(nodes.size());
        
        // 上传数据
        UploadBufferData(m_cardBVHNodeBuffer, nodes.data(), bufferSize);
    }
    
    // ========== 2. 创建 BVH Index Buffer ==========
    {
        uint32_t bufferSize = static_cast<uint32_t>(cardIndices.size() * sizeof(uint32_t));
        
        // 释放旧资源
        DX_SAFE_RELEASE(m_cardBVHIndexBuffer);
        
        // 创建 DEFAULT heap
        D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        
        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&m_cardBVHIndexBuffer)
        );
        
        if (FAILED(hr))
        {
            ERROR_AND_DIE("[DX12Renderer] Failed to create Card BVH index buffer!");
        }
        
        m_cardBVHIndexBuffer->SetName(L"CardBVH_IndexBuffer");
        m_cardBVHIndexCount = static_cast<uint32_t>(cardIndices.size());
        
        // 上传数据
        UploadBufferData(m_cardBVHIndexBuffer, cardIndices.data(), bufferSize);
    }
    
    // ========== 3. 创建 Descriptors ==========
    
    // Node Buffer SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_cardBVHNodeCount;
        srvDesc.Buffer.StructureByteStride = sizeof(GPUCardBVHNode);
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(
            m_cbvSrvDescHeap,
            CARD_BVH_NODE_SRV
        );
        m_device->CreateShaderResourceView(m_cardBVHNodeBuffer, &srvDesc, handle);
    }
    
    // Index Buffer SRV
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_UINT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = m_cardBVHIndexCount;
        srvDesc.Buffer.StructureByteStride = 0;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
        
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandle(
            m_cbvSrvDescHeap,
            CARD_BVH_INDEX_SRV
        );
        m_device->CreateShaderResourceView(m_cardBVHIndexBuffer, &srvDesc, handle);
    }
    
    DebuggerPrintf("[DX12Renderer] Created Card BVH buffers: %u nodes, %u indices\n",
                   m_cardBVHNodeCount, m_cardBVHIndexCount);
}

void DX12Renderer::TransitionResource(ID3D12Resource* resource,
	D3D12_RESOURCE_STATES beforeState,
	D3D12_RESOURCE_STATES afterState)
{
	if (beforeState == afterState)
	{
		DebuggerPrintf("Before and after states are the same, no transition needed.\n");
		return;
	}
    
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = beforeState;
	barrier.Transition.StateAfter = afterState;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    
	m_commandList->ResourceBarrier(1, &barrier);
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::GetCPUDescriptorHandle(ID3D12DescriptorHeap* heap, int index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += index * m_scuDescriptorSize;
	return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DX12Renderer::GetGPUDescriptorHandle(ID3D12DescriptorHeap* heap, int index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += index * m_scuDescriptorSize;
	return handle;
}

void DX12Renderer::SetSamplerMode(SamplerMode samplerMode)
{
	m_desiredSamplerMode = samplerMode;
}

void DX12Renderer::SetRasterizerMode(RasterizerMode rasterizerMode)
{
	m_desiredRasterizerMode = rasterizerMode;
}

void DX12Renderer::SetDepthMode(DepthMode depthMode)
{
	m_desiredDepthMode = depthMode;
}

void DX12Renderer::BeginRenderPass(RenderMode renderMode, Rgba8 const& backBufferClearColor)
{
	if (renderMode == m_currentRenderMode)
		return;

	switch (m_currentRenderMode)
	{
	case RenderMode::FORWARD:
		if (m_forwardPassActive)
		{
			EndForwardPass();
		}
		break;
	case RenderMode::GI:
		if (m_gBufferPassActive)
		{
			EndGBufferPass();
			UnbindGBufferPassRT();
		}
		break;
	}
	m_desiredRenderMode = renderMode;
	switch (m_desiredRenderMode)
	{
	case RenderMode::FORWARD:
		BeginForwardPass();
		if (!m_hasBackBufferCleared)
		{
			ClearForwardPassRTV(backBufferClearColor);
		}
		break;
	case RenderMode::GI:
		BeginGBufferPass();
		ClearGBufferPassRTV();
		break;
	}
        
	m_currentRenderMode = renderMode;
}

Texture* DX12Renderer::CreateTextureFromFile(char const* filePath)
{
	Image image = Image(filePath);
	Texture* newTexture = CreateTextureFromImage(image);
	newTexture->m_name = filePath;
	
	m_loadedTextures.push_back(newTexture);
	
	return newTexture;
}

void DX12Renderer::BindConstantBuffer(int slot, ConstantBuffer* cbo)
{
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = cbo->m_dx12ConstantBuffer->GetGPUVirtualAddress()
										+ cbo->m_offset;
	m_commandList->SetGraphicsRootConstantBufferView(slot, cbAddress);
}

IndexBuffer* DX12Renderer::CreateIndexBuffer(unsigned int size, unsigned int stride)
{
	IndexBuffer* indexBuffer = new IndexBuffer(size, stride);

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuffer->m_dx12IndexBuffer));
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 Cannot Create Index Buffer!");

	std::wstring name = L"IndexUpload_" + std::to_wstring(reinterpret_cast<uintptr_t>(indexBuffer));
	indexBuffer->m_dx12IndexBuffer->SetName(name.c_str());

	return indexBuffer;
}

void DX12Renderer::BindVertexBuffer(VertexBuffer* vbo)
{
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &vbo->m_vertexBufferView);
}

void DX12Renderer::WaitForPreviousFrame()              
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. More advanced samples 
	// illustrate how to use fences for efficient resource usage.

	// Signal and increment the fence value.

	// swap the current rtv buffer index so we draw on the correct buffer
	//m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	const UINT64 fence = m_fenceValue[m_frameIndex];
	HRESULT hr = m_commandQueue->Signal(m_fence[m_frameIndex], fence);
	GUARANTEE_OR_DIE(SUCCEEDED(hr), "d3d12 cannot signal!");
	//m_fenceValue[m_frameIndex]++;

   // if the current  fence value is still less than "fenceValue", then we know the GPU has not finished executing
   // the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
	if (m_fence[m_frameIndex]->GetCompletedValue() < fence)
	{
		//HRESULT hr;
		// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
		hr = m_fence[m_frameIndex]->SetEventOnCompletion(m_fenceValue[m_frameIndex], m_fenceEvent);
		GUARANTEE_OR_DIE(SUCCEEDED(hr), "D3D12 Cannot SetEventOnCompletion!");

		// We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
		// has reached "fenceValue", we know the command queue has finished executing
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// increment fenceValue for next frame
	m_fenceValue[m_frameIndex]++;
}

void DX12Renderer::ImGuiStartUp()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; 
	ImGui::StyleColorsDark();  // 或 Light/Classic

	D3D12_DESCRIPTOR_HEAP_DESC imGuiSRVDescHeap = {};
	imGuiSRVDescHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	imGuiSRVDescHeap.NumDescriptors = FRAME_BUFFER_COUNT;
	imGuiSRVDescHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	imGuiSRVDescHeap.NodeMask = 0;

	HRESULT hr = m_device->CreateDescriptorHeap(&imGuiSRVDescHeap, IID_PPV_ARGS(&m_imguiSrvDescHeap));

	GUARANTEE_OR_DIE(SUCCEEDED(hr), "Could not create D3D12 ImGui SRV desc heap!"); 

	m_imguiSrvDescHeap->SetName(L"ImGuiHeap");

	ImGui_ImplWin32_Init(g_theWindow->GetHwnd());
	ImGui_ImplDX12_Init(m_device, FRAME_BUFFER_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM, m_imguiSrvDescHeap,
	                    m_imguiSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
	                    m_imguiSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	// 手动调用一次NewFrame来初始化渲染器状态
	ImGui_ImplDX12_NewFrame();
}

void DX12Renderer::ImGuiBeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
}

void DX12Renderer::ImGuiEndFrame()
{
	ID3D12DescriptorHeap* heaps[] = { m_imguiSrvDescHeap };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);
}

void DX12Renderer::ImGuiShutDown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

size_t DX12Renderer::GetConstantBufferSize(int cbSlot)
{
	if (cbSlot == k_modelConstantsSlot)
	{
		return sizeof(ModelConstants);
	}
	if (cbSlot == k_cameraConstantsSlot)
	{
		return sizeof(CameraConstants);
	}
	if (cbSlot == k_perFrameConstantsSlot)
	{
		return sizeof(PerFrameConstants);
	}
	if (cbSlot == k_materialConstantsSlot)
	{
		return sizeof(MaterialConstants);
	}
	if (cbSlot == k_generalLightConstantsSlot)
	{
		return sizeof(GeneralLightConstants);
	}
	if (cbSlot == k_surfaceCacheConstantsSlot)
	{
		return sizeof(SurfaceCacheConstants);
	}
	if (cbSlot == k_passConstantsSlot)
	{
		return sizeof(PassModeConstants);
	}
	return 64;
}

#endif
