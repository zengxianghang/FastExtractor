#include "FastExtractByGPST.h"

int extractRange(const char* input,
                 const char* output,
                 const GPST& start,
                 const GPST& end)
{
    return fastExtractByGPST(input, output, start, end);
}
