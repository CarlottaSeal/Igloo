#pragma once
#include <vector>

#include "Vec2.hpp"

struct Rgba8;
struct Vertex_PCU;

class CubicHermiteCurve2D
{
public:
    CubicHermiteCurve2D();
    CubicHermiteCurve2D(Vec2 startPos, Vec2 endPos, Vec2 startVelocity, Vec2 endVelocity);
    //~CubicHermiteCurve2D();
    Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
    float GetApproximateLength(int numSubdivisions = 64) const;
    Vec2 EvaluatedAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
    void AddVertsForWholeCurve(std::vector<Vertex_PCU> &verts, float thickness, Rgba8 color, int numSubdivisions = 64) const;

public:
    Vec2 m_startPos;
    Vec2 m_endPos;
    Vec2 m_startV;
    Vec2 m_endV;
};

class CubicBezierCurve2D
{
public:
    CubicBezierCurve2D(Vec2 startPos, Vec2 guidPos1, Vec2 guidPos2, Vec2 endPos);
    explicit CubicBezierCurve2D(CubicHermiteCurve2D const& fromHermite);
    //~CubicBezierCurve2D();
    Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
    float GetApproximateLength(int numSubdivisions = 64) const;
    Vec2 EvaluatedAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;

    void AddVertsForWholeCurve(std::vector<Vertex_PCU> &verts, float thickness, Rgba8 color, int numSubdivisions = 64) const;

public:
    Vec2 m_startPos;
    Vec2 m_guidPos1;
    Vec2 m_guidPos2;
    Vec2 m_endPos;
};

class CubicHermiteSpline
{
public:
    CubicHermiteSpline();
    CubicHermiteSpline(std::vector<Vec2> const &points, std::vector<Vec2> const &velocities);
    Vec2 EvaluateAtParametric(float parametricZeroToOne) const;
    float GetApproximateLength(int numSubdivisions = 64) const;
    Vec2 EvaluatedAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
    void AddVertsForWholeCurve(std::vector<Vertex_PCU> &verts, float thickness, Rgba8 color, int numSubdivisions = 64) const;
    
public:
    std::vector<Vec2> m_points;
    std::vector<Vec2> m_velocities;

private:
    std::vector<CubicHermiteCurve2D> m_curves;
};
