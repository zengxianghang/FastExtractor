#include "RangeParser.h"

#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int extractRange(const char*, const char*, const GPST&, const GPST&);

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

static void printUsage()
{
    std::cout
        << "Usage:\n"
        << "  FastExtractor input output startWeek startSow endWeek endSow\n"
        << "  FastExtractor input startWeek startSow endWeek endSow\n";
}

int main(int argc, char** argv)
{
    if (argc != 6 && argc != 7)
    {
        printUsage();
        return -1;
    }

    const bool autoOutput = (argc == 6);
    const int timeArg = autoOutput ? 2 : 3;

    GPST start{std::atoi(argv[timeArg]), std::atof(argv[timeArg + 1])};
    GPST end{std::atoi(argv[timeArg + 2]), std::atof(argv[timeArg + 3])};

    std::string generatedOutput;
    const char* output = nullptr;

    if (autoOutput)
    {
        generatedOutput = makeAutoOutputPath(argv[1], start, end);
        output = generatedOutput.c_str();
    }
    else
    {
        output = argv[2];
    }

    const int result = extractRange(argv[1], output, start, end);

    if (result == 0 && autoOutput)
        std::cout << "Output: " << generatedOutput << "\n";

    return result;
}
