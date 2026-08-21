#include "FastExtractByGPST.h"
#include "FastScanner.h"
#include "FileTimeDetector.h"
#include "GpstLocator.h"
#include "RangeParser.h"

#include <cstdio>
#include <cstdint>
#include <new>

static int compareGPST(const GPST& a, const GPST& b)
{
    if (a.week < b.week) return -1;
    if (a.week > b.week) return 1;
    if (a.sow < b.sow) return -1;
    if (a.sow > b.sow) return 1;
    return 0;
}

static int seekFile64(FILE* fp, uint64_t offset)
{
#ifdef _WIN32
    return _fseeki64(fp, static_cast<__int64>(offset), SEEK_SET);
#else
    return fseeko(fp, static_cast<off_t>(offset), SEEK_SET);
#endif
}

static bool getFileSize(const char* filename, uint64_t& fileSize)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp)
        return false;

#ifdef _WIN32
    if (_fseeki64(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return false;
    }
    const __int64 size = _ftelli64(fp);
    fclose(fp);
    if (size < 0)
        return false;
    fileSize = static_cast<uint64_t>(size);
#else
    if (fseeko(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return false;
    }
    const off_t size = ftello(fp);
    fclose(fp);
    if (size < 0)
        return false;
    fileSize = static_cast<uint64_t>(size);
#endif
    return true;
}

static bool findFirstRangeFrom(FastScanner& scanner, uint64_t offset, GPST& t, uint64_t& rangeOffset)
{
    if (!scanner.seek(offset, offset != 0))
        return false;

    const char* data = nullptr;
    size_t length = 0;
    uint64_t lineOffset = 0;

    while (scanner.nextLine(data, length, lineOffset))
    {
        if (parseRangeTimeFast(data, length, t))
        {
            rangeOffset = lineOffset;
            return true;
        }
    }

    return false;
}

static bool locateOutwardBounds(
    const char* input,
    uint64_t fileSize,
    uint64_t estimatedOffset,
    const GPST& start,
    const GPST& end,
    uint64_t& startOffset,
    uint64_t& endExclusive,
    GPST& actualStart,
    GPST& actualEnd)
{
    FastScanner scanner(input);
    if (!scanner.valid())
        return false;

    uint64_t searchOffset = estimatedOffset;
    uint64_t backoff = 64ULL * 1024 * 1024;

    // The linear estimate may land after the requested start. Back off until
    // the first RANGE from the search point is <= start, or reach file start.
    while (true)
    {
        GPST firstTime{};
        uint64_t firstOffset = 0;
        const bool found = findFirstRangeFrom(scanner, searchOffset, firstTime, firstOffset);

        if (found && (compareGPST(firstTime, start) <= 0 || searchOffset == 0))
            break;

        if (searchOffset == 0)
            break;

        searchOffset = searchOffset > backoff ? searchOffset - backoff : 0;
        if (backoff < (1ULL << 62))
            backoff *= 2;
    }

    if (!scanner.seek(searchOffset, searchOffset != 0))
        return false;

    const char* data = nullptr;
    size_t length = 0;
    uint64_t lineOffset = 0;

    bool havePrevious = false;
    GPST previousTime{};
    uint64_t previousOffset = 0;

    bool startFound = false;
    bool endCovered = false;
    bool sawRangeAfterStart = false;
    GPST lastRangeAfterStart{};

    while (scanner.nextLine(data, length, lineOffset))
    {
        GPST t{};
        if (!parseRangeTimeFast(data, length, t))
            continue;

        if (!startFound)
        {
            if (compareGPST(t, start) <= 0)
            {
                havePrevious = true;
                previousTime = t;
                previousOffset = lineOffset;
                continue;
            }

            // Outward start: nearest RANGE <= requested start. If none exists,
            // use the first RANGE after start (requested start precedes file data).
            if (havePrevious)
            {
                startOffset = previousOffset;
                actualStart = previousTime;
            }
            else
            {
                startOffset = lineOffset;
                actualStart = t;
            }

            startFound = true;
            sawRangeAfterStart = true;
            lastRangeAfterStart = t;

            // start == end and the previous RANGE is already the outward endpoint.
            if (havePrevious && compareGPST(actualStart, end) >= 0)
            {
                actualEnd = actualStart;
                endExclusive = lineOffset;
                return true;
            }

            if (compareGPST(actualStart, end) >= 0)
            {
                actualEnd = actualStart;
                endCovered = true;
            }
            else if (compareGPST(t, end) >= 0)
            {
                // Outward end: first RANGE >= requested end.
                actualEnd = t;
                endCovered = true;
            }

            continue;
        }

        sawRangeAfterStart = true;
        lastRangeAfterStart = t;

        if (!endCovered)
        {
            if (compareGPST(t, end) >= 0)
            {
                actualEnd = t;
                endCovered = true;
            }
        }
        else
        {
            // Keep the endpoint RANGE and all following non-RANGE records.
            // Stop immediately before the next RANGE.
            endExclusive = lineOffset;
            return true;
        }
    }

    // EOF handling. If no next RANGE exists, deliberately extend to EOF rather
    // than risk dropping data at either boundary.
    if (!startFound && havePrevious)
    {
        startOffset = previousOffset;
        actualStart = previousTime;
        actualEnd = previousTime;
        endExclusive = fileSize;
        return true;
    }

    if (startFound)
    {
        if (!endCovered && sawRangeAfterStart)
            actualEnd = lastRangeAfterStart;
        endExclusive = fileSize;
        return true;
    }

    return false;
}

static bool copyRawRange(
    const char* input,
    const char* output,
    uint64_t startOffset,
    uint64_t endExclusive)
{
    if (endExclusive < startOffset)
        return false;

    FILE* fin = fopen(input, "rb");
    if (!fin)
        return false;

    FILE* fout = fopen(output, "wb");
    if (!fout)
    {
        fclose(fin);
        return false;
    }

    if (seekFile64(fin, startOffset) != 0)
    {
        fclose(fin);
        fclose(fout);
        remove(output);
        return false;
    }

    const size_t bufferSize = 16 * 1024 * 1024;
    char* buffer = new (std::nothrow) char[bufferSize];
    if (!buffer)
    {
        fclose(fin);
        fclose(fout);
        remove(output);
        return false;
    }

    uint64_t remaining = endExclusive - startOffset;
    bool ok = true;

    while (remaining > 0)
    {
        const size_t chunk = remaining < bufferSize ? static_cast<size_t>(remaining) : bufferSize;
        const size_t readCount = fread(buffer, 1, chunk, fin);
        if (readCount == 0)
        {
            ok = false;
            break;
        }

        if (fwrite(buffer, 1, readCount, fout) != readCount)
        {
            ok = false;
            break;
        }

        remaining -= readCount;
    }

    delete [] buffer;
    fclose(fin);
    fclose(fout);

    if (!ok)
        remove(output);

    return ok;
}

int fastExtractByGPST(
    const char* input,
    const char* output,
    const GPST& start,
    const GPST& end)
{
    if (compareGPST(start, end) > 0)
        return -1;

    FileTimeRange range{};
    if (!detectFileTimeRange(input, range) || !range.valid)
        return -2;

    // If the request has no overlap with the file at all, no outward expansion
    // can cover the requested interval, so report it instead of creating junk.
    if (compareGPST(end, range.start) < 0 || compareGPST(start, range.end) > 0)
        return -3;

    uint64_t fileSize = 0;
    if (!getFileSize(input, fileSize))
        return -4;

    FileTimeInfo info{};
    info.start = range.start;
    info.end = range.end;
    info.valid = true;

    uint64_t estimatedOffset = 0;
    if (!estimateOffset(fileSize, info, start, estimatedOffset))
        estimatedOffset = 0;

    uint64_t startOffset = 0;
    uint64_t endExclusive = 0;
    GPST actualStart{};
    GPST actualEnd{};

    if (!locateOutwardBounds(
            input, fileSize, estimatedOffset, start, end,
            startOffset, endExclusive, actualStart, actualEnd))
        return -5;

    if (!copyRawRange(input, output, startOffset, endExclusive))
        return -6;

    std::printf("Requested: %d %.3f ~ %d %.3f\n",
                start.week, start.sow, end.week, end.sow);
    std::printf("Extracted: %d %.3f ~ %d %.3f\n",
                actualStart.week, actualStart.sow, actualEnd.week, actualEnd.sow);

    return 0;
}
