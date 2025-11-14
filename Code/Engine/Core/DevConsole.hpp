#pragma once
#include <mutex>

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/EventSystem.hpp"
#include <string>
#include <vector>

struct AABB2;
class Renderer;
//class DX12Renderer;
class BitmapFont;
class Timer;
class Camera;

enum DevConsoleMode
{
	OPEN_FULL,
	OPEN_PARTIAL,
	HIDDEN,
	NUM
};

struct DevConsoleConfig
{
	Renderer* m_defaultRenderer;
	Camera* m_camera = nullptr;
	std::string m_defaultFontName = "SquirrelFixedFont";
	float m_fontAspect = 0.7f;
	int m_linesOnScreen = 40;
	int m_masCommandHistory = 128;
	bool m_startOpen = false;
//
//#ifdef ENGINE_DX12_RENDERER
//	DX12Renderer* m_defaultDX12Renderer;
//#endif
};

struct DevConsoleLine
{
	Rgba8 m_color;
	std::string m_text;
	//int m_frameNumberPrinted;
	//double m_timePrinted;
};

class DevConsole
{
public:
	DevConsole(DevConsoleConfig const& config);
	~DevConsole();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void Execute(std::string const& consoleCommandText);
	void AddLine(Rgba8 const& color, std::string const& text);
	void Render(AABB2 const& bounds, Renderer* rendererOverride = nullptr) const;

	DevConsoleMode GetMode() const;
	void SetMode(DevConsoleMode mode);
	void ToggleMode(DevConsoleMode mode);

	static bool Command_Test(EventArgs& args);

	static const Rgba8 ERRORLINE;
	static const Rgba8 WARNING;
	static const Rgba8 INFO_MAJOR;
	static const Rgba8 INFO_MINOR;
	static const Rgba8 INFO_SHADOW;
	static const Rgba8 INPUT_TEXT;
	static const Rgba8 INPUT_INSERTION_POINT;

	static bool OnKeyPressed(EventArgs& args);
	static bool OnCharInput(EventArgs& args);
	static bool Command_Clear(EventArgs& args);
	static bool Command_Help(EventArgs& args);
	static bool Command_Warning(EventArgs& args);

protected:
	void Render_Openfull(AABB2 const& bounds, Renderer& renderer, BitmapFont& font, float fontAspect = 1.f) const;

protected:
	DevConsoleConfig m_config;
	mutable std::recursive_mutex m_mutex;
	
	DevConsoleMode m_mode = DevConsoleMode::HIDDEN;
	std::vector<DevConsoleLine> m_lines;
	int m_frameNumber = 0;

	std::string m_fontPath;

	std::string m_inputText; //Our current line of input text
	int m_insertionPointPos = 0; //Index of the insertion point in our current input text
	bool m_insertionPointVisible = true; //True if our insertion point is currently in the visible phase of blinking.
	Timer* m_insertionPointBlinkTimer; //Timer for controlling insertion point visibility
	std::vector<std::string> m_commandHistory; //History of all commands executed.
	int m_historyIndex = -1; //Our current index in our history of commands as we are scrolling.
};