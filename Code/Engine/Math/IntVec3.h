#pragma once

#include <functional>

struct IntVec3
{
public: 
    int x = 0;
    int y = 0;
    int z = 0;

public:
    IntVec3() {};
    ~IntVec3() {};
    IntVec3(const IntVec3& copyFrom);
    explicit IntVec3(int initialX, int initialY, int initialZ);

    static const IntVec3 ZERO;
    static const IntVec3 NEGATIVEONE;

    //Accessors(const methods)
    float GetLength() const;
    int GetTaxicabLength() const;
    int GetLengthSquared() const;
    float GetOrientationRadians() const;
    float GetOrientationDegrees() const;
    IntVec3 const GetRotated90Degrees() const;
    IntVec3 const GetRotatedMinus90Degrees() const;

    //Mutators (non-const methods)
    void Rotate90Degrees();
    void RotateMinus90Degrees();

    void SetFromText(char const* text);

    //Operators(self-mutating/non-const)
    void operator = (const IntVec3 & copyFrom);

    IntVec3 operator+(const IntVec3& other) const;
    void operator+=(const IntVec3& other);

    IntVec3 operator-(const IntVec3& other) const;

    bool operator == (const IntVec3& other) const;
    bool operator != (const IntVec3& other) const;
};

namespace std
{
    template<> struct hash<IntVec3>
    {
        size_t operator()(const IntVec3& v) const noexcept
        {
            size_t h = std::hash<int>{}(v.x);
            h ^= std::hash<int>{}(v.y) + 0x9e3779b97f4a7c15ull + (h<<6) + (h>>2);
            h ^= std::hash<int>{}(v.z) + 0x9e3779b97f4a7c15ull + (h<<6) + (h>>2);
            return h;
        }
    };
}