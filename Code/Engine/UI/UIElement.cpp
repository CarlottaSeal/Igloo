﻿#include "UIElement.h"

#include "Engine/Core/VertexUtils.hpp"

UIElement::UIElement()
{
}

void UIElement::StartUp()
{
}

void UIElement::ShutDown()
{
}

void UIElement::Update()
{
}

void UIElement::Render() const
{
}

bool UIElement::AddChild(UIElement* child)
{
    if (child == nullptr)
        return false;
    m_children.push_back(child);
    return true;
}

bool UIElement::RemoveChild(UIElement* child)
{
    if (child == nullptr)
        return false;

    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end())
    {
        m_children.erase(it);
        return true;
    }
    return false;
}

void UIElement::SetParent(UIElement* parent)
{
    m_parent = parent;
}

// bool UIElement::IsOtherElementHavingFocus() const
// {
// }

void UIElement::SetEnabled(bool enabled)
{
    m_isEnabled = enabled;
}

void UIElement::SetFocused(bool focused)
{
    m_isFocused = focused;
}

bool UIElement::IsEnabled()
{
    return m_isEnabled;
}

bool UIElement::HasFocus()
{
    return m_isFocused;
}

void UIElement::InitializeVerts()
{
    AddVertsForAABB2D(m_verts, m_bound, m_originalColor); //TODO: UV
}
