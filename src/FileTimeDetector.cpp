#include "FileTimeDetector.h"
#include "FastScanner.h"
#include "TailScanner.h"

bool detectFileTimeRange(const char* filename, FileTimeRange& range)
{
    range.valid = false;

    FastScanner scanner(filename);
    if (!scanner.valid())
        return false;

    const char* data = nullptr;
    size_t length = 0;
    uint64_t offset = 0;
    GPST first{};
    bool foundFirst = false;

    while (scanner.nextLine(data, length, offset))
    {
        if (parseRangeTimeFast(data, length, first))
        {
            foundFirst = true;
            break;
        }
    }

    if (!foundFirst)
        return false;

    FileTimeRange tail{};
    if (!detectTailRange(filename, tail))
        return false;

    range.start = first;
    range.end = tail.end;
    range.valid = true;
    return true;
}
