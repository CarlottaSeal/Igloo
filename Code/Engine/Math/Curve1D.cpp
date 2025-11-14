#include "Curve1D.h"

#include "MathUtils.hpp"

LinearCurve1D::LinearCurve1D(float start, float end, float startT, float endT)
    : m_startT(startT)
    , m_endT(endT)
    , m_start(start)
    , m_end(end)
{
}

float LinearCurve1D::Evaluate(float t) const
{
    if (t<m_startT)
    {
        return m_start;
    }
    if (t>=m_endT)
    {
        return m_end;
    }
    float fraction = (t - m_startT) / (m_endT - m_startT);
    
    return Interpolate(m_start, m_end, fraction);
}

PiecewiseCurve1D::PiecewiseCurve1D()
{
}

float PiecewiseCurve1D::Evaluate(float t) const
{
    if (m_keyPoints.size() == 0)
        return 0.0f;

    for (int i = 0; i < m_keyPoints.size(); i++)
    {
        if (i == m_keyPoints.size() - 1)
        {
            float startT = m_keyPoints[i].m_t;
            float localT = (t - startT);

            return m_keyPoints[i].m_curve->Evaluate(localT);
        }

        float startT = m_keyPoints[i].m_t;
        float endT = m_keyPoints[i + 1].m_t;
        if ( t>= startT && t< endT )
        {
            float fraction = (t - startT) / (endT - startT);
            return m_keyPoints[i].m_curve->Evaluate(fraction);
        }
    }
        return m_keyPoints[0].m_curve->Evaluate(t);
}

void PiecewiseCurve1D::AddKeyPoint(float t, Curve1D* curve)
{
    m_keyPoints.push_back(KeyPoint(t, curve));
}

PiecewiseCurve1D::~PiecewiseCurve1D()
{
    for (auto pair: m_keyPoints)
    {
        delete pair.m_curve;
        pair.m_curve = nullptr;
    }
}
