#pragma once
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Math/AABB2.hpp"
#include <vector>
#include <string>

bool LoadStaticMeshFile(std::vector<Vertex_PCUTBN>& verts, std::string const& filePathNoExtension);
bool LoadOBJMeshFile(std::vector<Vertex_PCUTBN>& verts, std::string const& filePathNoExtension);
void ComputeMissingNormals(std::vector<Vertex_PCUTBN>& verts);
void ComputeMissingTangentsBitangents(std::vector<Vertex_PCUTBN>& verts);

