#include "SentenceClassifier.h"

#include <cstring>

static bool tokenEquals(
    const char* data,
    size_t length,
    const char* token)
{
    if (!data || !token || length == 0)
        return false;

    size_t begin = data[0] == '#' ? 1 : 0;
    size_t end = begin;

    while (end < length &&
           data[end] != ',' && data[end] != ';' &&
           data[end] != '\r' && data[end] != '\n' &&
           data[end] != ' ' && data[end] != '\t')
    {
        ++end;
    }

    const size_t tokenLength = std::strlen(token);
    return end - begin == tokenLength &&
           std::memcmp(data + begin, token, tokenLength) == 0;
}

static bool tokenEqualsBaseOrAscii(
    const char* data,
    size_t length,
    const char* base)
{
    if (tokenEquals(data, length, base))
        return true;

    char asciiName[32];
    const size_t n = std::strlen(base);
    if (n + 1 >= sizeof(asciiName))
        return false;

    std::memcpy(asciiName, base, n);
    asciiName[n] = 'A';
    asciiName[n + 1] = '\0';
    return tokenEquals(data, length, asciiName);
}

SentenceClass classifySentence(const char* data, size_t length)
{
    if (!data || length == 0)
        return SentenceClass::UNKNOWN;

    if (tokenEquals(data, length, "RANGEA") ||
        tokenEquals(data, length, "OBSVMA"))
    {
        return SentenceClass::OBSERVATION;
    }

    // NovAtel OEM7 ASCII names are listed exactly as they appear in logs.
    static const char* novatelNav[] = {
        "GLOEPHEMERISA",
        "QZSSEPHEMERISA",
        "GALEPHEMERISA",
        "GPSEPHEMA",
        "BD2EPHEMA",
        "IONUTCA",
        "BD2IONUTCA"
    };

    for (const char* name : novatelNav)
    {
        if (tokenEquals(data, length, name))
            return SentenceClass::NAVIGATION;
    }

    // Unicore command names are commonly written without the output-format
    // suffix, while ASCII log records append 'A' (for example GPSEPHA).
    static const char* unicoreNav[] = {
        "GPSION",
        "BD3ION",
        "BDSION",
        "GALION",
        "GPSEPH",
        "QZSSEPH",
        "BD3EPH",
        "BDSEPH",
        "GLOEPH",
        "GALEPH",
        "IRNSSEPH"
    };

    for (const char* name : unicoreNav)
    {
        if (tokenEqualsBaseOrAscii(data, length, name))
            return SentenceClass::NAVIGATION;
    }

    return SentenceClass::UNKNOWN;
}
