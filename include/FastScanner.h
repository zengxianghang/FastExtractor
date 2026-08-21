#pragma once

#include <cstddef>
#include <cstdint>

class FastScanner
{
public:
    explicit FastScanner(const char* filename);
    ~FastScanner();

    bool valid() const;
    bool seek(uint64_t offset, bool skipPartialLine);
    bool nextLine(const char*& data, size_t& length, uint64_t& offset);

private:
    bool ensureLineCapacity(size_t required);

    void* fp_;
    char* buffer_;
    char* line_;
    size_t bufferSize_;
    size_t lineCapacity_;
    size_t pos_;
    size_t size_;
    uint64_t absoluteOffset_;
};
