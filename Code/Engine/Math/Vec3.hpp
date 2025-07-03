#pragma once

struct Vec2;
struct EulerAngles;

struct Vec3
{
public:
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

public:
    // Construction/Destruction
    ~Vec3() {};
    Vec3() {};
    explicit Vec3(float initialX, float initialY, float initialZ);
    Vec3(Vec3 const& copyFrom);

    //Acessors (const methods)
    float GetLength() const;
    float GetLengthXY() const;
    float GetLengthSquared() const;
    float GetLengthXYSquared() const;
    float GetAngleAboutZRadians() const;
    float GetAngleAboutZDegrees() const;
    Vec3 const GetRotatedAboutZRadians(float deltaRadians) const;
    Vec3 const GetRotatedAboutZDegrees(float deltaDegrees) const;
    Vec3 const GetClamped(float maxLength) const;
    Vec3 const GetNormalized() const;
    Vec2 const GetXY() const;
    EulerAngles GetEulerAnglesFromVec3() const;

    bool IsNearlyZero(float epsilon = 1e-6f) const;

    void SetFromText(char const* text);

	const static Vec3 MakeFromPolarRadians(float latitudeRadians, float longitudeRadians, float length);
	const static Vec3 MakeFromPolarDegrees(float latitudeDegrees, float longitudeDegrees, float length);

    // 运算符重载
    bool       operator==(Vec3 const& compare) const;
    bool       operator!=(Vec3 const& compare) const;
    Vec3 const operator+(Vec3 const& vecToAdd) const;
    Vec3 const operator-(Vec3 const& vecToSubtract) const;
    Vec3 const operator-() const;
    Vec3 const operator*(float uniformScale) const;
    Vec3 const operator*(Vec3 const& vecToMultiply) const;
    Vec3 const operator/(float inverseScale) const;


    void operator+=(Vec3 const& vecToAdd);
    void operator-=(Vec3 const& vecToSubtract);
    void operator*=(float uniformScale);
    void operator/=(float uniformDivisor);
    void operator=(Vec3 const& copyFrom);
    
    friend Vec3 const operator*(float uniformScale, Vec3 const& vecToScale);
};


