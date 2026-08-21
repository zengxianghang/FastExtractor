#pragma once

#include "RangeParser.h"

struct FileTimeRange
{
    GPST start;
    GPST end;
};

bool detectTailRange(const char* filename, FileTimeRange& range);
