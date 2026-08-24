#include "ObservationParser.h"
#include "RangeParser.h"

#include <cstring>

static bool messageNameEquals(
    const char* data,
    size_t length,
    const char* name)
{
    if (!data || !name || length == 0)
        return false;

    size_t pos = data[0] == '#' ? 1 : 0;
    const size_t nameLength = std::strlen(name);

    if (length - pos < nameLength)
        return false;

    if (std::memcmp(data + pos, name, nameLength) != 0)
        return false;

    pos += nameLength;
    return pos == length || data[pos] == ',' || data[pos] == ';' ||
           data[pos] == '\r' || data[pos] == '\n';
}

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

static bool parseUnsignedInt(
    const char* data,
    size_t length,
    size_t pos,
    int& value)
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

static bool parseUnsignedDouble(
    const char* data,
    size_t length,
    size_t pos,
    double& value)
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

static bool parseUnicoreObsvmaTime(
    const char* data,
    size_t length,
    GPST& t)
{
    if (!messageNameEquals(data, length, "OBSVMA"))
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

        // Unicore N4 ASCII header:
        // #OBSVMA,<len>,GPS,<time status>,<week>,<milliseconds of week>,...
        if (field == 3)
        {
            fineTime = fieldEquals(data, length, valuePos, "FINE");
            if (!fineTime)
                return false;
        }
        else if (field == 4)
        {
            if (!fineTime || !parseUnsignedInt(data, length, valuePos, t.week))
                return false;
            haveWeek = true;
        }
        else if (field == 5)
        {
            if (!fineTime || !haveWeek)
                return false;

            double milliseconds = 0.0;
            if (!parseUnsignedDouble(data, length, valuePos, milliseconds))
                return false;

            t.sow = milliseconds * 0.001;
            return true;
        }
    }

    return false;
}

bool parseObservationTimeFast(const char* data, size_t length, GPST& t)
{
    if (parseRangeTimeFast(data, length, t))
        return true;

    return parseUnicoreObsvmaTime(data, length, t);
}
