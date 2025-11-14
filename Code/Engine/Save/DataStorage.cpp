#include "DataStorage.h"

void DataStorage::Clear()
{
    m_binaryData.clear();
    m_textData.clear();
    m_xmlDoc.reset();
    m_keyValuePairs.clear();
}
