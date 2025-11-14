#pragma once

class RandomNumberGenerator
{

public:
	int RollRandomIntLessThan(int maxNotInclusive);
	int RollRandomIntInRange(int minInclusive, int maxInclusive);
	float RollRandomFloatZeroToOne();
	float RollRandomFloatInRange(float minInclusive, float maxInclusive);
	bool RollPercentChance(float probabilityOfReturningTrue);
};