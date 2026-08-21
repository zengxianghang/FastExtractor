#include "RangeParser.h"
#include "FastScanner.h"
#include <cstdio>

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

int extractRange(const char* input,
                 const char* output,
                 const GPST& start,
                 const GPST& end)
{
    FastScanner scanner(input);

    if (!scanner.valid())
        return -1;

    FILE* fout = fopen(output, "wb");

    if (!fout)
        return -2;

    const char* data = nullptr;
    size_t length = 0;
    uint64_t offset = 0;

    bool copy = false;

    while (scanner.nextLine(data, length, offset))
    {
        GPST t;
        bool isRange = false;

        if (length >= 7 && data[0] == '#')
        {
            isRange = parseRangeTimeFast(data, length, t);
        }

        if (isRange)
        {
            if (inside(t, start, end))
            {
                copy = true;
            }
            else if (copy)
            {
                if (t.week > end.week ||
                   (t.week == end.week && t.sow > end.sow))
                {
                    break;
                }
            }
        }

        if (copy)
        {
            fwrite(data, 1, length, fout);
            fwrite("\n", 1, 1, fout);
        }
    }

    fclose(fout);

    return 0;
}
