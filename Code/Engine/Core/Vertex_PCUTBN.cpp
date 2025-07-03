#include "Vertex_PCUTBN.hpp"

Vertex_PCUTBN::Vertex_PCUTBN(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords, Vec3 const& tangent, Vec3 const& bitangent, Vec3 const& normal)
	: m_position(position)
	, m_color(color)
	, m_uvTexCoords(uvTexCoords)
	, m_tangent(tangent)
	, m_bitangent(bitangent)
	, m_normal(normal)
{
}

Vertex_PCUTBN::Vertex_PCUTBN(float px, float py, float pz, unsigned char r, unsigned char g, unsigned char b, unsigned char a, float u, float v, float tx, float ty, float tz, float bx, float by, float bz, float nx, float ny, float nz)
	: m_position(px, py, pz)
	, m_color(r, g, b, a)
	, m_uvTexCoords(u, v)
	, m_tangent(tx, ty, tz)
	, m_bitangent(bx, by, bz)
	, m_normal(nx, ny, nz)
{
}
