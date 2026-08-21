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

int extractRange(const char* input,
                 const char* output,
                 const GPST& start,
                 const GPST& end)
{
    std::ifstream fin(input, std::ios::binary);
    std::ofstream fout(output, std::ios::binary);

    if (!fin || !fout)
        return -1;

    std::string line;
    bool copy = false;

    while (std::getline(fin, line))
    {
        GPST t;
        if (parseRangeTime(line.c_str(), t))
        {
            if (inside(t, start, end))
                copy = true;
            else if (copy && (t.week > end.week ||
                     (t.week == end.week && t.sow > end.sow)))
                break;
        }

        if (copy)
        {
            fout.write(line.data(), (std::streamsize)line.size());
            fout.put('\n');
        }
    }

    return 0;
}
