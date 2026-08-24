#include "RangeParser.h"
#include <cstring>

static bool fieldEquals(
    const char* data,
    size_t length,
    size_t pos,
    const char* value)
{
    if (!data || !value || pos >= length)
        return false;

    const size_t valueLength = std::strlen(value);
    if (length - pos < valueLength ||
        std::memcmp(data + pos, value, valueLength) != 0)
        return false;

    pos += valueLength;
    return pos == length || data[pos] == ',' || data[pos] == ';' ||
           data[pos] == '\r' || data[pos] == '\n';
}

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
    bool fineTime = false;
    bool haveWeek = false;

    for (size_t i = 0; i < length; ++i)
    {
        if (data[i] != ',')
            continue;

        ++field;
        const size_t valuePos = i + 1;

        if (field == 4)
        {
            fineTime = fieldEquals(data, length, valuePos, "FINE");
            if (!fineTime)
                return false;
        }
        else if (field == 5)
        {
            if (!fineTime || !parseIntField(data, length, valuePos, t.week))
                return false;
            haveWeek = true;
        }
        else if (field == 6)
        {
            if (!fineTime || !haveWeek)
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
