#pragma once

#include <cstddef>
#include <cstdint>

class FastScanner
{
public:
    explicit FastScanner(const char* filename);
    ~FastScanner();

    bool valid() const;
    bool nextLine(const char*& data, size_t& length, uint64_t& offset);

private:
    void* fp_;
    char* buffer_;
    size_t bufferSize_;
    size_t pos_;
    size_t size_;
    uint64_t offset_;
};
