#pragma once

#include "RangeParser.h"
#include <cstdint>

struct FileTimeInfo
{
    GPST start;
    GPST end;
    bool valid;
};

bool estimateOffset(
    uint64_t fileSize,
    const FileTimeInfo& info,
    const GPST& target,
    uint64_t& offset);
