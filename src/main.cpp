#include "BaseWeekDetector.h"
#include "RangeParser.h"
#include "FastScanner.h"
#include "ObservationParser.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

int extractRange(
    const char*, const char*, const GPST&, const GPST&, bool keepNavigation);

static std::string makeAutoOutputPath(
    const char* input,
    const GPST& start,
    const GPST& end)
{
    const std::filesystem::path inputPath(input);

    std::ostringstream name;
    name << inputPath.stem().string()
         << "_" << start.week
         << "_" << std::fixed << std::setprecision(3) << start.sow
         << "_" << end.week
         << "_" << std::fixed << std::setprecision(3) << end.sow
         << inputPath.extension().string();

    return (inputPath.parent_path() / name.str()).string();
}

static bool parseDoubleStrict(const char* text, double& value)
{
    if (!text || *text == '\0')
        return false;

    char* end = nullptr;
    value = std::strtod(text, &end);

    return end != text && *end == '\0' && std::isfinite(value);
}

static bool resolveContinuousSeconds(int baseWeek, double seconds, GPST& t)
{
    if (baseWeek <= 0 || !std::isfinite(seconds) || seconds < 0.0)
        return false;

    const double weekOffsetDouble = std::floor(seconds / 604800.0);
    const double maxOffset = static_cast<double>(std::numeric_limits<int>::max()) -
                             static_cast<double>(baseWeek);

    if (weekOffsetDouble < 0.0 || weekOffsetDouble > maxOffset)
        return false;

    int weekOffset = static_cast<int>(weekOffsetDouble);
    double sow = seconds - weekOffsetDouble * 604800.0;

    if (sow >= 604800.0)
    {
        if (baseWeek + weekOffset == std::numeric_limits<int>::max())
            return false;
        ++weekOffset;
        sow -= 604800.0;
    }

    if (sow < 0.0)
        sow = 0.0;

    t.week = baseWeek + weekOffset;
    t.sow = sow;
    return true;
}

static void printUsage()
{
    std::cout
        << "Usage:\n"
        << "  FastExtractor [--keep-nav] input output startWeek startSow endWeek endSow\n"
        << "  FastExtractor [--keep-nav] input startWeek startSow endWeek endSow\n"
        << "  FastExtractor [--keep-nav] input output startSec endSec\n"
        << "  FastExtractor [--keep-nav] input startSec endSec\n\n"
        << "Default: NAV history is not retained.\n"
        << "Use --keep-nav to retain configured navigation records before the outward start boundary.\n";
}

int main(int argc, char** argv)
{
    const bool keepNavigation =
        argc > 1 && std::strcmp(argv[1], "--keep-nav") == 0;
    const int shift = keepNavigation ? 1 : 0;
    const int effectiveArgc = argc - shift;
    char** args = argv + shift;

    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '-' &&
        !keepNavigation)
    {
        std::cerr << "Unknown option: " << argv[1] << "\n";
        printUsage();
        return -1;
    }

    if (effectiveArgc < 4 || effectiveArgc > 7)
    {
        printUsage();
        return -1;
    }

    const bool implicitWeek = (effectiveArgc == 4 || effectiveArgc == 5);
    const bool autoOutput = (effectiveArgc == 4 || effectiveArgc == 6);
    const int timeArg = autoOutput ? 2 : 3;

    GPST start{};
    GPST end{};
    double startSec = 0.0;
    double endSec = 0.0;

    if (implicitWeek)
    {
        if (!parseDoubleStrict(args[timeArg], startSec) ||
            !parseDoubleStrict(args[timeArg + 1], endSec) ||
            startSec < 0.0 || endSec < 0.0 || startSec > endSec)
        {
            std::cerr << "Invalid continuous GPST seconds. Require 0 <= startSec <= endSec.\n";
            return -1;
        }

        int baseWeek = 0;
        if (!detectBaseWeek(args[1], baseWeek))
        {
            std::cerr << "Unable to detect a valid base GPS week from RANGEA/OBSVMA records.\n";
            return -2;
        }

        if (!resolveContinuousSeconds(baseWeek, startSec, start) ||
            !resolveContinuousSeconds(baseWeek, endSec, end))
        {
            std::cerr << "Continuous GPST seconds are outside the supported GPS week range.\n";
            return -1;
        }

        std::cout << std::fixed << std::setprecision(3)
                  << "Requested seconds: " << startSec << " ~ " << endSec << "\n"
                  << "Resolved GPST: " << start.week << " " << start.sow
                  << " ~ " << end.week << " " << end.sow << "\n";
    }
    else
    {
        start.week = std::atoi(args[timeArg]);
        start.sow = std::atof(args[timeArg + 1]);
        end.week = std::atoi(args[timeArg + 2]);
        end.sow = std::atof(args[timeArg + 3]);
    }

    std::string generatedOutput;
    const char* output = nullptr;

    if (autoOutput)
    {
        generatedOutput = makeAutoOutputPath(args[1], start, end);
        output = generatedOutput.c_str();
    }
    else
    {
        output = args[2];
    }

    const int result = extractRange(
        args[1], output, start, end, keepNavigation);

    if (result == 0 && autoOutput)
        std::cout << "Output: " << generatedOutput << "\n";

    return result;
}
