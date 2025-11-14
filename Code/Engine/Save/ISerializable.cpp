#include "ISerializable.h"

#include "RLECompression.h"

void ISerializable::SaveToBinaryWithRLE(std::vector<uint8_t>& buffer, const uint8_t* data, size_t dataSize) const
{
    std::vector<uint8_t> compressed = RLECompression::CompressBytes(data, dataSize);
    
    uint32_t compressedSize = (uint32_t)compressed.size();
    size_t oldSize = buffer.size();
    buffer.resize(oldSize + sizeof(uint32_t) + compressedSize);
    
    memcpy(buffer.data() + oldSize, &compressedSize, sizeof(uint32_t));
    memcpy(buffer.data() + oldSize + sizeof(uint32_t), compressed.data(), compressedSize);

}

bool ISerializable::LoadFromBinaryWithRLE(const std::vector<uint8_t>& buffer, size_t& offset, uint8_t* data,
    size_t expectedSize)
{
    if (offset + sizeof(uint32_t) > buffer.size())
        return false;
    
    uint32_t compressedSize;
    memcpy(&compressedSize, buffer.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    if (offset + compressedSize > buffer.size())
        return false;
    
    bool success = RLECompression::DecompressBytes(buffer.data() + offset, compressedSize, 
                                                   data, expectedSize);
    offset += compressedSize;
    
    return success;
}
