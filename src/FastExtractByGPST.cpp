#include "FastExtractByGPST.h"
#include "FastScanner.h"
#include "FileTimeDetector.h"
#include "GpstLocator.h"
#include "ObservationParser.h"
#include "RangeParser.h"
#include "SentenceClassifier.h"

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

static bool findFirstObservationFrom(
    FastScanner& scanner,
    uint64_t offset,
    GPST& t,
    uint64_t& observationOffset)
{
    if (!scanner.seek(offset, offset != 0))
        return false;

    const char* data = nullptr;
    size_t length = 0;
    uint64_t lineOffset = 0;

    while (scanner.nextLine(data, length, lineOffset))
    {
        if (parseObservationTimeFast(data, length, t))
        {
            observationOffset = lineOffset;
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

    // The interpolation estimate may land after the requested start. Back off
    // until the first observation from the search point is <= start, or until
    // the file start is reached. Observation means RANGEA or OBSVMA.
    while (true)
    {
        GPST firstTime{};
        uint64_t firstOffset = 0;
        const bool found = findFirstObservationFrom(
            scanner, searchOffset, firstTime, firstOffset);

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
    bool sawObservationAfterStart = false;
    GPST lastObservationAfterStart{};

    while (scanner.nextLine(data, length, lineOffset))
    {
        GPST t{};
        if (!parseObservationTimeFast(data, length, t))
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

            // Outward start: nearest observation <= requested start. If none
            // exists, use the first observation after start.
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
            sawObservationAfterStart = true;
            lastObservationAfterStart = t;

            // start == end and an exact previous observation already covers it.
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
                // Outward end: first observation >= requested end.
                actualEnd = t;
                endCovered = true;
            }

            continue;
        }

        sawObservationAfterStart = true;
        lastObservationAfterStart = t;

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
            // Keep the endpoint observation and all following non-observation
            // records. Stop immediately before the next observation epoch.
            endExclusive = lineOffset;
            return true;
        }
    }

    // EOF handling deliberately extends rather than risking dropped data.
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
        if (!endCovered && sawObservationAfterStart)
            actualEnd = lastObservationAfterStart;
        endExclusive = fileSize;
        return true;
    }

    return false;
}

static bool writeNavigationPrefix(
    const char* input,
    FILE* fout,
    uint64_t stopOffset,
    uint64_t& keptCount)
{
    keptCount = 0;
    if (stopOffset == 0)
        return true;

    FastScanner scanner(input);
    if (!scanner.valid())
        return false;

    const char* data = nullptr;
    size_t length = 0;
    uint64_t lineOffset = 0;

    while (scanner.nextLine(data, length, lineOffset))
    {
        if (lineOffset >= stopOffset)
            break;

        if (classifySentence(data, length) != SentenceClass::NAVIGATION)
            continue;

        // FastScanner removes only '\n'. For CRLF input the retained data still
        // ends in '\r', so writing '\n' reconstructs the original line ending.
        if (length > 0 && fwrite(data, 1, length, fout) != length)
            return false;
        if (fwrite("\n", 1, 1, fout) != 1)
            return false;

        ++keptCount;
    }

    return true;
}

static bool copyRawRangeTo(
    FILE* fin,
    FILE* fout,
    uint64_t startOffset,
    uint64_t endExclusive)
{
    if (!fin || !fout || endExclusive < startOffset)
        return false;

    if (seekFile64(fin, startOffset) != 0)
        return false;

    const size_t bufferSize = 16 * 1024 * 1024;
    char* buffer = new (std::nothrow) char[bufferSize];
    if (!buffer)
        return false;

    uint64_t remaining = endExclusive - startOffset;
    bool ok = true;

    while (remaining > 0)
    {
        const size_t chunk = remaining < bufferSize ?
            static_cast<size_t>(remaining) : bufferSize;
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
    return ok;
}

static bool writeSelectedData(
    const char* input,
    const char* output,
    uint64_t startOffset,
    uint64_t endExclusive,
    uint64_t& navigationPrefixCount)
{
    FILE* fin = fopen(input, "rb");
    if (!fin)
        return false;

    FILE* fout = fopen(output, "wb");
    if (!fout)
    {
        fclose(fin);
        return false;
    }

    bool ok = writeNavigationPrefix(
        input, fout, startOffset, navigationPrefixCount);

    if (ok)
        ok = copyRawRangeTo(fin, fout, startOffset, endExclusive);

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

    if (compareGPST(end, range.start) < 0 ||
        compareGPST(start, range.end) > 0)
    {
        return -3;
    }

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
    {
        return -5;
    }

    uint64_t navigationPrefixCount = 0;
    if (!writeSelectedData(
            input, output, startOffset, endExclusive,
            navigationPrefixCount))
    {
        return -6;
    }

    std::printf("Requested: %d %.3f ~ %d %.3f\n",
                start.week, start.sow, end.week, end.sow);
    std::printf("Extracted observation bounds: %d %.3f ~ %d %.3f\n",
                actualStart.week, actualStart.sow,
                actualEnd.week, actualEnd.sow);
    std::printf("Navigation records kept before start: %llu\n",
                static_cast<unsigned long long>(navigationPrefixCount));

    return 0;
}
