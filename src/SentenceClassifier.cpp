#include "SentenceClassifier.h"

#include <cstring>

template <size_t N>
static bool tokenIs(
    const char* token,
    size_t tokenLength,
    const char (&literal)[N])
{
    return tokenLength == N - 1 &&
           std::memcmp(token, literal, N - 1) == 0;
}

SentenceClass classifySentence(const char* data, size_t length)
{
    if (!data || length == 0)
        return SentenceClass::UNKNOWN;

    size_t begin = data[0] == '#' ? 1 : 0;
    size_t end = begin;

    while (end < length &&
           data[end] != ',' && data[end] != ';' &&
           data[end] != '\r' && data[end] != '\n' &&
           data[end] != ' ' && data[end] != '\t')
    {
        ++end;
    }

    if (end == begin)
        return SentenceClass::UNKNOWN;

    const char* token = data + begin;
    const size_t tokenLength = end - begin;

    switch (token[0])
    {
    case 'R':
        if (tokenIs(token, tokenLength, "RANGEA"))
            return SentenceClass::OBSERVATION;
        break;

    case 'O':
        if (tokenIs(token, tokenLength, "OBSVMA"))
            return SentenceClass::OBSERVATION;
        break;

    case 'G':
        if (tokenIs(token, tokenLength, "GLOEPHEMERISA") ||
            tokenIs(token, tokenLength, "GALEPHEMERISA") ||
            tokenIs(token, tokenLength, "GPSEPHEMA") ||
            tokenIs(token, tokenLength, "GPSION") ||
            tokenIs(token, tokenLength, "GPSIONA") ||
            tokenIs(token, tokenLength, "GPSEPH") ||
            tokenIs(token, tokenLength, "GPSEPHA") ||
            tokenIs(token, tokenLength, "GLOEPH") ||
            tokenIs(token, tokenLength, "GLOEPHA") ||
            tokenIs(token, tokenLength, "GALEPH") ||
            tokenIs(token, tokenLength, "GALEPHA") ||
            tokenIs(token, tokenLength, "GALION") ||
            tokenIs(token, tokenLength, "GALIONA"))
        {
            return SentenceClass::NAVIGATION;
        }
        break;

    case 'Q':
        if (tokenIs(token, tokenLength, "QZSSEPHEMERISA") ||
            tokenIs(token, tokenLength, "QZSSEPH") ||
            tokenIs(token, tokenLength, "QZSSEPHA"))
        {
            return SentenceClass::NAVIGATION;
        }
        break;

    case 'B':
        if (tokenIs(token, tokenLength, "BD2EPHEMA") ||
            tokenIs(token, tokenLength, "BD2IONUTCA") ||
            tokenIs(token, tokenLength, "BD3ION") ||
            tokenIs(token, tokenLength, "BD3IONA") ||
            tokenIs(token, tokenLength, "BDSION") ||
            tokenIs(token, tokenLength, "BDSIONA") ||
            tokenIs(token, tokenLength, "BD3EPH") ||
            tokenIs(token, tokenLength, "BD3EPHA") ||
            tokenIs(token, tokenLength, "BDSEPH") ||
            tokenIs(token, tokenLength, "BDSEPHA"))
        {
            return SentenceClass::NAVIGATION;
        }
        break;

    case 'I':
        if (tokenIs(token, tokenLength, "IONUTCA") ||
            tokenIs(token, tokenLength, "IRNSSEPH") ||
            tokenIs(token, tokenLength, "IRNSSEPHA"))
        {
            return SentenceClass::NAVIGATION;
        }
        break;

    default:
        break;
    }

    return SentenceClass::UNKNOWN;
}
