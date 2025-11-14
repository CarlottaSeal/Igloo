#include "MeshManager.h"

#include "Engine/Scene/Scene.h"

MeshManager::MeshManager(Scene* scene)
    : m_scene(scene)
{
}

MeshManager::~MeshManager()
{
    for (auto& pair : m_loadedMeshes)
    {
        delete pair.second;
        pair.second = nullptr;
    }
    m_loadedMeshes.clear();
}

StaticMesh* MeshManager::GetOrLoadMesh(const std::string& name, const std::string& path)
{
    if (m_loadedMeshes.find(name) == m_loadedMeshes.end())
    {
        m_loadedMeshes[name] = new StaticMesh((Renderer*)m_scene->m_config.m_renderer, path, true);
    }
    return m_loadedMeshes[name];
}
