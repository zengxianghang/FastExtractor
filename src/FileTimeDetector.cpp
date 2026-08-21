#include "FileTimeDetector.h"
#include <fstream>

bool detectFileTimeRange(const char* filename, FileTimeRange& range)
{
    range.valid = false;

    std::ifstream fin(filename, std::ios::binary);
    if(!fin)
        return false;

    const size_t BUF_SIZE = 16 * 1024 * 1024;
    char* buf = new char[BUF_SIZE];

    GPST first{};
    GPST last{};
    bool foundFirst = false;

    while(fin)
    {
        fin.read(buf, BUF_SIZE);
        std::streamsize n = fin.gcount();

        for(std::streamsize i=0;i<n;i++)
        {
            if(buf[i]=='#')
            {
                GPST t;
                if(parseRangeTime(buf+i, t))
                {
                    if(!foundFirst)
                    {
                        first=t;
                        foundFirst=true;
                    }
                    last=t;
                }
            }
        }
    }

    delete [] buf;

    if(foundFirst)
    {
        range.start=first;
        range.end=last;
        range.valid=true;
    }

    return range.valid;
}
