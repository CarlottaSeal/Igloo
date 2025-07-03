#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <string>

class InputSystem;

struct WindowConfig
{
	float m_aspectRatio = (16.f / 9.f);
	InputSystem* m_inputSystem = nullptr;
	std::string m_windowTitle = "Protogame2D";
};

class Window
{
public:
	Window(WindowConfig const& config);
	~Window();

	void Startup();
	void BeginFrame();
	void EndFrame();
	void Shutdown();

	WindowConfig const& GetConfig() const;
	void* GetDisplayContext() const;
	void* GetHwnd() const;
	IntVec2 GetClientDimensions() const;

	Vec2 GetNormalizedMouseUV() const;

	bool WindowHasFocus() const;

public:
	static Window* s_mainWindow;
	WindowConfig m_config;

private:
	//Private (internal) member functions will go here
	void RunMessagePump();
	//void CreateOSWindow(void* applicationInstanceHandle, float clientAspect);
	void CreateOSWindow();

private:
	//private(internal) data members
	//WindowConfig m_config;
	void* m_displayContext = nullptr;
	void* m_windowHandle = nullptr;
};