#pragma once
#include <vector>

#include "UIElement.h"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"

class Timer;

class Widget : public UIElement
{
public:
	Widget();
	Widget(Camera renderCamera, AABB2 bounds, std::string onClickEvent,
		Rgba8 originalColor = Rgba8::WHITE, std::string text = "", AABB2 textBox = AABB2::ZERO_TO_ONE);
	Widget(UIElement* myParent,Camera renderCamera, AABB2 bounds, std::string onClickEvent,
		Rgba8 originalColor = Rgba8::WHITE, std::string text = "", AABB2 textBox = AABB2::ZERO_TO_ONE);
	~Widget() override;

	void Update();
	void Render() const;
	
	void SetEnabled(bool enabled) override;
	Widget& SetBounds(const AABB2& localBounds);
	Widget& SetBounds(const Vec2& mins, const Vec2& maxs);
	Widget& SetText(const char* text, BitmapFont* font, const Rgba8& color = Rgba8::WHITE, float height = 0.f);
	//Widget& AddChild(Widget& widget, const Vec2& alignment = Vec2::ZERO);
	void Reset();
	void AddChild(UIElement& uiElement);

	void SetOnClickEvent(std::string onClickEvent);
	void OnClick();
	void UpdateIfClicked();

	void Build();

public:
	//General
	std::string m_name;
	//bool m_enabled = true;
	AABB2 m_bounds = AABB2::ZERO_TO_ONE;
	AABB2 m_textBox = AABB2::ZERO_TO_ONE;

	Camera m_renderCamera;
	std::string m_onClickEvent;
	Timer* m_popTextTimer;

	//std::vector<Widget> m_children;
	//Widget* m_parent = nullptr;

	//Background and border
	bool m_hasBackground = false;
	Rgba8 m_backgroundColor = Rgba8::BLACK;
	Rgba8 m_backgroundBorderColor = Rgba8::WHITE;
	float m_backgroundBorderWidth = 0.0f;
	std::vector<Vertex_PCU> m_backgroundVerts;

	//Textures
	bool m_hasTexture = false;
	Texture* m_texture = nullptr;
	Shader* m_shader = nullptr;
	Rgba8 m_textureColor = Rgba8::WHITE;
	std::vector<Vertex_PCU> m_textureVerts;

	//Text
	bool m_hasText = false;
	std::string m_text;
	BitmapFont* m_font = nullptr;
	Rgba8 m_textColor = Rgba8::WHITE;
	float m_textHeight = 1.0f;
	float m_textAspect = 1.0f;
	Vec2 m_textAlignment = Vec2::ZERO;
	std::vector<Vertex_PCU> m_textVerts;
};

