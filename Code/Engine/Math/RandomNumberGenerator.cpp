#include <stdlib.h>
#include "Engine/Math/RandomNumberGenerator.hpp"


int RandomNumberGenerator::RollRandomIntLessThan(int maxNotInclusive)
{
	return rand() % maxNotInclusive; //gives numbers from 0-9;

}

int RandomNumberGenerator::RollRandomIntInRange(int minInclusive, int maxInclusive)
{
	int range = (maxInclusive - minInclusive) + 1; //8-6=2,两个数字，应该再加一个；
	return minInclusive /*6*/ + RollRandomIntLessThan(range);
}

float RandomNumberGenerator::RollRandomFloatZeroToOne()
{
	return static_cast<float>( rand()) / static_cast<float>(RAND_MAX);
}

float RandomNumberGenerator::RollRandomFloatInRange(float minInclusive, float maxInclusive)
{
	float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

	return minInclusive + random * (maxInclusive - minInclusive);
}

bool RandomNumberGenerator::RollPercentChance(float probabilityOfReturningTrue)
{
	float percentChance = RollRandomFloatZeroToOne();
	/*if (percentChance < probabilityOfReturningTrue)
	{
		return true;
	}
	if (percentChance > probabilityOfReturningTrue)
	{
		return false;
	}*/
	return percentChance < probabilityOfReturningTrue;
}
