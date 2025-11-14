#pragma once

#ifdef ENGINE_DX11_RENDERER
const char* m_shaderSource = R"(
    cbuffer CameraConstants : register(b2)
    {
        
        float4x4 WorldToCameraTransform;
        float4x4 CameraToRenderTransform;
        float4x4 RenderToClipTransform;
    }
    cbuffer ModelConstants : register(b3)
    {
	    float4x4 ModelToWorldTransform;
	    float4 ModelColor;
    };

    Texture2D diffuseTexture : register(t0);
    SamplerState diffuseSampler : register(s0);

    struct vs_input_t 
	{
        float3 modelSpacePosition : POSITION;
        float4 color : COLOR;
        float2 uv : TEXCOORD;
    };

    struct v2p_t
	{
        float4 clipSpacePosition : SV_Position;
        float4 color : COLOR;
        float2 uv : TEXCOORD;
    };

    v2p_t VertexMain(vs_input_t input) 
	{
        float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
        float4 worldSpacePosition = mul(ModelToWorldTransform, modelSpacePosition);
        float4 cameraSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
        float4 renderSpacePosition = mul(CameraToRenderTransform, cameraSpacePosition);
        float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);

        v2p_t v2p;
        v2p.clipSpacePosition = clipSpacePosition;
        v2p.color = input.color;
        v2p.uv = input.uv;
        return v2p;
    }

    float4 PixelMain(v2p_t input) : SV_Target0 
	{
        float4 textureColor = diffuseTexture.Sample(diffuseSampler, input.uv);
        float4 vertexColor = input.color;
        float4 modelColor = ModelColor;

        clip(textureColor.a - 0.1f);

	    float4 color = textureColor * vertexColor * modelColor;
	    return float4(color);
    }
    )";
#endif
#ifdef ENGINE_DX12_RENDERER
const char* m_shaderSource = R"(

#define MAX_TEXTURE_COUNT 200
    cbuffer CameraConstants : register(b1)
    {
        
        float4x4 WorldToCameraTransform;
        float4x4 CameraToRenderTransform;
        float4x4 RenderToClipTransform;
    }
    cbuffer ModelConstants : register(b2)
    {
	    float4x4 ModelToWorldTransform;
	    float4 ModelColor;
    };
	cbuffer MaterialConstants : register(b3)
	{
		int diffuseTexId;
		int normalTexId;
		int specTexId;
		float padding;
	}

    Texture2D g_textures[MAX_TEXTURE_COUNT] : register(t0);
    SamplerState diffuseSampler : register(s0);

    struct vs_input_t 
	{
        float3 modelSpacePosition : POSITION;
        float4 color : COLOR;
        float2 uv : TEXCOORD;
    };

    struct v2p_t
	{
        float4 clipSpacePosition : SV_Position;
        float4 color : COLOR;
        float2 uv : TEXCOORD;
    };

    v2p_t VertexMain(vs_input_t input) 
	{
        float4 modelSpacePosition = float4(input.modelSpacePosition, 1);
        float4 worldSpacePosition = mul(ModelToWorldTransform, modelSpacePosition);
        float4 cameraSpacePosition = mul(WorldToCameraTransform, worldSpacePosition);
        float4 renderSpacePosition = mul(CameraToRenderTransform, cameraSpacePosition);
        float4 clipSpacePosition = mul(RenderToClipTransform, renderSpacePosition);

        v2p_t v2p;
        v2p.clipSpacePosition = clipSpacePosition;
        v2p.color = input.color;
        v2p.uv = input.uv;
        return v2p;
    }

    float4 PixelMain(v2p_t input) : SV_Target0 
	{
        float4 textureColor = g_textures[diffuseTexId].Sample(diffuseSampler, input.uv);
        float4 vertexColor = input.color;
        float4 modelColor = ModelColor;

        clip(textureColor.a - 0.1f);

	    float4 color = textureColor * vertexColor * modelColor;
	    return float4(color);
    }
    )";
#endif