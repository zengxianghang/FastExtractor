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

    // Scan one physical line while retaining only its first maxPrefix bytes.
    // rawLength is the exact byte span in the source file, including the
    // trailing '\n' when present. This avoids copying large RANGEA/OBSVMA
    // bodies when only the ASCII header is needed.
    bool nextLinePrefix(
        const char*& data,
        size_t& prefixLength,
        uint64_t& offset,
        uint64_t& rawLength,
        size_t maxPrefix);

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
