#include "NamedStrings.hpp"
#include "Engine/Core/XmlUtils.hpp" 
#include "Engine/Core/Rgba8.hpp"    
#include "Engine/Math/Vec2.hpp"     
#include "Engine/Math/IntVec2.hpp"  
#include <map>
#include <string>
#include <cstdlib>    

void NamedStrings::PopulateFromXmlElementAttributes(XmlElement const& element)
{
	const XmlAttribute* attribute = element.FirstAttribute();
	while (attribute) 
    {
		std::string keyName = attribute->Name();
		std::string value = attribute->Value();
		m_keyValuePairs[keyName] = value;
		attribute = attribute->Next();
	}
}

void NamedStrings::SetValue(std::string const& keyName, std::string const& newValue)
{
    m_keyValuePairs[keyName] = newValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, std::string const& defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end())
	{
		return iter->second;
	}
	return defaultValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, char const* defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end())
	{
		return iter->second;
	}
	return std::string(defaultValue);
}

bool NamedStrings::GetValue(std::string const& keyName, bool defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end()) 
	{
		std::string value = iter->second;
		return (value == "true" || value == "1");
	}
	return defaultValue;
}

int NamedStrings::GetValue(std::string const& keyName, int defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end()) 
	{
		return atoi(iter->second.c_str());
	}
	return defaultValue;
}

float NamedStrings::GetValue(std::string const& keyName, float defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end()) 
	{
		return static_cast<float>(atof(iter->second.c_str()));
	}
	return defaultValue;
}

Rgba8 NamedStrings::GetValue(std::string const& keyName, Rgba8 const& defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end()) 
	{
		Rgba8 color;
		color.SetFromText(iter->second.c_str());
		return color;
	}
	return defaultValue;
}

Vec2 NamedStrings::GetValue(std::string const& keyName, Vec2 const& defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end()) 
	{
		Vec2 vec;
		vec.SetFromText(iter->second.c_str());
		return vec;
	}
	return defaultValue;
}

IntVec2 NamedStrings::GetValue(std::string const& keyName, IntVec2 const& defaultValue) const
{
	auto iter = m_keyValuePairs.find(keyName);
	if (iter != m_keyValuePairs.end()) 
	{
		IntVec2 vec;
		vec.SetFromText(iter->second.c_str());
		return vec;
	}
	return defaultValue;
}

void NamedStrings::DebugPrintContents() const
{
	for (const auto& pair : m_keyValuePairs)
	{
		printf("%s = %s\n", pair.first.c_str(), pair.second.c_str());
	}
}
