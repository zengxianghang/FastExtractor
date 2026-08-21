#include "RangeParser.h"
#include <cstring>

static bool parseIntField(const char* data, size_t length, size_t pos, int& value)
{
    if (pos >= length || data[pos] < '0' || data[pos] > '9')
        return false;

    value = 0;
    while (pos < length && data[pos] >= '0' && data[pos] <= '9')
    {
        value = value * 10 + (data[pos] - '0');
        ++pos;
    }
    return true;
}

static bool parseDoubleField(const char* data, size_t length, size_t pos, double& value)
{
    if (pos >= length || data[pos] < '0' || data[pos] > '9')
        return false;

    value = 0.0;
    while (pos < length && data[pos] >= '0' && data[pos] <= '9')
    {
        value = value * 10.0 + (data[pos] - '0');
        ++pos;
    }

    if (pos < length && data[pos] == '.')
    {
        ++pos;
        double scale = 0.1;
        while (pos < length && data[pos] >= '0' && data[pos] <= '9')
        {
            value += (data[pos] - '0') * scale;
            scale *= 0.1;
            ++pos;
        }
    }

    return true;
}

bool parseRangeTimeFast(const char* data, size_t length, GPST& t)
{
    if (!data || length < 7 || std::memcmp(data, "#RANGEA", 7) != 0)
        return false;

    int field = 0;
    bool haveWeek = false;

    for (size_t i = 0; i < length; ++i)
    {
        if (data[i] != ',')
            continue;

        ++field;
        const size_t valuePos = i + 1;

        if (field == 5)
        {
            if (!parseIntField(data, length, valuePos, t.week))
                return false;
            haveWeek = true;
        }
        else if (field == 6)
        {
            if (!haveWeek)
                return false;
            return parseDoubleField(data, length, valuePos, t.sow);
        }
    }

    return false;
}

bool parseRangeTime(const char* line, GPST& t)
{
    if (!line)
        return false;
    return parseRangeTimeFast(line, std::strlen(line), t);
}
