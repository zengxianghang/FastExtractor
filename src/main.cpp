#include "RangeParser.h"
#include <iostream>

int extractRange(const char*, const char*, const GPST&, const GPST&);

int main(int argc, char** argv)
{
    if (argc != 7)
    {
        std::cout << "Usage: FastExtractor input output startWeek startSow endWeek endSow\n";
        return -1;
    }

    GPST start{std::atoi(argv[3]), std::atof(argv[4])};
    GPST end{std::atoi(argv[5]), std::atof(argv[6])};

    return extractRange(argv[1], argv[2], start, end);
}
