#include "FastScanner.h"
#include <cstdio>
#include <cstring>

static const size_t BUFFER_SIZE = 16 * 1024 * 1024;

FastScanner::FastScanner(const char* filename)
    : fp_(nullptr), buffer_(nullptr), bufferSize_(BUFFER_SIZE),
      pos_(0), size_(0), offset_(0)
{
    fp_ = fopen(filename, "rb");
    buffer_ = new char[bufferSize_];
}

FastScanner::~FastScanner()
{
    if (fp_)
        fclose(static_cast<FILE*>(fp_));
    delete [] buffer_;
}

bool FastScanner::valid() const
{
    return fp_ != nullptr;
}

bool FastScanner::nextLine(const char*& data, size_t& length, uint64_t& offset)
{
    static char line[1024 * 1024];
    size_t lineSize = 0;

    while (true)
    {
        if (pos_ >= size_)
        {
            size_ = fread(buffer_, 1, bufferSize_, static_cast<FILE*>(fp_));
            pos_ = 0;
            if (size_ == 0)
                return false;
        }

        char c = buffer_[pos_++];
        if (lineSize == 0)
            offset = offset_ + pos_ - 1;

        offset_++;

        if (c == '\n')
        {
            data = line;
            length = lineSize;
            line[lineSize] = 0;
            return true;
        }

        if (lineSize < sizeof(line)-1)
            line[lineSize++] = c;
    }
}
