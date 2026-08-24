#pragma once

#include <cstddef>

enum class SentenceClass
{
    UNKNOWN = 0,
    OBSERVATION,
    NAVIGATION
};

SentenceClass classifySentence(const char* data, size_t length);
