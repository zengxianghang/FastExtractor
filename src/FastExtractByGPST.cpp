#include "FastExtractByGPST.h"
#include "FastScanner.h"
#include "ObservationParser.h"
#include "SentenceClassifier.h"

#include <cstdio>
#include <cstdint>
#include <new>

static const size_t HEADER_PREFIX_SIZE = 256;
static const size_t COPY_BUFFER_SIZE = 16 * 1024 * 1024;

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

struct RawSpan
{
    uint64_t offset;
    uint64_t length;
    uint64_t records;
};

class RawSpanList
{
public:
    RawSpanList()
        : spans_(nullptr), count_(0), capacity_(0)
    {
    }

    ~RawSpanList()
    {
        delete [] spans_;
    }

    bool add(uint64_t offset, uint64_t length)
    {
        if (count_ > 0)
        {
            RawSpan& last = spans_[count_ - 1];
            if (last.offset + last.length == offset)
            {
                last.length += length;
                ++last.records;
                return true;
            }
        }

        if (count_ == capacity_)
        {
            const size_t newCapacity = capacity_ == 0 ? 64 : capacity_ * 2;
            RawSpan* replacement = new (std::nothrow) RawSpan[newCapacity];
            if (!replacement)
                return false;

            for (size_t i = 0; i < count_; ++i)
                replacement[i] = spans_[i];

            delete [] spans_;
            spans_ = replacement;
            capacity_ = newCapacity;
        }

        spans_[count_].offset = offset;
        spans_[count_].length = length;
        spans_[count_].records = 1;
        ++count_;
        return true;
    }

    size_t count() const
    {
        return count_;
    }

    const RawSpan& operator[](size_t index) const
    {
        return spans_[index];
    }

private:
    RawSpan* spans_;
    size_t count_;
    size_t capacity_;
};

enum class ScanResult
{
    OK = 0,
    NO_OBSERVATION,
    NO_OVERLAP,
    ERROR
};

static ScanResult scanSelection(
    const char* input,
    uint64_t fileSize,
    const GPST& start,
    const GPST& end,
    RawSpanList& navigationPrefix,
    uint64_t& startOffset,
    uint64_t& endExclusive,
    GPST& actualStart,
    GPST& actualEnd)
{
    FastScanner scanner(input);
    if (!scanner.valid())
        return ScanResult::ERROR;

    const char* data = nullptr;
    size_t prefixLength = 0;
    uint64_t lineOffset = 0;
    uint64_t rawLength = 0;

    bool haveObservation = false;
    GPST lastObservation{};

    bool haveStartCandidate = false;
    GPST startCandidateTime{};
    uint64_t startCandidateOffset = 0;

    bool startResolved = false;
    bool endResolved = false;

    while (scanner.nextLinePrefix(
        data, prefixLength, lineOffset, rawLength, HEADER_PREFIX_SIZE))
    {
        const SentenceClass sentenceClass =
            classifySentence(data, prefixLength);

        if (sentenceClass == SentenceClass::NAVIGATION)
        {
            // Until the final outward start boundary is known, retain only the
            // source spans. They are tiny transient metadata, not a persistent
            // cache/index. Spans at/after startOffset are filtered before write.
            if (!startResolved &&
                !navigationPrefix.add(lineOffset, rawLength))
            {
                return ScanResult::ERROR;
            }
            continue;
        }

        if (sentenceClass != SentenceClass::OBSERVATION)
            continue;

        GPST t{};
        if (!parseObservationTimeFast(data, prefixLength, t))
            continue;

        if (!haveObservation)
        {
            haveObservation = true;
            lastObservation = t;

            // Requested interval lies entirely before the first observation.
            if (compareGPST(end, t) < 0)
                return ScanResult::NO_OVERLAP;
        }
        else
        {
            lastObservation = t;
        }

        if (!startResolved)
        {
            if (compareGPST(t, start) <= 0)
            {
                if (!haveStartCandidate ||
                    compareGPST(t, startCandidateTime) > 0)
                {
                    haveStartCandidate = true;
                    startCandidateTime = t;
                    startCandidateOffset = lineOffset;
                }
                // Same-GPST RANGEA/OBSVMA keeps the first byte offset of the
                // epoch, so all records at the outward start are preserved.
                continue;
            }

            if (haveStartCandidate)
            {
                startOffset = startCandidateOffset;
                actualStart = startCandidateTime;
            }
            else
            {
                startOffset = lineOffset;
                actualStart = t;
            }

            startResolved = true;

            if (compareGPST(actualStart, end) >= 0)
            {
                actualEnd = actualStart;
                endResolved = true;

                // t is the first observation strictly after start. If the
                // candidate epoch already covers end, this line is the first
                // later epoch and therefore the exclusive end offset.
                if (compareGPST(t, actualEnd) > 0)
                {
                    endExclusive = lineOffset;
                    return ScanResult::OK;
                }
            }
            else if (compareGPST(t, end) >= 0)
            {
                actualEnd = t;
                endResolved = true;
            }

            continue;
        }

        if (!endResolved)
        {
            if (compareGPST(t, end) >= 0)
            {
                actualEnd = t;
                endResolved = true;
            }
        }
        else if (compareGPST(t, actualEnd) > 0)
        {
            // Keep every RANGEA/OBSVMA sharing actualEnd GPST plus all
            // following non-observation records; stop at the next epoch.
            endExclusive = lineOffset;
            return ScanResult::OK;
        }
    }

    if (!haveObservation)
        return ScanResult::NO_OBSERVATION;

    if (!startResolved)
    {
        if (!haveStartCandidate || compareGPST(start, lastObservation) > 0)
            return ScanResult::NO_OVERLAP;

        startOffset = startCandidateOffset;
        actualStart = startCandidateTime;
        startResolved = true;

        if (compareGPST(actualStart, end) >= 0)
        {
            actualEnd = actualStart;
            endResolved = true;
        }
    }

    // If requested end is after the file's final observation, keep through EOF
    // and report the final available observation as the actual outward end.
    if (!endResolved)
        actualEnd = lastObservation;

    endExclusive = fileSize;
    return ScanResult::OK;
}

static bool copyRawSpanTo(
    FILE* fin,
    FILE* fout,
    uint64_t offset,
    uint64_t length,
    char* buffer,
    size_t bufferSize)
{
    if (!fin || !fout || !buffer)
        return false;

    if (seekFile64(fin, offset) != 0)
        return false;

    uint64_t remaining = length;
    while (remaining > 0)
    {
        const size_t chunk = remaining < bufferSize ?
            static_cast<size_t>(remaining) : bufferSize;
        const size_t readCount = fread(buffer, 1, chunk, fin);

        if (readCount == 0)
            return false;

        if (fwrite(buffer, 1, readCount, fout) != readCount)
            return false;

        remaining -= readCount;
    }

    return true;
}

static bool writeSelectedData(
    const char* input,
    const char* output,
    const RawSpanList& navigationPrefix,
    uint64_t startOffset,
    uint64_t endExclusive,
    uint64_t& navigationPrefixCount)
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

    char* buffer = new (std::nothrow) char[COPY_BUFFER_SIZE];
    if (!buffer)
    {
        fclose(fin);
        fclose(fout);
        remove(output);
        return false;
    }

    navigationPrefixCount = 0;
    bool ok = true;

    for (size_t i = 0; i < navigationPrefix.count(); ++i)
    {
        const RawSpan& span = navigationPrefix[i];
        if (span.offset >= startOffset)
            break;

        if (!copyRawSpanTo(
                fin, fout, span.offset, span.length,
                buffer, COPY_BUFFER_SIZE))
        {
            ok = false;
            break;
        }

        navigationPrefixCount += span.records;
    }

    if (ok)
    {
        ok = copyRawSpanTo(
            fin, fout, startOffset, endExclusive - startOffset,
            buffer, COPY_BUFFER_SIZE);
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

    uint64_t fileSize = 0;
    if (!getFileSize(input, fileSize))
        return -4;

    RawSpanList navigationPrefix;
    uint64_t startOffset = 0;
    uint64_t endExclusive = 0;
    GPST actualStart{};
    GPST actualEnd{};

    const ScanResult scanResult = scanSelection(
        input, fileSize, start, end, navigationPrefix,
        startOffset, endExclusive, actualStart, actualEnd);

    if (scanResult == ScanResult::NO_OBSERVATION)
        return -2;
    if (scanResult == ScanResult::NO_OVERLAP)
        return -3;
    if (scanResult != ScanResult::OK)
        return -5;

    uint64_t navigationPrefixCount = 0;
    if (!writeSelectedData(
            input, output, navigationPrefix,
            startOffset, endExclusive, navigationPrefixCount))
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
