#pragma once

class RandomNumberGenerator
{

public:
	int RollRandomIntLessThan(int maxNotInclusive);
	int RollRandomIntInRange(int minInclusive, int maxInclusive);
	float RollRandomFloatZeroToOne();
	float RollRandomFloatInRange(float minInclusive, float maxInclusive);
	bool RollPercentChance(float probabilityOfReturningTrue);

private:
//	unsigned int m_seed = 0; 
//	unsigned int m_position = 0;
	// We will use these later on...
	// ...when we replace rand() with noise
};