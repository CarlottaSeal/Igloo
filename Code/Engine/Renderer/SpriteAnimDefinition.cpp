#include "SpriteAnimDefinition.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Core/XmlUtils.hpp"

SpriteAnimDefinition::SpriteAnimDefinition(SpriteSheet const& sheet, int startSpriteIndex, int endSpriteIndex, float framesPerSecond, SpriteAnimPlaybackType playbackType)
	:m_spriteSheet(&sheet)
	,m_startSpriteIndex(startSpriteIndex)
	,m_endSpriteIndex(endSpriteIndex)
	,m_framesPerSecond(framesPerSecond)
	,m_playbackType(playbackType)
{
	m_secondsPerFrame = 1.f / framesPerSecond;
	m_animationPeriod = (float)(endSpriteIndex - startSpriteIndex +1)*m_secondsPerFrame;
}

SpriteDefinition const& SpriteAnimDefinition::GetSpriteDefAtTime(float seconds) const
{
	int totalFrames = m_endSpriteIndex - m_startSpriteIndex + 1;
	//int frameIndex = static_cast<int>(seconds * m_framesPerSecond) % totalFrames;
	int frameIndex = 0;
	if (m_playbackType == SpriteAnimPlaybackType::LOOP)
	{
		float totalAnimationTime = totalFrames / m_framesPerSecond;
		float timeInLoop = static_cast<float>(fmod(seconds, totalAnimationTime));

		frameIndex = static_cast<int>(timeInLoop * m_framesPerSecond);
		frameIndex = m_startSpriteIndex + frameIndex;
	}
	else if (m_playbackType == SpriteAnimPlaybackType::PINGPONG)
	{
		float totalAnimationTime = totalFrames / m_framesPerSecond;
		float timeInLoop = static_cast<float>(fmod(seconds, totalAnimationTime));
		
		if (timeInLoop < (totalAnimationTime / 2.0f)) 
		{
			float progress = timeInLoop / (totalAnimationTime / 2.0f);
			frameIndex = m_startSpriteIndex + static_cast<int>((m_endSpriteIndex - m_startSpriteIndex) * progress);
		}
		else
		{
			float progress = (timeInLoop - (totalAnimationTime / 2.0f)) / (totalAnimationTime / 2.0f);
			frameIndex = m_endSpriteIndex - static_cast<int>((m_endSpriteIndex - m_startSpriteIndex) * progress);
		}

		//frameIndex = std::max(m_startSpriteIndex, std::min(frameIndex, m_endSpriteIndex));
		frameIndex = GetClampedInt(frameIndex, m_startSpriteIndex, m_endSpriteIndex);
	}
	else if(m_playbackType == SpriteAnimPlaybackType::ONCE)
	{
		float totalAnimationTime = (m_endSpriteIndex - m_startSpriteIndex + 1) / m_framesPerSecond;
		float timeInAnimation = static_cast<float>(fmin(seconds, totalAnimationTime));
		
		if (seconds > totalAnimationTime)
		{
			frameIndex = m_endSpriteIndex;
		}

		else
		{
			frameIndex = static_cast<int>(timeInAnimation * m_framesPerSecond);
			frameIndex = m_startSpriteIndex + frameIndex;

			if (frameIndex > m_endSpriteIndex)
			{
				frameIndex = m_endSpriteIndex;
			}
		}	
	}

	return m_spriteSheet->GetSpriteDef(frameIndex);
}

SpriteAnimDefinition::SpriteAnimDefinition(XmlElement& spriteAnimDef, SpriteSheet const& sheet, float secondsPerFrame,
	SpriteAnimPlaybackType playbackType)
	: m_spriteSheet(&sheet)
	, m_secondsPerFrame(secondsPerFrame)
	, m_playbackType(playbackType)
{
	m_startSpriteIndex	= ParseXmlAttribute(spriteAnimDef, "startFrame", 0);
	m_endSpriteIndex	= ParseXmlAttribute(spriteAnimDef, "endFrame", 0);
	m_framesPerSecond = 1.f / secondsPerFrame;
	m_animationPeriod = (float)(m_endSpriteIndex - m_startSpriteIndex +1)*m_secondsPerFrame;
}

float SpriteAnimDefinition::GetAnimationDuration() const
{
	return m_animationPeriod;
}

// void SpriteAnimeRenderCache::UpdateIfChanged(float timeInAnim, SpriteAnimDefinition* animDef, const std::vector<AABB2>& boxes,
//                                              const Rgba8& tint)
// {
// 	const SpriteDefinition* currentDef = &animDef->GetSpriteDefAtTime(timeInAnim);
// 	if (currentDef == m_lastSpriteDef && animDef == m_lastAnimDef)
// 	{
// 		return;
// 	}
//
// 	// build verts
// 	m_cachedVerts.clear();
// 	AABB2 uvs = currentDef->GetUVs();
// 	for (const AABB2& box : boxes)
// 	{
// 	}
// }