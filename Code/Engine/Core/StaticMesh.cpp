#include "StaticMesh.h"

#include <ThirdParty/cgltf/cgltf.h>

#include "Engine/Core/StaticMeshUtils.h"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/DX12Renderer.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Renderer/Cache/SurfaceCard.h"

StaticMesh::StaticMesh(Renderer* renderer, std::string const& xmlPathNoExtensions, bool enableCardTemplates)
    //:m_renderer(renderer)
{
    XmlDocument meshDefDoc; 
    XmlResult meshLoadResult = meshDefDoc.LoadFile((xmlPathNoExtensions+".xml").c_str());
    if (meshLoadResult != XmlResult::XML_SUCCESS)
    {
        ERROR_AND_DIE(Stringf("Cannot load this static mesh!"))
    }

    XmlElement* meshElement = meshDefDoc.RootElement();
    
    m_filePath = ParseXmlAttribute(*meshElement, "objFile", "");
    bool flipUV = ParseXmlAttribute(*meshElement, "flipUV", false);
    if (m_filePath.empty())
        ERROR_AND_DIE("Failed to read mesh path!")

    std::string mtlPath = ParseXmlAttribute(*meshElement, "mtllib", "");
    
    bool isLoaded = LoadStaticMeshFile(m_verts, m_indices, m_filePath, flipUV, mtlPath);
    if (!isLoaded)
        ERROR_AND_DIE("Failed to load the mesh!")

    m_vertexBuffer = renderer->CreateVertexBuffer((unsigned int)m_verts.size() * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
    m_indexBuffer = renderer->CreateIndexBuffer((unsigned int)m_indices.size() * sizeof(unsigned int), sizeof(unsigned int));
    renderer->CopyCPUToGPU(m_verts.data(), (unsigned int)(m_verts.size() * sizeof(Vertex_PCUTBN)), m_vertexBuffer);
    renderer->CopyCPUToGPU(m_indices.data(), (unsigned int)(m_indices.size() * sizeof(unsigned int)), m_indexBuffer);
    
    std::string normalTexture = ParseXmlAttribute(*meshElement, "normalMap", "");
    std::string diffuseTexture = ParseXmlAttribute(*meshElement, "diffuseMap", "");
    std::string specularTexture = ParseXmlAttribute(*meshElement, "specGlossEmitMap", "");
    std::string shader = ParseXmlAttribute(*meshElement, "shader", "");
    if (!normalTexture.empty())
        m_normalTexture = renderer->CreateTextureFromFile(normalTexture.c_str());
    if (!diffuseTexture.empty())
        m_diffuseTexture = renderer->CreateTextureFromFile(diffuseTexture.c_str());
    if (!specularTexture.empty())
        m_specularTexture = renderer->CreateTextureFromFile(specularTexture.c_str());
    if (!shader.empty())
        m_shader = renderer->CreateOrGetShader(shader.c_str(), VertexType::VERTEX_PCUTBN);

    if (EndsWith(m_filePath, ".glb"))
    {
        Image* diffuseI = LoadImageDueToGLTFData(LoadGLTFDataFromFile(m_filePath), GLBChannel::Albedo, m_filePath);
        if (diffuseI)
        {
            diffuseI->SetName(xmlPathNoExtensions + "_diffuse");
            m_diffuseTexture = renderer->CreateTextureFromImage(*diffuseI);
#ifdef ENGINE_DX12_RENDERER
			renderer->GetSubRenderer()->PushBackNewTextureManually(m_diffuseTexture);
#endif
        }
        Image* normalI = LoadImageDueToGLTFData(LoadGLTFDataFromFile(m_filePath), GLBChannel::Normal, m_filePath);
        if (normalI)
        {
            diffuseI->SetName(xmlPathNoExtensions + "_normal");
            m_normalTexture = renderer->CreateTextureFromImage(*normalI);
#ifdef ENGINE_DX12_RENDERER
			renderer->GetSubRenderer()->PushBackNewTextureManually(m_normalTexture);
#endif
        }
        Image* specI = LoadImageDueToGLTFData(LoadGLTFDataFromFile(m_filePath), GLBChannel::AO, m_filePath);
        if (specI)
        {
            diffuseI->SetName(xmlPathNoExtensions + "_ao");
            m_specularTexture = renderer->CreateTextureFromImage(*specI);
#ifdef ENGINE_DX12_RENDERER
			renderer->GetSubRenderer()->PushBackNewTextureManually(m_specularTexture);
#endif
        }
    }
    
    std::string x = ParseXmlAttribute(*meshElement, "x", "");
    std::string y = ParseXmlAttribute(*meshElement, "y", "");
    std::string z = ParseXmlAttribute(*meshElement, "z", "");
	Vec3 I = Vec3(1, 0, 0);  
	Vec3 J = Vec3(0, 1, 0);  
	Vec3 K = Vec3(0, 0, 1);  
	bool hasI = false, hasJ = false, hasK = false;

	if (!x.empty()) 
    {
		if (x == "left") { I = Vec3(0, +1, 0); hasI = true; }
		else if (x == "right") { I = Vec3(0, -1, 0); hasI = true; }
		else if (x == "up") { I = Vec3(0, 0, +1); hasI = true; }
		else if (x == "down") { I = Vec3(0, 0, -1); hasI = true; }
		else if (x == "forward") { I = Vec3(+1, 0, 0); hasI = true; }
		else if (x == "back") { I = Vec3(-1, 0, 0); hasI = true; }
	}

	if (!y.empty()) 
    {
		if (y == "left") { J = Vec3(0, +1, 0); hasJ = true; }
		else if (y == "right") { J = Vec3(0, -1, 0); hasJ = true; }
		else if (y == "up") { J = Vec3(0, 0, +1); hasJ = true; }
		else if (y == "down") { J = Vec3(0, 0, -1); hasJ = true; }
		else if (y == "forward") { J = Vec3(+1, 0, 0); hasJ = true; }
		else if (y == "back") { J = Vec3(-1, 0, 0); hasJ = true; }
	}

	if (!z.empty()) 
    {
		if (z == "left") { K = Vec3(0, +1, 0); hasK = true; }
		else if (z == "right") { K = Vec3(0, -1, 0); hasK = true; }
		else if (z == "up") { K = Vec3(0, 0, +1); hasK = true; }
		else if (z == "down") { K = Vec3(0, 0, -1); hasK = true; }
		else if (z == "forward") { K = Vec3(+1, 0, 0); hasK = true; }
		else if (z == "back") { K = Vec3(-1, 0, 0); hasK = true; }
	}

	int count = (int)hasI + (int)hasJ + (int)hasK;
	if (count == 1) 
    {
		if (hasI) 
        {
			Vec3 temp = (std::fabs(I.z) < 0.9f) ? Vec3(0, 0, 1) : Vec3(0, 1, 0);
			J = CrossProduct3D(temp, I);       
			K = CrossProduct3D(I, J);
		}
		else if (hasJ) 
        {
			Vec3 temp = (std::fabs(J.z) < 0.9f) ? Vec3(0, 0, 1) : Vec3(1, 0, 0);
			K = CrossProduct3D(J, temp);
			I = CrossProduct3D(J, K);       
		}
		else 
        { 
			Vec3 temp = (std::fabs(K.y) < 0.9f) ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
			I = CrossProduct3D(K, temp);
			J = CrossProduct3D(K, I);      
		}
	}
	else if (count == 2) 
    {
		if (!hasK) { K = CrossProduct3D(I, J); }
		else if (!hasJ) { J = CrossProduct3D(K, I); }
		else if (!hasI) { I = CrossProduct3D(J, K); }
	}
	Mat44 M;
	M.SetIJK3D(I, J, K);
	M.Orthonormalize_IFwd_JLeft_KUp();
	m_transform = M;

    m_unitsPerMeter = ParseXmlAttribute(*meshElement, "unitsPerMeter", 1.f);
    m_modelRelativeScale = 1.f / m_unitsPerMeter;
    Mat44 mat;
    mat = mat.MakeUniformScale3D(1/m_unitsPerMeter);
	m_transformWithoutAxisTransform = mat;
    m_transform.Append(mat);
	
    Vec3 translation = ParseXmlAttribute(*meshElement, "translation", Vec3(0.f, 0.f, 0.f));
    Mat44 translate;
    translate.SetTranslation3D(translation);
    m_transform.Append(translate);
	m_transformWithoutAxisTransform.Append(translate);

    //ApplyTransformToVertices();

	if (enableCardTemplates)
	{
		GenerateCardTemplates();
	}
}

StaticMesh::~StaticMesh()
{
    delete m_vertexBuffer;
    m_vertexBuffer = nullptr;
    delete m_indexBuffer;
    m_indexBuffer = nullptr;
}

void StaticMesh::GenerateCardTemplates()
{
	if (m_hasCardTemplates)
		return;

	AABB3 bounds = GetAABB3Bounds();
	Vec3 size = bounds.m_maxs - bounds.m_mins;

	for (int dir = 0; dir < 6; dir++)
	{
		SurfaceCardTemplate templ;
		templ.m_direction = (uint8_t)dir;

		switch (dir)
		{
		case 0: case 1:  // ±X
			templ.m_localSize = Vec2(size.y, size.z);
			break;
		case 2: case 3:  // ±Y
			templ.m_localSize = Vec2(size.x, size.z);
			break;
		case 4: case 5:  // ±Z
			templ.m_localSize = Vec2(size.x, size.y);
			break;
		}

		float maxDim = max(templ.m_localSize.x, templ.m_localSize.y);
		int resolution;

		if (maxDim < 1.0f)
			resolution = 32;        
		else if (maxDim < 3.0f)
			resolution = 64;        
		else if (maxDim < 8.0f)
			resolution = 128;       
		else if (maxDim < 16.0f)
			resolution = 256;       
		else
			resolution = 512;       

		templ.m_recommendedResolution = IntVec2(resolution, resolution);

		m_cardTemplates.push_back(templ);
	}

	m_hasCardTemplates = true;
}

SurfaceCardTemplate* StaticMesh::GetCardTemplate(uint8_t direction)
{
	if (!m_hasCardTemplates || direction >= m_cardTemplates.size())
		return nullptr;
	return &m_cardTemplates[direction];
}

const SurfaceCardTemplate* StaticMesh::GetCardTemplate(uint8_t direction) const
{
	if (!m_hasCardTemplates || direction >= m_cardTemplates.size())
		return nullptr;
	return &m_cardTemplates[direction];
}

Sphere StaticMesh::GetBoundsSphere() const
{
    Sphere bs;
    if (m_verts.empty())
        return bs;

    Vec3 c = Vec3();
    for (const auto& v : m_verts) c += v.m_position;
    c /= static_cast<float>(m_verts.size());

    float r2 = 0.f;
    for (const auto& v : m_verts)
    {
        r2 = MaxF(r2, (v.m_position - c).GetLengthSquared());
    }

    bs.m_center = c;
    bs.m_radius = sqrtf(r2) * m_modelRelativeScale;

    return bs;
}

Sphere StaticMesh::GetTransformedBoundsSphere() const
{
	Sphere bs;

	if (m_verts.empty())
		return bs;

	std::vector<Vertex_PCUTBN> transformed = GetTransformedVertices();

	Vec3 center = Vec3();
	for (const auto& v : transformed)
	{
		center += v.m_position;
	}
	center /= static_cast<float>(transformed.size());

	float maxRadiusSq = 0.f;
	for (const auto& v : transformed)
	{
		maxRadiusSq = MaxF(maxRadiusSq, (v.m_position - center).GetLengthSquared());
	}

	bs.m_center = center;
	bs.m_radius = sqrtf(maxRadiusSq);

	return bs;
}

AABB3 StaticMesh::GetAABB3Bounds() const
{
    AABB3 box;
    Vec3 mn = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 mx = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (Vertex_PCUTBN vert : m_verts)
    {
        Vec3 p = vert.m_position;
		mn.x = MinF(mn.x, p.x); 
        mn.y = MinF(mn.y, p.y); 
        mn.z = MinF(mn.z, p.z);

		mx.x = MaxF(mx.x, p.x); 
        mx.y = MaxF(mx.y, p.y); 
        mx.z = MaxF(mx.z, p.z);

		box.m_mins = mn; 
        box.m_maxs = mx;
    }
    return box;
}

void StaticMesh::ApplyTransformToVertices()
{
	if (m_verts.empty())
		return;

	DebuggerPrintf("[StaticMesh] Applying transform to %zu vertices (scale: %f)\n",
		m_verts.size(), m_modelRelativeScale);

	for (auto& vert : m_verts)
	{
		vert.m_position = m_transform.TransformPosition3D(vert.m_position);

		// 变换法线（仅旋转）
		Mat44 rotationOnly = m_transform;
		rotationOnly.SetTranslation3D(Vec3(0, 0, 0));

		vert.m_normal = rotationOnly.TransformVectorQuantity3D(vert.m_normal);
		vert.m_normal.GetNormalized();

		vert.m_tangent = rotationOnly.TransformVectorQuantity3D(vert.m_tangent);
		vert.m_tangent.GetNormalized();

		vert.m_bitangent = rotationOnly.TransformVectorQuantity3D(vert.m_bitangent);
		vert.m_bitangent.GetNormalized();
	}

	m_transform = Mat44();

	DebuggerPrintf("[StaticMesh] Transform applied and reset to identity\n");
}

float StaticMesh::QuantizeScale(float scale)
{
	return std::round(scale * 10.0f) / 10.0f;
}

AABB3 StaticMesh::GetScaledBounds(float scale) const //Transformed 版本
{
	AABB3 bounds = GetTransformedAABB3Bounds();

	Vec3 center = (bounds.m_mins + bounds.m_maxs) * 0.5f;
	Vec3 halfSize = (bounds.m_maxs - bounds.m_mins) * 0.5f * scale;

	return AABB3(center - halfSize, center + halfSize);
}

AABB3 StaticMesh::GetTransformedAABB3Bounds() const
{
	AABB3 box;

	if (m_verts.empty())
		return box;

	std::vector<Vertex_PCUTBN> transformed = GetTransformedVertices();

	for (const auto& vert : transformed)
	{
		box.StretchToIncludePoint(vert.m_position);
	}

	return box;
}

AABB3 StaticMesh::GetTransformedAABB3BoundsWithoutAxisTransform() const
{
	AABB3 box;

	if (m_verts.empty())
		return box;

	std::vector<Vertex_PCUTBN> transformed = GetTransformedVerticesWithoutAxisTransform();

	for (const auto& vert : transformed)
	{
		box.StretchToIncludePoint(vert.m_position);
	}

	return box;
}

void StaticMesh::BuildBVH(float scale)
{
	float quantized = QuantizeScale(scale);

	if (m_bvhsByScale.find(quantized) != m_bvhsByScale.end())
	{
		DebuggerPrintf("[StaticMesh] BVH already exists for scale=%.1f\n", quantized);
		return;
	}

	if (m_verts.empty() || m_indices.empty())
	{
		DebuggerPrintf("[StaticMesh] Cannot build BVH: empty data\n");
		return;
	}

	DebuggerPrintf("[StaticMesh] Building BVH for scale=%.1f with %zu verts, %zu indices\n",
		quantized, m_verts.size(), m_indices.size());

	std::vector<Vertex_PCUTBN> scaledVerts = GetScaledAndTransformedVertices(scale);

	BVH bvh;
	bvh.Build(scaledVerts, m_indices);

	m_bvhsByScale.emplace(quantized, std::move(bvh));

	DebuggerPrintf("[StaticMesh] BVH built successfully for scale=%.1f\n", quantized);
}

bool StaticMesh::HasBVH(float scale) const
{
	float quantized = QuantizeScale(scale);
	return m_bvhsByScale.find(quantized) != m_bvhsByScale.end();
}

const BVH* StaticMesh::GetBVH(float scale) const
{
	float quantized = QuantizeScale(scale);
	auto it = m_bvhsByScale.find(quantized);
	if (it != m_bvhsByScale.end())
	{
		return &it->second;
	}
	return nullptr;
}

SDFTexture3D* StaticMesh::GetSDF(float scale) const
{
	float quantized = QuantizeScale(scale);
    
	auto it = m_sdfsByScale.find(quantized);
	if (it != m_sdfsByScale.end())
	{
		return it->second;
	}
    
	return nullptr;
}

void StaticMesh::SetSDF(float scale, SDFTexture3D* sdf)
{
	float quantized = QuantizeScale(scale);
	m_sdfsByScale[quantized] = sdf;
    
	DebuggerPrintf("[StaticMesh] Registered SDF for scale=%.1f\n", quantized);
}

bool StaticMesh::HasSDF(float scale) const
{
	return GetSDF(scale) != nullptr;
}

std::vector<Vertex_PCUTBN> StaticMesh::GetScaledAndTransformedVertices(float scale) const
{
	std::vector<Vertex_PCUTBN> transformed = GetTransformedVertices();

	// 应用额外的scale（来自MeshObject）
	if (scale != 1.0f)
	{
		for (auto& vert : transformed)
		{
			vert.m_position *= scale;
		}
	}

	return transformed;
}

std::vector<Vertex_PCUTBN> StaticMesh::GetTransformedVertices() const
{
	std::vector<Vertex_PCUTBN> transformed = m_verts;

	for (auto& vert : transformed)
	{
		// 应用完整的mesh transform（轴变换 + scale + translation）
		vert.m_position = m_transform.TransformPosition3D(vert.m_position);

		// 法线只应用旋转（不受scale和translation影响）
		vert.m_normal = m_transform.TransformVectorQuantity3D(vert.m_normal);
		vert.m_normal.GetNormalized();

		vert.m_tangent = m_transform.TransformVectorQuantity3D(vert.m_tangent);
		vert.m_tangent.GetNormalized();

		vert.m_bitangent = m_transform.TransformVectorQuantity3D(vert.m_bitangent);
		vert.m_bitangent.GetNormalized();
	}

	return transformed;
}

std::vector<Vertex_PCUTBN> StaticMesh::GetTransformedVerticesWithoutAxisTransform() const
{
	std::vector<Vertex_PCUTBN> transformed = m_verts;

	for (auto& vert : transformed)
	{
		// 应用完整的mesh transform（轴变换 + scale + translation）
		vert.m_position = m_transformWithoutAxisTransform.TransformPosition3D(vert.m_position);

		// 法线只应用旋转（不受scale和translation影响）
		vert.m_normal = m_transformWithoutAxisTransform.TransformVectorQuantity3D(vert.m_normal);
		vert.m_normal.GetNormalized();

		vert.m_tangent = m_transformWithoutAxisTransform.TransformVectorQuantity3D(vert.m_tangent);
		vert.m_tangent.GetNormalized();

		vert.m_bitangent = m_transformWithoutAxisTransform.TransformVectorQuantity3D(vert.m_bitangent);
		vert.m_bitangent.GetNormalized();
	}

	return transformed;
}
