﻿#define STB_IMAGE_IMPLEMENTATION // Exactly one .CPP (this Image.cpp) should #define this before #including stb_image.h
#include "ThirdParty/stb/stb_image.h"
#include "EngineCommon.hpp"
#include "Image.hpp"

Image::Image()
{
}

Image::Image(char const* imageFilePath)
    : m_imageFilePath(imageFilePath)
{
	//std::string fullPath = "Data/" + m_imageFilePath;
	std::string fullPath = m_imageFilePath;

	int width, height, channels;
	unsigned char* imageData = stbi_load(fullPath.c_str(), &width, &height, &channels, 4); // 加载图像为 RGBA
	if (channels != 3 && channels != 4)
	{
		ERROR_AND_DIE("Dies");
	}

	if (imageData == nullptr) 
	{
		ERROR_AND_DIE(Stringf("Failed to load image"));
	}

	m_dimensions = IntVec2(width, height);
	m_rgbaTexelsData.resize(width * height);

	/*for (int i = 0; i < width * height; ++i)
	{
		int offset = i * channels;
		m_rgbaTexelsData[i] = Rgba8(imageData[offset], imageData[offset + 1], imageData[offset + 2], (channels == 3 ? (unsigned char)255 : imageData[offset + 3]));
	}*/
	for (int i = 0; i < width * height; ++i)
	{
		int offset = i * 4;
		unsigned char r = imageData[offset + 0];
		unsigned char g = imageData[offset + 1];
		unsigned char b = imageData[offset + 2];
		unsigned char a = imageData[offset + 3];
		m_rgbaTexelsData[i] = Rgba8(r, g, b, a);
	}

	stbi_image_free(imageData); 
}

Image::Image(IntVec2 size, Rgba8 color)
	:m_dimensions(size)
{
	int totalPixels = size.x * size.y;

	m_rgbaTexelsData.resize(totalPixels);

	for (int i = 0; i < totalPixels; ++i)
	{
		m_rgbaTexelsData[i] = color;
	}
}

Image::~Image()
{
}

std::string const& Image::GetImageFilePath() const
{
    return m_imageFilePath;
}

IntVec2 Image::GetDimensions() const
{
    return m_dimensions;
}

Rgba8 Image::GetTexelColor(IntVec2 const& texelCoords) const  //get a goal coords' texel color
{
	if (texelCoords.x < 0 || texelCoords.x >= m_dimensions.x ||
		texelCoords.y < 0 || texelCoords.y >= m_dimensions.y) 
	{
		ERROR_AND_DIE(Stringf("Cannot get image texel color"));
	}

	int index = texelCoords.y * m_dimensions.x + texelCoords.x;
	return m_rgbaTexelsData[index];
}

void Image::SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor)
{
	if (texelCoords.x < 0 || texelCoords.x >= m_dimensions.x ||
		texelCoords.y < 0 || texelCoords.y >= m_dimensions.y) 
	{
		ERROR_AND_DIE(Stringf("Texel coordinates out of bounds"));
	}

	int index = texelCoords.y * m_dimensions.x + texelCoords.x;
	m_rgbaTexelsData[index] = newColor;
}

const void* Image::GetRawData() const
{
	return m_rgbaTexelsData.data();
}
