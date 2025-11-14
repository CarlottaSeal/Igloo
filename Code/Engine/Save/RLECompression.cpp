#include "RLECompression.h"

#include "Engine/Core/ErrorWarningAssert.hpp"

template <typename T>
std::vector<RLEEntry<T>> RLECompression::Compress(const T* data, size_t count)
{
    std::vector<RLEEntry<T>> result;
    if (!data || count == 0)
        return result;
    
    size_t i = 0;
    while (i < count)
    {
        T currentValue = data[i];
        uint8_t runLength = 1;
        
        // 计算连续相同值的长度（最多255）
        while (i + runLength < count && 
               runLength < 255 && 
               data[i + runLength] == currentValue)
        {
            runLength++;
        }
        RLEEntry<T> entry;
        entry.value = currentValue;
        entry.runLength = runLength;
        result.push_back(entry);
        i += runLength;
    }
    return result;
}

template <typename T>
bool RLECompression::Decompress(const std::vector<RLEEntry<T>>& rleData, T* output, size_t expectedCount)
{
    if (!output || expectedCount == 0)
        return false;
    
    size_t currentIndex = 0;
    
    for (const auto& entry : rleData)
    {
        if (currentIndex + entry.runLength > expectedCount)
        {
            DebuggerPrintf("RLE decompression would exceed expected count");
            return false;
        }
        for (uint8_t i = 0; i < entry.runLength; i++)
        {
            output[currentIndex++] = entry.value;
        }
    }
    
    if (currentIndex != expectedCount)
    {
        DebuggerPrintf("RLE decompression count mismatch");
        return false;
    }
    return true;
}

std::vector<uint8_t> RLECompression::CompressBytes(const uint8_t* data, size_t count)
{
    std::vector<uint8_t> result;
    if (!data || count == 0)
        return result;
    
    size_t i = 0;
    while (i < count)
    {
        uint8_t currentValue = data[i];
        uint8_t runLength = 1;
        
        while (i + runLength < count && 
               runLength < 255 && 
               data[i + runLength] == currentValue)
        {
            runLength++;
        }
        
        // store in keypaire: [m_value][length]
        result.push_back(currentValue);
        result.push_back(runLength);
        
        i += runLength;
    }
    return result;
}

bool RLECompression::DecompressBytes(const uint8_t* compressedData, size_t compressedSize, uint8_t* output,
    size_t expectedCount)
{
    if (!compressedData || !output || compressedSize == 0 || compressedSize % 2 != 0)
        return false;
    
    size_t currentIndex = 0;
    
    for (size_t i = 0; i < compressedSize; i += 2)
    {
        uint8_t value = compressedData[i];
        uint8_t runLength = compressedData[i + 1];
        
        if (currentIndex + runLength > expectedCount)
        {
            ERROR_RECOVERABLE("RLE decompression would exceed buffer");
            return false;
        }
        for (uint8_t j = 0; j < runLength; j++)
        {
            output[currentIndex++] = value;
        }
    }
    return currentIndex == expectedCount;
}
