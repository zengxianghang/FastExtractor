#pragma once

#include "RangeParser.h"

// NAV-history mode: retains all configured navigation records before the
// outward observation start boundary.
int fastExtractByGPST(
    const char* input,
    const char* output,
    const GPST& start,
    const GPST& end);

// Default fast mode: restores the pre-NAV-history extraction strategy and
// copies only the outward observation byte window.
int fastExtractByGPSTNoNav(
    const char* input,
    const char* output,
    const GPST& start,
    const GPST& end);
