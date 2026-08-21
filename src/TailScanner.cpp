#include "TailScanner.h"
#include <fstream>
#include <vector>
#include <cstring>

static bool findFirstRange(const char* data, size_t size, GPST& t)
{
    for(size_t i=0;i+7<size;i++)
    {
        if(data[i]=='#' && std::strncmp(data+i,"#RANGEA",7)==0)
        {
            return parseRangeTime(data+i,t);
        }
    }
    return false;
}

bool detectTailRange(const char* filename, FileTimeRange& range)
{
    std::ifstream fin(filename,std::ios::binary|std::ios::ate);
    if(!fin)
        return false;

    std::streamoff size=fin.tellg();
    const size_t block=64*1024*1024;

    std::vector<char> buffer((size_t)(size<block?size:block));

    fin.seekg(size-(std::streamoff)buffer.size());
    fin.read(buffer.data(),buffer.size());

    return findFirstRange(buffer.data(),buffer.size(),range.end);
}
