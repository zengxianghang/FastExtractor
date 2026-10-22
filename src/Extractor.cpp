#include "FastExtractByGPST.h"
#include "FileTimeDetector.h"

#include <iomanip>
#include <iostream>
#include <sstream>

static void reportNoOverlap(
    const char* input, const GPST& start, const GPST& end)
{
    std::ostringstream message;
    message << std::fixed << std::setprecision(3)
            << "[ERROR] Requested GPST range does not overlap the input observation range.\n";

    FileTimeRange range{};
    if (detectFileTimeRange(input, range) && range.valid)
    {
        message << "Input observation range: "
                << range.start.week << " " << range.start.sow
                << " ~ " << range.end.week << " " << range.end.sow << "\n";
    }
    else
    {
        message << "Input observation range: unavailable\n";
    }

    message << "Requested range:       "
            << start.week << " " << start.sow
            << " ~ " << end.week << " " << end.sow << "\n"
            << "No output file was created or modified.\n";

    std::cerr << message.str();
}

int extractRange(const char* input,
                 const char* output,
                 const GPST& start,
                 const GPST& end,
                 bool keepNavigation)
{
    const int result = keepNavigation ?
        fastExtractByGPST(input, output, start, end) :
        fastExtractByGPSTNoNav(input, output, start, end);

    if (result == -3)
        reportNoOverlap(input, start, end);

    return result;
}
