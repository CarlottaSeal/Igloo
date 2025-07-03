#include "XmlUtils.hpp"
#include "Engine/Renderer/SpriteAnimDefinition.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"

int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr != nullptr) 
	{
		return atoi(attr);
	}
	else 
	{
		return defaultValue;
	}
}

char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr != nullptr) 
	{
        return attr[0];
    } 
	else 
	{
        return defaultValue;
    }
}

bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr) 
	{
		return (strcmp(attr, "true") == 0 || strcmp(attr, "1") == 0);
	}
	return defaultValue;
}

float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	return attr ? static_cast<float>(atof(attr)) : defaultValue;
}

Rgba8 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Rgba8 const& defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr) 
	{
		Rgba8 color;
		color.SetFromText(attr);
		return color;
	}
	return defaultValue;
}

Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr) 
	{
		Vec2 vec;
		vec.SetFromText(attr);
		return vec;
	}
	return defaultValue;
}

Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr)
	{
		Vec3 vec;
		vec.SetFromText(attr);
		return vec;
	}
	return defaultValue;
}

EulerAngles ParseXmlAttribute(XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr)
	{
		EulerAngles eulerAngles;
		eulerAngles.SetFromText(attr);
		return eulerAngles;
	}
	return defaultValue;
}

IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr) 
	{
		IntVec2 vec;
		vec.SetFromText(attr);
		return vec;
	}
	return defaultValue;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr != nullptr) 
	{
		return std::string(attr);
	}
	else 
	{
		return defaultValue;
	}
}

BillboardType ParseXmlAttribute(XmlElement const& element, char const* attributeName, BillboardType const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	std::string value = LowerAll(text); 

	if (value == "worldupfacing") return BillboardType::WORLD_UP_FACING;
	if (value == "worldupopposing") return BillboardType::WORLD_UP_OPPOSING;
	if (value == "fullfacing") return BillboardType::FULL_FACING;
	if (value == "fullopposing") return BillboardType::FULL_OPPOSING;
	if (value == "none") return BillboardType::NONE;

	return defaultValue;
}

SpriteAnimPlaybackType ParseXmlAttribute(XmlElement const& element, char const* attributeName,
	SpriteAnimPlaybackType const& defaultValue)
{
	const char* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	std::string value = LowerAll(text);

	if (value == "loop") return SpriteAnimPlaybackType::LOOP;
	if (value == "once") return SpriteAnimPlaybackType::ONCE;
	if (value == "pingpong") return SpriteAnimPlaybackType::PINGPONG;
	else return defaultValue;
}

Strings ParseXmlAttribute(XmlElement const& element, char const* attributeName, Strings const& defaultValues)
{
	const char* attr = element.Attribute(attributeName);
	if (attr) 
	{
		return SplitStringOnDelimiter(attr, ',');
	}
	return defaultValues;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr != nullptr) 
	{
		return std::string(attr);
	}
	else 
	{
		return defaultValue;
	}
}

FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue)
{
	const char* attr = element.Attribute(attributeName);
	if (attr)
	{
		FloatRange range;
		range.SetFromText(attr);
		return range;
	}
	return defaultValue;
}

