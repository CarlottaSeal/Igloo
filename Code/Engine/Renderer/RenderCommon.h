#pragma once

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

struct CameraConstants
{
    Mat44 WorldToCameraTransform;
    Mat44 CameraToRenderTransform;
    Mat44 RenderToClipTransform;
    
    Vec3 CameraWorldPosition;
    float padding;
};
static const int k_cameraConstantsSlot = 2;

struct ModelConstants
{
    Mat44 ModelToWorldTransform;
    float ModelColor[4];
};
static const int k_modelConstantsSlot = 3;

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

static const int s_maxLights = 8;
struct GeneralLight
{
    float Color[4]          =  {0.f, 0.f, 0.f, 0.f};
    float WorldPosition[3]  = {0.f, 0.f, 0.f};
    float PADDING = 0.f;

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
static const int k_generalLightConstantsSlot = 4;

struct ShadowConstants
{
    Mat44 LightViewProjectionMatrix;
};
static const int k_shadowConstantsSlot = 6;

struct PerFrameConstants  //仅用于debug
{
    float Time;
    int DebugInt;
    float DebugFloat;
    float EMPTY_PADDING;
};
static const int k_perFrameConstantsSlot = 1;

//static int const k_dx12LightConstantsSlot = 0;
//static int const k_dx12CameraConstantsSlot = 2;
//static int const k_dx12ModelConstantsSlot = 3;
