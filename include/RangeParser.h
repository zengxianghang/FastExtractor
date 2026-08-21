#pragma once

#include <cstddef>

struct GPST
{
    int week;
    double sow;
};

// Legacy interface
bool parseRangeTime(const char* line, GPST& t);

// Zero-copy interface for buffer based scanning.
// The input buffer is not modified and does not require null termination.
bool parseRangeTimeFast(const char* data, size_t length, GPST& t);
