#include "TailScanner.h"
#include <fstream>
#include <vector>
#include <cstring>

static bool findLastRange(const char* data, size_t size, GPST& t)
{
    if(size < 7)
        return false;

    for(size_t i = size - 7; ; --i)
    {
        if(data[i] == '#' &&
           std::strncmp(data + i, "#RANGEA", 7) == 0)
        {
            if(parseRangeTime(data + i, t))
                return true;
        }

        if(i == 0)
            break;
    }

    return false;
}

bool detectTailRange(const char* filename, FileTimeRange& range)
{
    std::ifstream fin(filename, std::ios::binary | std::ios::ate);
    if(!fin)
        return false;

    std::streamoff size = fin.tellg();

    const size_t blocks[] =
    {
        64ULL * 1024 * 1024,
        256ULL * 1024 * 1024,
        1024ULL * 1024 * 1024
    };

    for(size_t k = 0; k < 3; k++)
    {
        size_t readSize = blocks[k];
        if((std::streamoff)readSize > size)
            readSize = (size_t)size;

        std::vector<char> buffer(readSize);

        fin.clear();
        fin.seekg(size - (std::streamoff)readSize);
        fin.read(buffer.data(), buffer.size());

        if(findLastRange(buffer.data(), buffer.size(), range.end))
            return true;
    }

    return false;
}
