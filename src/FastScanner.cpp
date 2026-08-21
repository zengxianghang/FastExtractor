#include "FastScanner.h"
#include <cstdio>
#include <new>

static const size_t BUFFER_SIZE = 16 * 1024 * 1024;
static const size_t INITIAL_LINE_CAPACITY = 64 * 1024;

static int seekFile64(FILE* fp, uint64_t offset)
{
#ifdef _WIN32
    return _fseeki64(fp, static_cast<__int64>(offset), SEEK_SET);
#else
    return fseeko(fp, static_cast<off_t>(offset), SEEK_SET);
#endif
}

FastScanner::FastScanner(const char* filename)
    : fp_(nullptr), buffer_(nullptr), line_(nullptr), bufferSize_(BUFFER_SIZE),
      lineCapacity_(INITIAL_LINE_CAPACITY), pos_(0), size_(0), absoluteOffset_(0)
{
    fp_ = fopen(filename, "rb");
    buffer_ = new (std::nothrow) char[bufferSize_];
    line_ = new (std::nothrow) char[lineCapacity_];

    if (!buffer_ || !line_)
    {
        if (fp_)
        {
            fclose(static_cast<FILE*>(fp_));
            fp_ = nullptr;
        }
        delete [] buffer_;
        delete [] line_;
        buffer_ = nullptr;
        line_ = nullptr;
    }
}

FastScanner::~FastScanner()
{
    if (fp_)
        fclose(static_cast<FILE*>(fp_));
    delete [] buffer_;
    delete [] line_;
}

bool FastScanner::valid() const
{
    return fp_ != nullptr && buffer_ != nullptr && line_ != nullptr;
}

bool FastScanner::ensureLineCapacity(size_t required)
{
    if (required <= lineCapacity_)
        return true;

    size_t newCapacity = lineCapacity_;
    while (newCapacity < required)
        newCapacity *= 2;

    char* newLine = new (std::nothrow) char[newCapacity];
    if (!newLine)
        return false;

    for (size_t i = 0; i < lineCapacity_; ++i)
        newLine[i] = line_[i];

    delete [] line_;
    line_ = newLine;
    lineCapacity_ = newCapacity;
    return true;
}

bool FastScanner::seek(uint64_t offset, bool skipPartialLine)
{
    if (!valid())
        return false;

    FILE* fp = static_cast<FILE*>(fp_);
    if (seekFile64(fp, offset) != 0)
        return false;

    uint64_t alignedOffset = offset;

    if (offset > 0 && skipPartialLine)
    {
        if (seekFile64(fp, offset - 1) != 0)
            return false;

        const int previous = fgetc(fp);
        if (previous == EOF)
            return false;

        if (previous != '\n')
        {
            int c = 0;
            while ((c = fgetc(fp)) != EOF)
            {
                ++alignedOffset;
                if (c == '\n')
                    break;
            }
        }
        // If previous == '\n', fgetc already positioned the file at exactly offset.
    }

    pos_ = 0;
    size_ = 0;
    absoluteOffset_ = alignedOffset;
    clearerr(fp);
    return true;
}

bool FastScanner::nextLine(const char*& data, size_t& length, uint64_t& offset)
{
    if (!valid())
        return false;

    size_t lineSize = 0;
    const uint64_t lineStart = absoluteOffset_;

    while (true)
    {
        if (pos_ >= size_)
        {
            size_ = fread(buffer_, 1, bufferSize_, static_cast<FILE*>(fp_));
            pos_ = 0;

            if (size_ == 0)
            {
                if (lineSize == 0)
                    return false;

                data = line_;
                length = lineSize;
                offset = lineStart;
                return true;
            }
        }

        const char c = buffer_[pos_++];
        ++absoluteOffset_;

        if (c == '\n')
        {
            data = line_;
            length = lineSize;
            offset = lineStart;
            return true;
        }

        if (!ensureLineCapacity(lineSize + 1))
            return false;

        line_[lineSize++] = c;
    }
}
