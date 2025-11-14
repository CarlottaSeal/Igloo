#pragma once
#include <string>
#include <vector>

#include "Vertex_PCUTBN.hpp"
#include "Engine/Math/Sphere.h"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Scene/BVH.h"

struct SurfaceCardTemplate;
class SDFTexture3D;

enum class StaticMeshType
{
    OBJ,
    GLB,
    COUNT
};

enum class GLBChannel
{
    Albedo,
    Normal,
    AO,
    Metallic,
    Roughness,
    Count
};

class StaticMesh
{
public:
    StaticMesh(Renderer* renderer, std::string const& xmlPathNoExtensions, bool enableCardTemplates = false);
    ~StaticMesh();

    void GenerateCardTemplates();
    SurfaceCardTemplate* GetCardTemplate(uint8_t direction);
    const SurfaceCardTemplate* GetCardTemplate(uint8_t direction) const;

    Sphere GetBoundsSphere() const;
    Sphere GetTransformedBoundsSphere() const;
    AABB3 GetAABB3Bounds() const;
    AABB3 GetScaledBounds(float scale) const;
    AABB3 GetTransformedAABB3Bounds() const;
    AABB3 GetTransformedAABB3BoundsWithoutAxisTransform() const;

	void BuildBVH(float scale = 1.0f);
	bool HasBVH(float scale = 1.0f) const;
	const BVH* GetBVH(float scale = 1.0f) const;

	SDFTexture3D* GetSDF(float scale) const;
	void SetSDF(float scale, SDFTexture3D* sdf);
	bool HasSDF(float scale) const;

    std::vector<Vertex_PCUTBN> GetScaledAndTransformedVertices(float scale) const;
    std::vector<Vertex_PCUTBN> GetTransformedVertices() const;
    std::vector<Vertex_PCUTBN> GetTransformedVerticesWithoutAxisTransform() const;

private:
    void ApplyTransformToVertices();
    static float QuantizeScale(float scale);

public:
    std::string m_filePath;
    std::vector<Vertex_PCUTBN> m_verts;
    std::vector<unsigned int> m_indices;
    Texture* m_normalTexture = nullptr;
    Texture* m_diffuseTexture = nullptr;
    Texture* m_specularTexture = nullptr;
    Shader* m_shader = nullptr;

    float m_unitsPerMeter;
    float m_modelRelativeScale;

    std::string m_x;
    std::string m_y;
    std::string m_z;

    Mat44 m_transform = Mat44(); //no位移，aligned with SD engine
    Mat44 m_transformWithoutAxisTransform = Mat44(); //no位移no轴变换，aligned with SD engine

    VertexBuffer* m_vertexBuffer = nullptr;
    IndexBuffer* m_indexBuffer = nullptr;
	std::vector<SurfaceCardTemplate> m_cardTemplates;
	bool m_hasCardTemplates = false;

    std::unordered_map<float, BVH> m_bvhsByScale;
	bool m_bvhBuilt = false;

	// scale → sdfResourceID 
    std::map<float, SDFTexture3D*> m_sdfsByScale;
};
