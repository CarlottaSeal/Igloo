#pragma once

#include "Engine/Core/XmlUtils.hpp"

struct Vertex_PCU;
struct AABB2;
class Texture;
class SpriteSheet;
class SpriteDefinition;

enum class SpriteAnimPlaybackType
{
	ONCE, // for 5-frame animation, plays 0,1,2,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4...
	LOOP, // for 5-frame animation, plays 0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0,1,2,3,4,0...
	PINGPONG, // for 5-frame animation, plays 0,1,2,3,4,3,2,1,0,1,2,3,4,3,2,1,0,1,2,3,4,3,2,1,0,1...
};

class SpriteAnimDefinition
{
public:
	SpriteAnimDefinition(SpriteSheet const& sheet, int startSpriteIndex, int endSpriteIndex,
		float framesPerSecond, SpriteAnimPlaybackType playbackType = SpriteAnimPlaybackType::LOOP);
	SpriteAnimDefinition(XmlElement& spriteAnimDef, SpriteSheet const& sheet, float secondsPerFrame, SpriteAnimPlaybackType playbackType);
	
	SpriteDefinition const& GetSpriteDefAtTime(float seconds) const; // Most of the logic for this class is done here!
	float GetAnimationDuration() const;
private:
	SpriteSheet const* m_spriteSheet;
	int m_startSpriteIndex = -1;
	int m_endSpriteIndex = -1;
	float m_framesPerSecond = 1.f;
	float m_secondsPerFrame = 0.25f;
	float m_animationPeriod = 0.f;
	SpriteAnimPlaybackType m_playbackType = SpriteAnimPlaybackType::LOOP;
};

// class SpriteAnimeRenderCache
// {
// public:
// 	void UpdateIfChanged(float timeInAnim, SpriteAnimDefinition* animDef, const std::vector<AABB2>& boxes, const Rgba8& tint = Rgba8::WHITE);
//
// public:
// 	std::vector<Vertex_PCU> m_cachedVerts;
// 	const SpriteDefinition* m_lastSpriteDef = nullptr;
// 	SpriteAnimDefinition* m_lastAnimDef = nullptr;
// };