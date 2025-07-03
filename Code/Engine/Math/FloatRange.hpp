#pragma once


struct FloatRange
{
public:
	float m_min;
	float m_max;

public:
	FloatRange();
	explicit FloatRange(float min, float max);
	~FloatRange();

	void SetFromText(char const* text);

	FloatRange& operator=(const FloatRange& other);
	bool operator==(const FloatRange& other) const;
	bool operator!=(const FloatRange& other) const;
	bool IsInRange(float value) const;
	bool IsOverlappingWith(const FloatRange& other) const;

	static const FloatRange ZERO;
	static const FloatRange ONE;
	static const FloatRange ZERO_TO_ONE;
};