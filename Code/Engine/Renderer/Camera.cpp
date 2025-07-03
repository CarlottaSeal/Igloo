#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/Mat44.hpp"


void Camera::SetOrthographicView(const Vec2& bottomLeft, const Vec2& topRight, float zNear, float zFar)
{
    m_orthographicBottomLeft = bottomLeft;
    m_orthographicTopRight = topRight;
	m_orthographicNear = zNear;
	m_orthographicFar = zFar;
}

void Camera::SetPerspectiveView(float aspect, float fov, float near, float far)
{
    m_perspectiveAspect = aspect;
    m_perspectiveFOV = fov;
    m_perspectiveNear = near;
    m_perspectiveFar = far;
}

void Camera::SetPositionAndOrientation(const Vec3& position, const EulerAngles& orientation)
{
    m_position = position;
    m_orientation = orientation;
}

void Camera::SetPosition(const Vec3& position)
{
    m_position = position;
}

Vec3 Camera::GetPosition() const
{
    return m_position;
}

void Camera::SetOrientation(const EulerAngles& orientation)
{
    m_orientation = orientation;
}

EulerAngles Camera::GetOrientation() const
{
    return m_orientation;
}

Mat44 Camera::GetCameraToWorldTransform() const
{
    Mat44 camToWorld = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
    camToWorld.SetTranslation3D(m_position);
    return camToWorld;
}

Mat44 Camera::GetWorldToCameraTransform() const
{
    Mat44 camToWorld = GetCameraToWorldTransform();
    return camToWorld.GetOrthonormalInverse();
}

void Camera::SetCameraToRenderTransform(const Mat44& m)
{
    m_cameraToRenderTransform = m;
}

Mat44 Camera::GetCameraToRenderTransform() const
{
    return m_cameraToRenderTransform;
}

Mat44 Camera::GetRenderToClipTransform() const
{
    return GetProjectionMatrix();
}

Vec2 Camera::GetOrthographicBottomLeft() const
{
    return m_orthographicBottomLeft;
}

Vec2 Camera::GetOrthographicTopRight() const
{
    return m_orthographicTopRight;
}

AABB2 Camera::GetOrthographicBounds() const
{
    return AABB2(m_orthographicBottomLeft, m_orthographicTopRight);
}

void Camera::Translate2D(const Vec2& translation2D)
{
    m_orthographicBottomLeft += translation2D;
    m_orthographicTopRight += translation2D;
}

Mat44 Camera::GetOrthographicMatrix() const
{
    Mat44 ortho = Mat44::MakeOrthoProjection(m_orthographicBottomLeft.x, m_orthographicTopRight.x,
    m_orthographicBottomLeft.y, m_orthographicTopRight.y, m_orthographicNear, m_orthographicFar);
    return ortho;
}

Mat44 Camera::GetPerspectiveMatrix() const
{
    Mat44 persp = Mat44::MakePerspectiveProjection(m_perspectiveFOV, m_perspectiveAspect, m_perspectiveNear, m_perspectiveFar);
    return persp;
}

Mat44 Camera::GetProjectionMatrix() const
{
    Mat44 rawMat;
    if (m_mode == eMode_OrthoGraphic)
    {
        rawMat = GetOrthographicMatrix();
    }
    if (m_mode == eMode_Perspective)
    {
        rawMat = GetPerspectiveMatrix();
    }   
    //rawMat.Append(GetWorldToCameraTransform()); //if comment, append this mat in shader
    //rawMat.Append(m_cameraToRenderTransform); //if comment, append this mat in shader
    return rawMat;
}

void Camera::SetCameraMode(CameraMode mode)
{
    m_mode = mode;
}

AABB2 Camera::MakePlayerViewport(int numOfPlayers, int playerIndex) const
{
    if (numOfPlayers == 2)
    {
        if (playerIndex == 0)
        {
            return AABB2(m_orthographicBottomLeft + Vec2(0.f, (m_orthographicTopRight - m_orthographicBottomLeft).y/2.f),
                m_orthographicTopRight);
        }
        if (playerIndex == 1)
        {
            return AABB2(m_orthographicBottomLeft ,
                m_orthographicTopRight - Vec2(0.f, (m_orthographicTopRight - m_orthographicBottomLeft).y/2.f));
        }
    }
	if (numOfPlayers == 3)
	{
		if (playerIndex == 0)
		{
			return AABB2(m_orthographicBottomLeft + Vec2(0.f, (m_orthographicTopRight - m_orthographicBottomLeft).y / 2.f),
				m_orthographicTopRight);
		}
		if (playerIndex == 1)
		{
			return AABB2(m_orthographicBottomLeft,
				(m_orthographicTopRight - m_orthographicBottomLeft) / 2.f);
		}
		if (playerIndex == 2)
		{
			return AABB2(Vec2((m_orthographicTopRight - m_orthographicBottomLeft).x / 2.f, 0.f),
				Vec2(m_orthographicTopRight.x, (m_orthographicTopRight - m_orthographicBottomLeft).y / 2.f));
		}
	}
    if (numOfPlayers == 4)
    {
        if (playerIndex == 0)
        {
            return AABB2(m_orthographicBottomLeft + Vec2(0.f, (m_orthographicTopRight - m_orthographicBottomLeft).y / 2.f),
                m_orthographicTopRight - Vec2((m_orthographicTopRight - m_orthographicBottomLeft).x / 2.f, 0.f));
        }
        if (playerIndex == 1)
        {
            return AABB2(Vec2((m_orthographicTopRight - m_orthographicBottomLeft).x / 2.f, (m_orthographicTopRight - m_orthographicBottomLeft).y / 2.f),
                m_orthographicTopRight);
        }
        if (playerIndex == 2)
        {
            return AABB2(m_orthographicBottomLeft,
                (m_orthographicTopRight - m_orthographicBottomLeft) / 2.f);
        }
        if (playerIndex == 3)
        {
            return AABB2(Vec2((m_orthographicTopRight - m_orthographicBottomLeft).x / 2.f, 0.f),
                Vec2(m_orthographicTopRight.x, (m_orthographicTopRight - m_orthographicBottomLeft).y / 2.f));
        }
    }
    else
        return AABB2();

    return AABB2::ZERO_TO_ONE;
}
