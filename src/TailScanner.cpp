#include "TailScanner.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>

static bool findLastRange(const char* data, size_t size, GPST& t)
{
    if (size < 7)
        return false;

    for (size_t i = size - 7; ; --i)
    {
        if (data[i] == '#' && std::memcmp(data + i, "#RANGEA", 7) == 0)
        {
            if (parseRangeTimeFast(data + i, size - i, t))
                return true;
        }

        if (i == 0)
            break;
    }

    return false;
}

bool detectTailRange(const char* filename, FileTimeRange& range)
{
    std::ifstream fin(filename, std::ios::binary | std::ios::ate);
    if (!fin)
        return false;

    const std::streamoff fileSize = fin.tellg();
    if (fileSize <= 0)
        return false;

    const uint64_t blockSize = 64ULL * 1024 * 1024;
    const uint64_t overlap = 64ULL * 1024;
    uint64_t endPos = static_cast<uint64_t>(fileSize);

    while (endPos > 0)
    {
        const uint64_t startPos = endPos > blockSize ? endPos - blockSize : 0;
        const size_t readSize = static_cast<size_t>(endPos - startPos);
        std::vector<char> buffer(readSize);

        fin.clear();
        fin.seekg(static_cast<std::streamoff>(startPos));
        fin.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const size_t got = static_cast<size_t>(fin.gcount());

        if (findLastRange(buffer.data(), got, range.end))
            return true;

        if (startPos == 0)
            break;

        endPos = startPos + (overlap < readSize ? overlap : 0);
    }

    return false;
}
