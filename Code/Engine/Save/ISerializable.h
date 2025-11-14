#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/XmlUtils.hpp"

#pragma pack(push, 1) //general structure of header file
struct SaveFileHeader
{
    char m_fourCC[4];     
    uint32_t m_version;   
    
    SaveFileHeader() = default;
    SaveFileHeader(const char* cc, uint32_t ver) : m_version(ver)
    {
        m_fourCC[0] = cc[0];
        m_fourCC[1] = cc[1];
        m_fourCC[2] = cc[2];
        m_fourCC[3] = cc[3];
    }
    
    bool Validate(const char* expectedCC, uint32_t maxVersion) const
    {
        if (memcmp(m_fourCC, expectedCC, 4) != 0)
            return false;
        if (m_version > maxVersion)
            return false;
        return true;
    }
};
#pragma pack(pop)

class ISerializable
{
public:
    ISerializable() = default;
    virtual ~ISerializable() = default;

    virtual void SaveToBinary(std::vector<uint8_t>& buffer) const { UNUSED(buffer); }
    virtual bool LoadFromBinary(const std::vector<uint8_t>& buffer, size_t& offset) { UNUSED(buffer); UNUSED(offset); return false; }
    
    virtual void SaveToXML(tinyxml2::XMLElement* element) const { UNUSED(element); }
    virtual bool LoadFromXML(const tinyxml2::XMLElement* element) { UNUSED(element); return false; }
    
    virtual std::string SaveToString() const { return ""; }
    virtual bool LoadFromString(const std::string& str) { UNUSED(str); return false; }
    
    virtual std::map<std::string, std::string> SaveToKeyValues() const { return {}; }
    virtual bool LoadFromKeyValues(const std::map<std::string, std::string>& kvp) { UNUSED(kvp); return false; }
    
    virtual std::string GetSaveIdentifier() const = 0;

    void SaveToBinaryWithRLE(std::vector<uint8_t>& buffer, const uint8_t* data, size_t dataSize) const;
    bool LoadFromBinaryWithRLE(const std::vector<uint8_t>& buffer, size_t& offset, uint8_t* data, size_t expectedSize);
};
