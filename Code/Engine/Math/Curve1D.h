#pragma once
#include <vector>

class Curve1D
{
public:
    virtual ~Curve1D()  = default;

    virtual float Evaluate(float t) const = 0;    
};

class LinearCurve1D : public Curve1D
{
    public:
    LinearCurve1D( float start, float end, float startT = 0.f, float endT = 1.f);

    float Evaluate(float t) const override;

public:
    float m_startT;
    float m_endT;
    float m_start;
    float m_end;
};

class PiecewiseCurve1D : public Curve1D
{
public:
    struct KeyPoint
    {
        float m_t;
        Curve1D* m_curve;

        KeyPoint(float t, Curve1D* curve)
        {
            m_t = t;
            m_curve = curve;
        }
    };

    PiecewiseCurve1D();
    ~PiecewiseCurve1D() override;

    void AddKeyPoint(float t, Curve1D* curve);
    
    virtual float Evaluate(float t) const override;

private:
    std::vector<KeyPoint> m_keyPoints;
};