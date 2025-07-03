#include "Spline.h"

#include "MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"

CubicHermiteCurve2D::CubicHermiteCurve2D()
{
}

CubicHermiteCurve2D::CubicHermiteCurve2D(Vec2 startPos, Vec2 endPos, Vec2 startVelocity, Vec2 endVelocity)
    : m_startPos(startPos)
    , m_endPos(endPos)
    , m_startV(startVelocity)
    , m_endV(endVelocity)
{
}

Vec2 CubicHermiteCurve2D::EvaluateAtParametric(float parametricZeroToOne) const
{
    float t = parametricZeroToOne;
    float t2 = t * t;
    float t3 = t2 * t;

    float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h10 = t3 - 2.0f * t2 + t;
    float h01 = -2.0f * t3 + 3.0f * t2;
    float h11 = t3 - t2;

    return h00 * m_startPos +
           h10 * m_startV +
           h01 * m_endPos +
           h11 * m_endV;
}

float CubicHermiteCurve2D::GetApproximateLength(int numSubdivisions) const
{
    float length = 0.0f;
    Vec2 prev = EvaluateAtParametric(0.0f);

    for (int i = 1; i <= numSubdivisions; ++i)
    {
        float t = static_cast<float>(i) / numSubdivisions;
        Vec2 curr = EvaluateAtParametric(t);
        length += (curr - prev).GetLength();
        prev = curr;
    }

    return length;
}

Vec2 CubicHermiteCurve2D::EvaluatedAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
    float traveled = 0.0f;
    Vec2 prevPoint = EvaluateAtParametric(0.0f);

    for (int i = 1; i <= numSubdivisions; ++i)
    {
        float t = static_cast<float>(i) / numSubdivisions;
        Vec2 currentPoint = EvaluateAtParametric(t);
        float segmentLength = (currentPoint - prevPoint).GetLength();

        if (traveled + segmentLength >= distanceAlongCurve)
        {
            float remaining = distanceAlongCurve - traveled;
            float alpha = remaining / segmentLength;
            return prevPoint + (currentPoint - prevPoint) * alpha;
        }

        traveled += segmentLength;
        prevPoint = currentPoint;
    }

    return m_endPos;
}

void CubicHermiteCurve2D::AddVertsForWholeCurve(std::vector<Vertex_PCU>& verts, float thickness, Rgba8 color,
    int numSubdivisions) const
{
    Vec2 prevPoint = EvaluateAtParametric(0.0f);

    for (int i = 1; i <= numSubdivisions; ++i)
    {
        float t = static_cast<float>(i) / numSubdivisions;
        Vec2 currentPoint = EvaluateAtParametric(t);

        AddVertsForLineSegment2D(verts, prevPoint, currentPoint, thickness, color);
        prevPoint = currentPoint;
    }
}

CubicBezierCurve2D::CubicBezierCurve2D(Vec2 startPos, Vec2 guidPos1, Vec2 guidPos2, Vec2 endPos)
    : m_startPos(startPos)
    , m_endPos(endPos)
    , m_guidPos1(guidPos1)
    , m_guidPos2(guidPos2)
{
}

CubicBezierCurve2D::CubicBezierCurve2D(CubicHermiteCurve2D const& fromHermite)
{
    m_startPos = fromHermite.m_startPos;
    m_endPos = fromHermite.m_endPos;
    m_guidPos1 = m_startPos + (fromHermite.m_startV / 3.0f);
    m_guidPos2 = m_endPos - (fromHermite.m_endV / 3.0f);
}

Vec2 CubicBezierCurve2D::EvaluateAtParametric(float parametricZeroToOne) const
{
    float oneMinusT = 1.f - parametricZeroToOne;
    return
        oneMinusT * oneMinusT * oneMinusT * m_startPos +
        3.f * oneMinusT * oneMinusT * parametricZeroToOne * m_guidPos1 +
        3.f * oneMinusT * parametricZeroToOne * parametricZeroToOne * m_guidPos2 +
        parametricZeroToOne * parametricZeroToOne * parametricZeroToOne * m_endPos;
}

float CubicBezierCurve2D::GetApproximateLength(int numSubdivisions) const
{
    float length = 0.0f;
    Vec2 prevPoint = EvaluateAtParametric(0.0f);

    for (int i = 1; i <= numSubdivisions; ++i)
    {
        float t = static_cast<float>(i) / numSubdivisions;
        Vec2 currentPoint = EvaluateAtParametric(t);
        length += (currentPoint - prevPoint).GetLength();
        prevPoint = currentPoint;
    }

    return length;
}

Vec2 CubicBezierCurve2D::EvaluatedAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
    float traveled = 0.0f;
    Vec2 prevPoint = EvaluateAtParametric(0.0f);

    for (int i = 1; i <= numSubdivisions; ++i)
        {
        float t = static_cast<float>(i) / numSubdivisions;
        Vec2 currentPoint = EvaluateAtParametric(t);
        float segmentLength = (currentPoint - prevPoint).GetLength();

        if (traveled + segmentLength >= distanceAlongCurve)
            {
            float remaining = distanceAlongCurve - traveled;
            float alpha = remaining / segmentLength;
            return prevPoint + (currentPoint - prevPoint) * alpha;
        }

        traveled += segmentLength;
        prevPoint = currentPoint;
    }

    return m_endPos;
}

void CubicBezierCurve2D::AddVertsForWholeCurve(std::vector<Vertex_PCU>& verts, float thickness, Rgba8 color,
    int numSubdivisions) const
{
    Vec2 prevPoint = EvaluateAtParametric(0.0f);

    for (int i = 1; i <= numSubdivisions; ++i)
    {
        float t = static_cast<float>(i) / numSubdivisions;
        Vec2 currentPoint = EvaluateAtParametric(t);

        AddVertsForLineSegment2D(verts, prevPoint, currentPoint, thickness, color);
        prevPoint = currentPoint;
    }
}

CubicHermiteSpline::CubicHermiteSpline()
{
}

CubicHermiteSpline::CubicHermiteSpline(std::vector<Vec2> const& points, std::vector<Vec2> const& velocities)
    : m_points(points), m_velocities(velocities)
{
    if (points.size() < 2 || points.size() != velocities.size())
    {
        return;
    }
    for (int i = 0; i < m_points.size()-1; ++i)
    {
        CubicHermiteCurve2D curve(m_points[i], m_points[i+1],
            m_velocities[i], m_velocities[i+1]);
        m_curves.push_back(curve);
    }
}

Vec2 CubicHermiteSpline::EvaluateAtParametric(float parametricZeroToOne) const
{
    //if 2.67,在第二条curve的0.67位置
    int numCurves = static_cast<int>(m_points.size()) - 1;
    if (numCurves <= 0)
        return Vec2();

    parametricZeroToOne = GetClamped(parametricZeroToOne, 0.0f, static_cast<float>(numCurves));
    
    int curveIndex = RoundDownToInt(parametricZeroToOne);
    float curveT = parametricZeroToOne - (float)curveIndex;

    if (curveIndex >= numCurves)
    {
        curveIndex = numCurves - 1;
        curveT = 1.0f;
    }
    
    Vec2 start = m_points[curveIndex];
    Vec2 end = m_points[curveIndex + 1];
    Vec2 startTan = m_velocities[curveIndex];
    Vec2 endTan = m_velocities[curveIndex + 1];

    CubicHermiteCurve2D curve(start, end, startTan, endTan);
    return curve.EvaluateAtParametric(curveT);
}

float CubicHermiteSpline::GetApproximateLength(int numSubdivisions) const
{
    float sum = 0.f;
    for (int curveIndex = 0; curveIndex < (int)m_curves.size(); curveIndex++)
    {
        sum += m_curves[curveIndex].GetApproximateLength(numSubdivisions);
    }
    return sum;
}

Vec2 CubicHermiteSpline::EvaluatedAtApproximateDistance(float distanceAlongCurve, int numSubdivisions) const
{
    float start = 0.f;
    float end = 0.f;
    for (int curveIndex = 0; curveIndex < (int)m_curves.size(); curveIndex++)
    {
        start = end;
        end += m_curves[curveIndex].GetApproximateLength(numSubdivisions);
        if (start <= distanceAlongCurve && distanceAlongCurve <= end)
        {
            return m_curves[curveIndex].EvaluatedAtApproximateDistance(distanceAlongCurve - start, numSubdivisions);
        }
    }
    return m_curves[(int)m_curves.size() - 1].m_endPos;
}

void CubicHermiteSpline::AddVertsForWholeCurve(std::vector<Vertex_PCU>& verts, float thickness, Rgba8 color,
    int numSubdivisions) const
{
    for (int curveIndex = 0; curveIndex < m_curves.size(); curveIndex++)
    {
        m_curves[curveIndex].AddVertsForWholeCurve(verts, thickness, color, numSubdivisions);
    }
}