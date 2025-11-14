#pragma once
#include <cstdint>

class Renderer;
class Window;

struct GIConfig
{
	Renderer* m_renderer;
	Window* m_window;

	uint32_t m_primaryAtlasSize = 4096;
	uint32_t m_primaryTileSize = 64;
	uint32_t m_reflectionAtlasSize = 2048;
	uint32_t m_reflectionTileSize = 128;
	uint32_t m_giAtlasSize = 1024;
	uint32_t m_giTileSize = 256;
	bool m_enableTemporal = true;
	bool m_enableMultipleTypes = false;
};
