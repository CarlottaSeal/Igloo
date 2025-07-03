#pragma once

struct IntRange
{
public:
	int m_min;
	int m_max;

public:
	IntRange();

	explicit IntRange(int min, int max);

	~IntRange();

	IntRange& operator=(const IntRange& other);

	bool operator==(const IntRange& other) const;

	bool operator!=(const IntRange& other) const;

	bool IsInRange(int value) const;

	bool IsOverlappingWith(const IntRange& other) const;

	static const IntRange ZERO;
	static const IntRange ONE;
	static const IntRange ZERO_TO_ONE;
};