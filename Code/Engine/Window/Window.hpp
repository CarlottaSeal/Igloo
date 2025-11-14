#pragma once

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/IntVec2.hpp"
#include <string>

class InputSystem;

struct WindowConfig
{
	float m_aspectRatio = (16.f / 9.f);
	InputSystem* m_inputSystem = nullptr;
	std::string m_windowTitle = "Protogame2D";
	bool m_isFullscreen = false;
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

	void ToggleFullscreen();

	WindowConfig const& GetConfig() const;
	void* GetDisplayContext() const;
	void* GetHwnd() const;
	IntVec2 GetClientDimensions() const;
	Vec2 GetNormalizedMouseUV() const;
	float GetCurrentAspectRatio() const;
	
	bool WindowHasFocus() const;

public:
	static Window* s_mainWindow;
	WindowConfig m_config;
	float m_currentAspectRatio = 16.f / 9.f;

private:
	//Private (internal) member functions will go here
	void RunMessagePump();
	//void CreateOSWindow(void* applicationInstanceHandle, float clientAspect);
	void CreateOSWindow();

private:
	//private(internal) data members
	void* m_displayContext = nullptr;
	void* m_windowHandle = nullptr;

	bool m_isCurrentlyFullscreen = false;
	void* m_windowedPlacementData = nullptr; 
};