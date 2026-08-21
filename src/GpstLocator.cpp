#include "GpstLocator.h"

bool estimateOffset(
    uint64_t fileSize,
    const FileTimeInfo& info,
    const GPST& target,
    uint64_t& offset)
{
    if (!info.valid)
        return false;

    double start = info.start.week * 604800.0 + info.start.sow;
    double end   = info.end.week   * 604800.0 + info.end.sow;
    double t     = target.week     * 604800.0 + target.sow;

    if (end <= start)
        return false;

    double ratio = (t - start) / (end - start);

    if (ratio < 0.0)
        ratio = 0.0;
    if (ratio > 1.0)
        ratio = 1.0;

    offset = static_cast<uint64_t>(ratio * static_cast<double>(fileSize));
    return true;
}
