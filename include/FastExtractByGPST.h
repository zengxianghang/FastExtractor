#pragma once

#include "RangeParser.h"

int fastExtractByGPST(
    const char* input,
    const char* output,
    const GPST& start,
    const GPST& end);
