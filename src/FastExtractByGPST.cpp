#include "FastExtractByGPST.h"
#include "FileTimeDetector.h"
#include "GpstLocator.h"
#include "RangeParser.h"

#include <fstream>
#include <string>

static bool inside(const GPST& t, const GPST& s, const GPST& e)
{
    if (t.week < s.week || t.week > e.week)
        return false;

    if (t.week == s.week && t.sow < s.sow)
        return false;

    if (t.week == e.week && t.sow > e.sow)
        return false;

    return true;
}

static bool afterEnd(const GPST& t, const GPST& e)
{
    if (t.week > e.week)
        return true;

    return (t.week == e.week && t.sow > e.sow);
}

int fastExtractByGPST(
    const char* input,
    const char* output,
    const GPST& start,
    const GPST& end)
{
    FileTimeRange range;
    if (!detectFileTimeRange(input, range) || !range.valid)
        return -2;

    FileTimeInfo info;
    info.start = range.start;
    info.end = range.end;
    info.valid = true;

    std::ifstream fin(input, std::ios::binary | std::ios::ate);
    if (!fin)
        return -3;

    uint64_t fileSize = static_cast<uint64_t>(fin.tellg());

    uint64_t offset = 0;
    if (!estimateOffset(fileSize, info, start, offset))
        return -4;

    fin.seekg(static_cast<std::streamoff>(offset));

    std::ofstream fout(output, std::ios::binary);
    if (!fout)
        return -5;

    std::string line;
    bool writing = false;

    while (std::getline(fin, line))
    {
        GPST t;

        if (parseRangeTime(line.c_str(), t))
        {
            if (inside(t, start, end))
            {
                writing = true;
            }
            else if (writing && afterEnd(t, end))
            {
                break;
            }
        }

        if (writing)
        {
            fout.write(line.data(), static_cast<std::streamsize>(line.size()));
            fout.put('\n');
        }
    }

    return writing ? 0 : -6;
}
