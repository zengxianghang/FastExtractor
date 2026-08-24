#include "SentenceClassifier.h"

#include <cstring>

static bool startsWith(const char* data, size_t length, const char* token)
{
    const size_t n = std::strlen(token);
    return length >= n && std::memcmp(data, token, n) == 0;
}

SentenceClass classifySentence(const char* data, size_t length)
{
    if (!data || length == 0)
        return SentenceClass::UNKNOWN;

    if (startsWith(data, length, "#RANGEA") ||
        startsWith(data, length, "OBSVMA"))
        return SentenceClass::OBSERVATION;

    static const char* nav[] = {
        "GLOEPHEMERISA", "QZSSEPHEMERISA", "GALEPHEMERISA",
        "GPSEPHEMA", "BD2EPHEMA", "IONUTCA", "BD2IONUTCA",
        "GPSION", "BD3ION", "BDSION", "GALION",
        "GPSEPH", "QZSSEPH", "BD3EPH", "BDSEPH",
        "GLOEPH", "GALEPH", "IRNSSEPH"
    };

    for (const char* s : nav)
    {
        if (startsWith(data, length, s))
            return SentenceClass::NAVIGATION;
    }

    return SentenceClass::UNKNOWN;
}
