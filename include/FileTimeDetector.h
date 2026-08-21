#pragma once

#include "RangeParser.h"

struct FileTimeRange
{
    GPST start;
    GPST end;
    bool valid;
};

bool detectFileTimeRange(const char* filename, FileTimeRange& range);
