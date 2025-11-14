#include "IntVec3.h"

#include <corecrt_math.h>

const IntVec3 IntVec3::ZERO = IntVec3(0,0,0);
const IntVec3 IntVec3::NEGATIVEONE = IntVec3(-1, -1, -1);

IntVec3::IntVec3(const IntVec3& copyFrom)
{
    x = copyFrom.x;
    y = copyFrom.y;
    z = copyFrom.z;
}

IntVec3::IntVec3(int initialX, int initialY, int initialZ)
{
    x = initialX;
    y = initialY;
    z = initialZ;
}

float IntVec3::GetLength() const
{
    return sqrtf((float)(x*x + y*y + z*z));
}

int IntVec3::GetTaxicabLength() const
{
    return abs(x) + abs(y) + abs(z);
}

int IntVec3::GetLengthSquared() const
{
    return x*x + y*y + z*z;
}

void IntVec3::operator=(const IntVec3& copyFrom)
{
    x = copyFrom.x;
    y = copyFrom.y;
    z = copyFrom.z;
}

IntVec3 IntVec3::operator+(const IntVec3& other) const
{
    return IntVec3(x + other.x, y + other.y, z + other.z);
}

void IntVec3::operator+=(const IntVec3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
}

IntVec3 IntVec3::operator-(const IntVec3& other) const
{
    return IntVec3(x - other.x, y - other.y, z - other.z);
}

bool IntVec3::operator==(const IntVec3& other) const
{
    return x == other.x && y == other.y && z == other.z;
}

bool IntVec3::operator!=(const IntVec3& other) const
{
    return x != other.x || y != other.y || z != other.z;
}
