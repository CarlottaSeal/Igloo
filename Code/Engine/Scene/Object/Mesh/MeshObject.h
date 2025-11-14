#pragma once
#include "Engine/Scene/Object/SceneObject.h"
#include "Engine/Renderer/RenderCommon.h"
#include "Engine/Scene/SDF/SDFCommon.h"
#include "Engine/Scene/Scene.h"

class StaticMesh;
struct SurfaceCard;
struct CardInstanceData;

class MeshObject : public SceneObject
{
    friend class SceneObject;
public:
    MeshObject(uint32_t id, const std::string& name, const std::string& path, Vec3 position, EulerAngles rotation = EulerAngles());

    virtual void OnCreate(Scene* scene) override;
    virtual void OnDestroy() override;

    void InitializeCardInstances();
    const CardInstanceData* GetCardInstance(size_t index) const;
    CardInstanceData* GetCardInstance(size_t index);
    size_t GetCardInstanceCount() const;
    
    StaticMesh* GetMesh() const { return m_mesh; }
    virtual Sphere GetLocalBoundsSphere() const override;
    Sphere GetWorldBoundsSphere() const;
    virtual AABB3 GetLocalBounds() const override;
    virtual AABB3 GetLocalBoundsUntransformed() const;
    virtual AABB3 GetWorldBounds() const override;

    float CalculateCaptureDepth(const CardInstanceData* instance, uint8_t direction);
    
    const std::vector<float>* GetSDFData() const;
    int GetSDFResolution() const;
    
    RenderItem GetRenderItem() const;
    
    void SetStaticForGI(bool isStatic) { m_isStaticForGI = isStatic; }
    bool IsStaticForGI() const { return m_isStaticForGI; }
    virtual void OnTransformChanged() override;

public:
    uint32_t m_sdfResourceID;
    std::vector<CardInstanceData> m_cardInstances;
    //std::vector<SurfaceCard> m_runtimeCards;
    //std::vector<SurfaceCard> m_cards;
    
protected:
	virtual void UpdateWorldMatrix() override;
    StaticMesh* m_mesh;
    std::string m_path;
    
private:
    bool m_isStaticForGI = true;  //TODO: 用处不大
};
