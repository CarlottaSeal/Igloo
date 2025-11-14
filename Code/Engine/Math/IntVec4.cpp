#include "IntVec4.h"

const IntVec4 IntVec4::ZERO = IntVec4(0, 0, 0, 0);
const IntVec4 IntVec4::NEGATIVEONE = IntVec4(-1, -1, -1, -1);

IntVec4::IntVec4(const IntVec4& copyFrom)
{
    x = copyFrom.x;
    y = copyFrom.y;
    z = copyFrom.z;
    w = copyFrom.w;
}

IntVec4::IntVec4(int initialX, int initialY, int initialZ, int initialW)
{
    x = initialX;
    y = initialY;
    z = initialZ;
    w = initialW;
}
