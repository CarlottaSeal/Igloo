#pragma once

struct IntVec4
{
public: 
    int x = 0;
    int y = 0;
    int z = 0;
    int w = 0;

public:
    IntVec4() {};
    ~IntVec4() {};
    IntVec4(const IntVec4& copyFrom);
    explicit IntVec4(int initialX, int initialY, int initialZ, int initialW);

    static const IntVec4 ZERO;
    static const IntVec4 NEGATIVEONE;
};
