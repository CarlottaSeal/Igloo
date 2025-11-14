#pragma once
#include <functional>   

struct IntVec2
{
public: 
	int x = 0;
	int y = 0;

public:
	IntVec2() {};
	~IntVec2() {};
	IntVec2(const IntVec2& copyFrom);
	explicit IntVec2(int initialX, int initialY);

	static const IntVec2 ZERO;
	static const IntVec2 NEGATIVEONE;

	//Accessors(const methods)
	float GetLength() const;
	int GetTaxicabLength() const;
	int GetLengthSquared() const;
	float GetOrientationRadians() const;
	float GetOrientationDegrees() const;
	IntVec2 const GetRotated90Degrees() const;
	IntVec2 const GetRotatedMinus90Degrees() const;

	//Mutators (non-const methods)
	void Rotate90Degrees();
	void RotateMinus90Degrees();

	void SetFromText(char const* text);

	//Operators(self-mutating/non-const)
	void operator = (const IntVec2 & copyFrom);

	IntVec2 operator+(const IntVec2& other) const;
	void operator+=(const IntVec2& other);

	IntVec2 operator-(const IntVec2& other) const;

	bool operator == (const IntVec2& other) const;
	bool operator != (const IntVec2& other) const;

	bool operator<(const IntVec2& rhs) const;
};

namespace std
{
	template<>
	struct hash<IntVec2>
	{
		size_t operator()(const IntVec2& v) const
		{
			return hash<int>()(v.x) ^ (hash<int>()(v.y) << 1);
		}
	};
}