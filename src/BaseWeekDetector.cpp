#include "BaseWeekDetector.h"
#include "FastScanner.h"
#include "ObservationParser.h"

#include <cstddef>
#include <cstdint>

bool detectBaseWeek(const char* input, int& baseWeek)
{
    FastScanner scanner(input);
    if (!scanner.valid())
        return false;

    const char* data = nullptr;
    size_t prefixLength = 0;
    uint64_t offset = 0;
    uint64_t rawLength = 0;

    while (scanner.nextLinePrefix(
        data, prefixLength, offset, rawLength, 256))
    {
        GPST t{};
        if (parseObservationTimeFast(data, prefixLength, t) && t.week > 0)
        {
            baseWeek = t.week;
            return true;
        }
    }

    return false;
}
