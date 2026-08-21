#include "RangeParser.h"
#include <cstring>
#include <cstdlib>

bool parseRangeTime(const char* line, GPST& t)
{
    if (std::strncmp(line, "#RANGEA", 7) != 0)
        return false;

    int field = 0;
    const char* p = line;

    while (*p)
    {
        if (*p == ',')
        {
            ++field;
            ++p;

            if (field == 5)
                t.week = std::atoi(p);
            else if (field == 6)
            {
                t.sow = std::atof(p);
                return true;
            }
        }
        else
        {
            ++p;
        }
    }

    return false;
}
