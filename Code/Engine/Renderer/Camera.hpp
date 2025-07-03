﻿#pragma once
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/EulerAngles.hpp"

class Camera 
{
public:
    enum CameraMode
    {
        eMode_OrthoGraphic,
        eMode_Perspective,

        eMode_Count
    };

    void SetOrthographicView(const Vec2& bottomLeft, const Vec2& topRight, float zNear = 0.f, float zFar = 1.f);
    void SetPerspectiveView(float aspect, float fov, float near, float far);

    void SetPositionAndOrientation(const Vec3& position, const EulerAngles& orientation);
    void SetPosition(const Vec3& position);
    Vec3 GetPosition() const;
    void SetOrientation(const EulerAngles& orientation);
    EulerAngles GetOrientation() const;

    Mat44 GetCameraToWorldTransform() const;
    Mat44 GetWorldToCameraTransform() const;

    void SetCameraToRenderTransform(const Mat44& m);
    Mat44 GetCameraToRenderTransform() const;

    Mat44 GetRenderToClipTransform() const;

    Vec2 GetOrthographicBottomLeft() const;
    Vec2 GetOrthographicTopRight() const;
    AABB2 GetOrthographicBounds() const;
    void Translate2D(const Vec2& translation2D);

    Mat44 GetOrthographicMatrix() const;
    Mat44 GetPerspectiveMatrix() const;
    Mat44 GetProjectionMatrix() const;

    void SetCameraMode(CameraMode mode);

    AABB2 MakePlayerViewport(int numOfPlayers, int playerIndex = 0) const;

protected:
    //Vec2 m_bottomLeft;
    //Vec2 m_topRight;

    CameraMode m_mode = eMode_OrthoGraphic;
    
    Vec3 m_position;
    EulerAngles m_orientation;

    Vec2 m_orthographicBottomLeft;
    Vec2 m_orthographicTopRight;
    float m_orthographicNear;
    float m_orthographicFar;

    float m_perspectiveAspect;
    float m_perspectiveFOV;
    float m_perspectiveNear;
    float m_perspectiveFar;

    Mat44 m_cameraToRenderTransform = Mat44();
};