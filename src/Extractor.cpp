#include "FastExtractByGPST.h"

int extractRange(const char* input,
                 const char* output,
                 const GPST& start,
                 const GPST& end,
                 bool keepNavigation)
{
    if (keepNavigation)
        return fastExtractByGPST(input, output, start, end);

    return fastExtractByGPSTNoNav(input, output, start, end);
}
