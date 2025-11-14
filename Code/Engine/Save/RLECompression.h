#pragma once
#include <vector>
#include <cstdint>

template<typename T>
struct RLEEntry
{
    T m_value;
    uint8_t m_runLength;  // 1-255
};

class RLECompression
{
public:
    template<typename T>
    static std::vector<RLEEntry<T>> Compress(const T* data, size_t count);
    
    template<typename T>
    static bool Decompress(const std::vector<RLEEntry<T>>& rleData, T* output, size_t expectedCount);
    
    // Byets
    static std::vector<uint8_t> CompressBytes(const uint8_t* data, size_t count);
    static bool DecompressBytes(const uint8_t* compressedData, size_t compressedSize, 
                                uint8_t* output, size_t expectedCount);
};