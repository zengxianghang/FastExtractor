#pragma once

struct GPST
{
    int week;
    double sow;
};

bool parseRangeTime(const char* line, GPST& t);
