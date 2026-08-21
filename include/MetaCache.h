#pragma once

#include "RangeParser.h"
#include <cstdint>

struct FileMeta
{
    uint64_t fileSize;
    GPST start;
    GPST end;
    bool valid;
};

bool loadMeta(const char* filename, FileMeta& meta);
bool saveMeta(const char* filename, const FileMeta& meta);
