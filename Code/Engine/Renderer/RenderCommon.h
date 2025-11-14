#pragma once
#include "Engine/Math/AABB3.hpp"
#include "Engine/Renderer/Cache/SurfaceCacheCommon.h"

extern const char* m_shaderSource;

#define DX_SAFE_RELEASE(dxObject)\
{\
if ((dxObject) != nullptr)\
{\
(dxObject)->Release();\
(dxObject) = nullptr;\
}\
}\

#include "Engine/Window/Window.hpp"
#include "Engine/Math/IntVec4.h"

extern Window* g_theWindow;

 //#if defined(ENGINE_DEBUG_RENDER)
 //void* m_dxgiDebug = nullptr;
 //void* m_dxgiDebugModule = nullptr;
 //#endif

#if defined(OPAQUE)
#undef OPAQUE
#endif

enum class BlendMode
{
    ALPHA,
    ADDITIVE,
    OPAQUE,
    COUNT
};

enum class SamplerMode
{
    POINT_CLAMP,
    BILINEAR_WRAP,
    COUNT
};

enum class RasterizerMode
{
    SOLID_CULL_NONE,
    SOLID_CULL_BACK,
    WIREFRAME_CULL_NONE,
    WIREFRAME_CULL_BACK,
    COUNT
};

enum class DepthMode
{
    DISABLED,
    READ_ONLY_ALWAYS,
    READ_ONLY_LESS_EQUAL,
    READ_WRITE_LESS_EQUAL,
    COUNT
};

enum class VertexType
{
    VERTEX_PCU,
    VERTEX_PCUTBN,
    COUNT
};

#ifdef ENGINE_DX11_RENDERER
static const int k_perFrameConstantsSlot = 1;
static const int k_cameraConstantsSlot = 2;
static const int k_modelConstantsSlot = 3;
static const int k_generalLightConstantsSlot = 4;
static const int k_shadowConstantsSlot = 6;
#endif
#ifdef ENGINE_DX12_RENDERER
static const int k_perFrameConstantsSlot = 0;
static const int k_cameraConstantsSlot = 1;
static const int k_modelConstantsSlot = 2;
static const int k_materialConstantsSlot = 3;
static const int k_generalLightConstantsSlot = 4;
static const int k_shadowConstantsSlot = 6;
static const int k_passConstantsSlot = 5;
static const int k_surfaceCacheConstantsSlot = 12;
static const int k_radianceCacheConstantsSlot = 13;
static const int k_sdfGenerationConstantsSlot = 11;
static const int k_cardCaptureConstantsSlot = 10;
static const int k_cardCaptureModelConstantsSlot = 9;
static const int k_cardCaptureMaterialConstantsSlot = 8;
#endif

struct CameraConstants
{
    Mat44 WorldToCameraTransform;
    Mat44 CameraToRenderTransform;
    Mat44 RenderToClipTransform;
    
    Vec3 CameraWorldPosition;
    float padding;
};

struct ModelConstants
{
    Mat44 ModelToWorldTransform;
    float ModelColor[4];
};

#ifdef ENGINE_PAST_VERSION_LIGHTS
struct LightConstants //DirectionalLight
{
    float SunDirection[3];
    float SunIntensity;
    //float AmbientIntensity[4];
    float AmbientIntensity;
    float AmbientLightColor[3];
};
static const int k_lightConstantsSlot = 7;

struct PointLightConstants
{
    float PointLightPosition[3];
    float LightIntensity = 0.f;
    float LightColor[3];
    float LightRange;
};
static const int k_pointLightConstantsSlot = 4;

struct SpotLightConstants
{
    Vec3 SpotLightPosition;
    float SpotLightCutOff; // cos(45°)，表示光锥半角
    Vec3 SpotLightDirection; // Normalized
    float pad0;
    float SpotLightColor[3];
    float pad1;
};
static const int k_spotLightConstantsSlot = 5;
#endif

static const int s_maxLights = 15;
struct GeneralLight
{
    float Color[4]          =  {0.f, 0.f, 0.f, 0.f};
    float WorldPosition[3]  = {0.f, 0.f, 0.f};
    int LightType = 0; //不放进Constant里

    float SpotForward[3]    = {0.f, 0.f, 0.f};
    float Ambience          = 0.f;
    float InnerRadius       = 0.f;
    float OuterRadius       = 0.f;
    float InnerDotThreshold = -1.f;
    float OuterDotThreshold = -1.f;
};
struct GeneralLightConstants
{
    float SunColor[4];
    float SunNormal[3];
    int NumLights;
    GeneralLight LightsArray[s_maxLights];
};

struct ShadowConstants
{
    Mat44 LightViewProjectionMatrix;
};

struct PerFrameConstants 
{
    float Time;
    int DebugInt;
    float DebugFloat;
    float EMPTY_PADDING;
};

struct MaterialConstants
{
    int DiffuseId = 0;
    int NormalId = 1;
    int SpecularId = 2;

    float EMPTY_PADDING;
};

enum PrimitiveTopology
{
    PRIMITIVE_TRIANGLES,
    PRIMITIVE_LINES,
    PRIMITIVE_LINE_STRIP,
    PRIMITIVE_POINTS
};

enum class RenderMode
{
    FORWARD,  
    GI,
    UNKNOWN
};

struct PassModeConstants
{
    uint32_t RenderMode;
    uint32_t ObjectID;
    float padding[2];
};

static const int FRAME_BUFFER_COUNT = 3;  // Swap Chain 三重缓冲
static const int MAX_TEXTURE_COUNT = 200;
static const unsigned int NUM_CONSTANT_BUFFERS = 14;
static constexpr int GBUFFER_COUNT = 4;
static constexpr int DEPTH_SRV_COUNT = 1;
static constexpr int SURFACE_CACHE_DESCRIPTOR_COUNT = 4;
static constexpr int PREVIOUS_SURFACE_CACHE_RESOURCE_COUNT = 1;

// SurfaceCache 配置
static constexpr int SURFACE_CACHE_BUFFER_COUNT = 2;        // 双缓冲
static constexpr int DESCRIPTORS_PER_SURFACE_BUFFER = 4;    // Atlas SRV, Meta SRV, Atlas UAV, Meta UAV
// Radiance Cache 
static constexpr int RADIANCE_CACHE_BUFFER_COUNT = 2;       // 双缓冲
static constexpr int DESCRIPTORS_PER_RADIANCE_BUFFER = 2;   // Probe SRV, Probe UAV
// Card Capture
static constexpr int CARD_CAPTURE_RTV_COUNT = SURFACE_CACHE_LAYER_COUNT;
// SDF
static const int MAX_SDF_TEXTURE_COUNT = 128;

// ========================================
// Descriptor Heap Physical Layout
// ========================================
// 说明：这里的 slot 是指 Descriptor Heap 中的物理位置
// Shader register binding 通过 Root Signature 映射

// --- Section 1: Constant Buffers (Slot 0-13) ---
static constexpr int CBV_START = 0;                         // Physical slot 0

// --- Section 2: Scene Textures (Slot 14-213) ---
static constexpr int TEXTURE_SRV_START = NUM_CONSTANT_BUFFERS;  // Physical slot 14

// --- Section 3: GBuffer + Depth (Slot 214-218) ---
static constexpr int GBUFFER_SRV_START_SLOT = TEXTURE_SRV_START + MAX_TEXTURE_COUNT;  // 214
static constexpr int DEPTH_SRV_START_SLOT = GBUFFER_SRV_START_SLOT + GBUFFER_COUNT;   // 218

// --- Section 4: SurfaceCache PRIMARY (Slot 219-226) ---
static constexpr int SURFCACHE_PRIMARY_BASE = DEPTH_SRV_START_SLOT + DEPTH_SRV_COUNT; // 219

static constexpr int SURFCACHE_PRIMARY_BUF0_ATLAS_SRV = SURFCACHE_PRIMARY_BASE + 0;   // 219
static constexpr int SURFCACHE_PRIMARY_BUF0_META_SRV  = SURFCACHE_PRIMARY_BASE + 1;   // 220
static constexpr int SURFCACHE_PRIMARY_BUF0_ATLAS_UAV = SURFCACHE_PRIMARY_BASE + 2;   // 221
static constexpr int SURFCACHE_PRIMARY_BUF0_META_UAV  = SURFCACHE_PRIMARY_BASE + 3;   // 222

static constexpr int SURFCACHE_PRIMARY_BUF1_ATLAS_SRV = SURFCACHE_PRIMARY_BASE + 4;   // 223
static constexpr int SURFCACHE_PRIMARY_BUF1_META_SRV  = SURFCACHE_PRIMARY_BASE + 5;   // 224
static constexpr int SURFCACHE_PRIMARY_BUF1_ATLAS_UAV = SURFCACHE_PRIMARY_BASE + 6;   // 225
static constexpr int SURFCACHE_PRIMARY_BUF1_META_UAV  = SURFCACHE_PRIMARY_BASE + 7;   // 226

// --- Section 5: SurfaceCache GI (Slot 227-234) ---
static constexpr int SURFCACHE_GI_BASE = SURFCACHE_PRIMARY_BASE + 
    (DESCRIPTORS_PER_SURFACE_BUFFER * SURFACE_CACHE_BUFFER_COUNT);  // 227

static constexpr int SURFCACHE_GI_BUF0_ATLAS_SRV = SURFCACHE_GI_BASE + 0;  // 227
static constexpr int SURFCACHE_GI_BUF0_META_SRV  = SURFCACHE_GI_BASE + 1;  // 228
static constexpr int SURFCACHE_GI_BUF0_ATLAS_UAV = SURFCACHE_GI_BASE + 2;  // 229
static constexpr int SURFCACHE_GI_BUF0_META_UAV  = SURFCACHE_GI_BASE + 3;  // 230

static constexpr int SURFCACHE_GI_BUF1_ATLAS_SRV = SURFCACHE_GI_BASE + 4;  // 231
static constexpr int SURFCACHE_GI_BUF1_META_SRV  = SURFCACHE_GI_BASE + 5;  // 232
static constexpr int SURFCACHE_GI_BUF1_ATLAS_UAV = SURFCACHE_GI_BASE + 6;  // 233
static constexpr int SURFCACHE_GI_BUF1_META_UAV  = SURFCACHE_GI_BASE + 7;  // 234

// --- Section 6: Radiance Cache (Slot 235-239) ---
static constexpr int RADIANCE_CACHE_BASE = SURFCACHE_GI_BASE + 
    (DESCRIPTORS_PER_SURFACE_BUFFER * SURFACE_CACHE_BUFFER_COUNT);  // 235

static constexpr int RADIANCE_CACHE_BUF0_PROBE_SRV = RADIANCE_CACHE_BASE + 0;  // 235 (Previous)
static constexpr int RADIANCE_CACHE_BUF0_PROBE_UAV = RADIANCE_CACHE_BASE + 1;  // 236

static constexpr int RADIANCE_CACHE_BUF1_PROBE_SRV = RADIANCE_CACHE_BASE + 2;  // 237 (Current)
static constexpr int RADIANCE_CACHE_BUF1_PROBE_UAV = RADIANCE_CACHE_BASE + 3;  // 238

static constexpr int RADIANCE_CACHE_UPDATE_LIST_SRV = RADIANCE_CACHE_BASE + 4; // 239

// --- Section 7: Card BVH (Slot 240-241) ---
static constexpr int CARD_BVH_BASE = RADIANCE_CACHE_UPDATE_LIST_SRV + 1;  // 240
static constexpr int CARD_BVH_NODE_SRV = CARD_BVH_BASE + 0;   // 240
static constexpr int CARD_BVH_INDEX_SRV = CARD_BVH_BASE + 1;  // 241

// --- Section 8: Per-Frame Resources (Slot 242+) ---
static constexpr int PER_FRAME_BASE = CARD_BVH_INDEX_SRV + 1;  // 242

static constexpr int PROBE_GRID_SRV_BASE = PER_FRAME_BASE;                                  // 242
static constexpr int SSR_SRV_BASE = PROBE_GRID_SRV_BASE + FRAME_BUFFER_COUNT;              // 245
static constexpr int TEMPORAL_PREV_SRV_BASE = SSR_SRV_BASE + FRAME_BUFFER_COUNT;           // 248
static constexpr int TEMPORAL_MOTION_SRV_BASE = TEMPORAL_PREV_SRV_BASE + FRAME_BUFFER_COUNT; // 251

// --- Section 9: SDF Generation (Slot 254+) ---
static constexpr int SDF_GEN_BASE = TEMPORAL_MOTION_SRV_BASE + FRAME_BUFFER_COUNT;  // 254
static constexpr int SDF_GEN_VERTEX_SRV = SDF_GEN_BASE + 0;   // 254
static constexpr int SDF_GEN_INDEX_SRV = SDF_GEN_BASE + 1;    // 255
static constexpr int SDF_GEN_BVH_NODE_SRV = SDF_GEN_BASE + 2; // 256
static constexpr int SDF_GEN_BVH_TRI_SRV = SDF_GEN_BASE + 3;  // 257
static constexpr int SDF_GEN_OUTPUT_UAV = SDF_GEN_BASE + 4;   // 258

// --- Section 10: SDF Textures (Slot 259+) ---
static constexpr int SDF_TEXTURE_SRV_BASE = SDF_GEN_OUTPUT_UAV + 1;  // 259

static constexpr int TOTAL_NUM_DESCRIPTORS = SDF_TEXTURE_SRV_BASE + MAX_SDF_TEXTURE_COUNT; // 387

// ========================================
// Shader Register Bindings (for Root Signature)
// ========================================
// 说明：这些是 HLSL shader 中看到的 register numbers
// Root Signature 将 descriptor heap 的物理 slot 映射到这些 registers

// GBuffer 在 shader 中从 t200 开始（通过 Root Parameter 15 映射）
static constexpr int GBUFFER_SRV_START = MAX_TEXTURE_COUNT;  // shader registers: t200, t201, t202, 203 204

// SurfaceCache 在 shader 中从 t205 开始
static constexpr int SURFACE_CACHE_SRV = GBUFFER_SRV_START + GBUFFER_COUNT+ DEPTH_SRV_COUNT; 

// Radiance Cache
static constexpr int RADIANCE_CACHE_SRV = SURFACE_CACHE_SRV + DESCRIPTORS_PER_SURFACE_BUFFER;  // shader register

// 其他逐帧资源的 shader register 映射
static constexpr int RADIANCE_CACHE_SRV_INDEX = RADIANCE_CACHE_BUF1_PROBE_SRV;  // Current frame
static constexpr int PROBE_GRID_SRV_INDEX = PROBE_GRID_SRV_BASE;
static constexpr int SSR_SRV_INDEX = SSR_SRV_BASE;
static constexpr int TEMPORAL_PREV_SRV_INDEX = TEMPORAL_PREV_SRV_BASE;
static constexpr int TEMPORAL_MOTION_SRV_INDEX = TEMPORAL_MOTION_SRV_BASE;
static constexpr int SDF_VOLUME_SRV = SDF_TEXTURE_SRV_BASE;
static constexpr int BVH_STRUCTURE_SRV = CARD_BVH_NODE_SRV;

// ========================================
// Helper Functions: SurfaceCache Descriptor Indices
// ========================================
// 这些函数返回 Descriptor Heap 中的物理 slot index

// --- PRIMARY Cache ---
constexpr int PrimaryAtlasSrvIndex(int bufferIndex) 
{ 
    return SURFCACHE_PRIMARY_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 0; 
}

constexpr int PrimaryMetaSrvIndex(int bufferIndex) 
{ 
    return SURFCACHE_PRIMARY_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 1; 
}
constexpr int PrimaryAtlasUavIndex(int bufferIndex) 
{ 
    return SURFCACHE_PRIMARY_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 2; 
}

constexpr int PrimaryMetaUavIndex(int bufferIndex) 
{ 
    return SURFCACHE_PRIMARY_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 3; 
}

// --- GI Cache ---
constexpr int GIAtlasSrvIndex(int bufferIndex) 
{ 
    return SURFCACHE_GI_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 0; 
}

constexpr int GIMetaSrvIndex(int bufferIndex) 
{ 
    return SURFCACHE_GI_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 1; 
}
constexpr int GIAtlasUavIndex(int bufferIndex) 
{ 
    return SURFCACHE_GI_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 2; 
}

constexpr int GIMetaUavIndex(int bufferIndex) 
{ 
    return SURFCACHE_GI_BASE + bufferIndex * DESCRIPTORS_PER_SURFACE_BUFFER + 3; 
}

// ========================================
// Helper Functions: Radiance Cache Descriptor Indices
// ========================================
constexpr int RadianceCacheProbeSrvIndex(int bufferIndex) 
{ 
    return RADIANCE_CACHE_BASE + bufferIndex * DESCRIPTORS_PER_RADIANCE_BUFFER + 0; 
}

constexpr int RadianceCacheProbeUavIndex(int bufferIndex) 
{ 
    return RADIANCE_CACHE_BASE + bufferIndex * DESCRIPTORS_PER_RADIANCE_BUFFER + 1; 
}

constexpr int RadianceCacheUpdateListSrvIndex() 
{ 
    return RADIANCE_CACHE_UPDATE_LIST_SRV; 
}

static constexpr int s_maxCardsPerBatch = 128;
static constexpr int s_maxProbesPerUpdate = 256;

struct SurfaceCacheConstants
{
    float ScreenWidth;
    float ScreenHeight;
    float AtlasWidth;
    float AtlasHeight;

    uint32_t TileSize;  //Default Card Resolution
    uint32_t TilesPerRow; //这个可以优化掉，之后如果有还要往这里加的东西
    uint32_t CurrentFrame;
    uint32_t ActiveCardCount;  

    Mat44 ViewProj;
    Mat44 ViewProjInverse;
    Mat44 PrevViewProj;

    Vec3 CameraPosition;
    float TemporalBlend;

    //IntVec4 DirtyCardIndices[s_maxCardsPerBatch / 4];
};

enum SurfaceCacheType
{
	SURFACE_CACHE_TYPE_PRIMARY,
	//SURFACE_CACHE_TYPE_REFLECTION,
	SURFACE_CACHE_TYPE_GI,
	SURFACE_CACHE_TYPE_COUNT
};

struct SDFGenerationConstants
{
	Vec3 BoundsMin;
	uint32_t Resolution;
	Vec3 BoundsMax;
	uint32_t NumTriangles;
};

struct CardCaptureConstants
{
    //Mat44 CardViewMatrix;        // Card的view matrix
    //Mat44 CardProjectionMatrix;  // Card的正交投影

    Vec3 CardOrigin;             // Card在世界空间的原点
    float padding0;
    Vec3 CardAxisX;              // Card的X轴（世界空间）
    float padding1;
    Vec3 CardAxisY;              // Card的Y轴（世界空间）
    float padding2;
    Vec3 CardNormal;             // Card的法线（世界空间）
    float CaptureDepth;         // 当前捕获深度

    Vec2 CardSize;               // Card的世界空间尺寸
    uint32_t CaptureDirection;   // 当前捕获方向 (0-5)
    uint32_t Resolution;         // Card分辨率

    // ✅ LightMask支持（从CardInstanceData传入）
    uint32_t LightMask[4];       // 支持128个lights (4 * 32 bits)
};

struct RadianceCacheConstants
{
    uint32_t MaxProbes;
    uint32_t ActiveProbeCount;
    uint32_t UpdateProbeCount;
    uint32_t RaysPerProbe;
    
    float CameraPositionX;
    float CameraPositionY;
    float CameraPositionZ;
    float Padding0;
    
    Mat44 ViewProj;
    Mat44 ViewProjInverse;
    
    float ScreenWidth;
    float ScreenHeight;
    uint32_t CurrentFrame;
    float TemporalBlend;
    
    float MaxTraceDistance;
    float ProbeSpacing;
    uint32_t ActiveCardCount;
    float Padding1;
    
    uint32_t AtlasWidth;
    uint32_t AtlasHeight;
    uint32_t TileSize;
    uint32_t BVHNodeCount;
};

struct RenderItem //TODO: 删掉~
{
    Mat44 m_worldMatrix;
    uint32_t m_meshID;
    uint32_t m_materialID;
    uint32_t m_objectID;
    AABB3 m_bounds;
    bool m_visible;
};

enum class ActivePass
{
    BACKBUFFER,
    GBUFFER,
    CARDCAPTURE,
    UNKNOWN
};
