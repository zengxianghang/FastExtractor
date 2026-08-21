#include "RangeParser.h"
#include <cstring>
#include <cstdlib>

static bool parseIntField(const char* p, int& value)
{
    value = 0;

    while (*p >= '0' && *p <= '9')
    {
        value = value * 10 + (*p - '0');
        ++p;
    }

    return true;
}

static bool parseDoubleField(const char* p, double& value)
{
    value = 0.0;

    double scale = 0.1;
    bool decimal = false;

    while (*p >= '0' && *p <= '9')
    {
        if (decimal)
        {
            value += (*p - '0') * scale;
            scale *= 0.1;
        }
        else
        {
            value = value * 10.0 + (*p - '0');
        }

        ++p;

        if (*p == '.')
        {
            decimal = true;
            ++p;
        }
    }

    return true;
}

bool parseRangeTimeFast(const char* data, size_t length, GPST& t)
{
    if (length < 7 || std::memcmp(data, "#RANGEA", 7) != 0)
        return false;

    int field = 0;

    for (size_t i = 0; i < length; ++i)
    {
        if (data[i] != ',')
            continue;

        ++field;

        if (field == 5)
        {
            parseIntField(data + i + 1, t.week);
        }
        else if (field == 6)
        {
            parseDoubleField(data + i + 1, t.sow);
            return true;
        }
    }

    return false;
}

bool parseRangeTime(const char* line, GPST& t)
{
    return parseRangeTimeFast(line, std::strlen(line), t);
}
