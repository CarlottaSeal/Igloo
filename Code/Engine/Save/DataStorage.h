#pragma once
#include "Iserializable.h"
#include "SaveCommon.h"

class DataStorage
{
public:
    SaveFormat m_format = SaveFormat::BINARY;
    
    std::vector<uint8_t> m_binaryData;
    std::string m_textData;
    std::unique_ptr<XmlDocument> m_xmlDoc;
    std::map<std::string, std::string> m_keyValuePairs;
    
    void Clear();
};
