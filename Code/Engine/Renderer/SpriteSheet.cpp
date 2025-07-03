#include "SpriteSheet.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"

SpriteSheet::SpriteSheet(Texture& texture, IntVec2 const& simpleGridLayOut)
    : m_texture(texture)
	, m_simpleGridLayOut(simpleGridLayOut)
{
	Vec2 spriteUvSize(1.0f / static_cast<float>(simpleGridLayOut.x), 1.0f / static_cast<float>(simpleGridLayOut.y));

	m_spriteDefs.reserve(simpleGridLayOut.x * simpleGridLayOut.y);

	float uOffset = 1.0f / 262144.0f;
	float vOffset = 1.0f / 65536.0f;

	for (int y = 0; y < simpleGridLayOut.y; ++y)
	{
		for (int x = 0; x < simpleGridLayOut.x; ++x)
		{
			int spriteIndex = x + y * simpleGridLayOut.x;

			float minU = x * spriteUvSize.x + uOffset;
			float maxU = minU + spriteUvSize.x - uOffset;
			float minV = 1.0f - (y + 1) * spriteUvSize.y + vOffset;
			float maxV = minV + spriteUvSize.y - vOffset;

			Vec2 uvAtMins = Vec2(minU, minV);
			Vec2 uvAtMaxs = Vec2(maxU, maxV);

			m_spriteDefs.push_back(SpriteDefinition(*this, spriteIndex, uvAtMins, uvAtMaxs));
		}
	}
}

Texture& SpriteSheet::GetTexture() const
{
	return m_texture;
}

int SpriteSheet::GetNumSprites() const
{
	return static_cast<int>(m_spriteDefs.size());
}

SpriteDefinition const& SpriteSheet::GetSpriteDef(int spriteIndex) const
{
	if (spriteIndex < 0)
		return m_spriteDefs[0];
	return m_spriteDefs[spriteIndex];
}

void SpriteSheet::GetSpriteUVs(Vec2& out_uvAtMins, Vec2& out_uvAtMaxs, int spriteIndex) const
{
	m_spriteDefs[spriteIndex].GetUVs(out_uvAtMins, out_uvAtMaxs);
}

AABB2 SpriteSheet::GetSpriteUVs(int spriteIndex) const
{
	return m_spriteDefs[spriteIndex].GetUVs();
}

AABB2 SpriteSheet::GetSpriteUVs(IntVec2 tileCoords) const
{
	int convertedSpriteIndex = tileCoords.x + tileCoords.y * m_simpleGridLayOut.x;
	return  m_spriteDefs[convertedSpriteIndex].GetUVs();
}
