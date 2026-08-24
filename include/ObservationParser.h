#pragma once

#include "RangeParser.h"

#include <cstddef>

// Parse the GPST carried by an observation-epoch sentence.
// Supported ASCII records:
//   - NovAtel OEM7 RANGEA
//   - Unicore N4 OBSVMA
//
// RANGEA keeps its existing parser semantics. Unicore OBSVMA uses the
// common N4 ASCII header fields: GPS week followed by milliseconds of week.
bool parseObservationTimeFast(const char* data, size_t length, GPST& t);
