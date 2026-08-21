# FastExtractor

High performance GPST range extractor for large NovAtel log files.

## Features

- Extract data by GPST start/end time.
- Designed for GB-scale NovAtel text logs.
- Uses file-size/time estimation to seek near target position.
- Performs local RANGEA scanning for correction.
- Uses outward GPST boundaries so boundary data is not accidentally dropped.
- Copies the selected byte range directly, preserving the original log bytes.
- Supports automatic output filename generation in the input file directory.
- Supports continuous GPST seconds without explicitly supplying GPS week, including cross-week intervals.
- C++ implementation with CMake support.

## Boundary policy

FastExtractor intentionally prefers extracting slightly more data rather than dropping data near the requested GPST boundaries.

For a requested interval `[start, end]`:

- Start boundary: use the nearest `RANGEA` whose GPST is `<= start`.
- If no earlier RANGE exists but the request overlaps the file, start from the first RANGE in the file.
- End boundary: use the first `RANGEA` whose GPST is `>= end`.
- After the selected end RANGE, keep all following non-RANGE records and stop immediately before the next RANGE.
- If there is no RANGE after the requested end, extraction continues to EOF.
- If the requested interval has no overlap with the file GPST range, extraction fails instead of creating a misleading output file.

Example, if RANGE epochs are `100.0, 101.0, 102.0, 103.0, 104.0` and the request is `100.4 ~ 102.3`, the effective RANGE bounds are `100.0 ~ 103.0`.

The program prints both ranges:

```text
Requested: 2300 100.400 ~ 2300 102.300
Extracted: 2300 100.000 ~ 2300 103.000
```

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage

FastExtractor supports four CLI forms.

### Explicit GPS week, explicit output filename

```bash
FastExtractor input.log output.log start_week start_sow end_week end_sow
```

Example:

```bash
FastExtractor test.log result.log 2300 345600 2300 346000
```

### Explicit GPS week, automatic output filename

```bash
FastExtractor input.log start_week start_sow end_week end_sow
```

### Continuous GPST seconds, explicit output filename

```bash
FastExtractor input.log output.log start_sec end_sec
```

### Continuous GPST seconds, automatic output filename

```bash
FastExtractor input.log start_sec end_sec
```

In continuous-seconds mode, the GPS week of the first valid `RANGEA` record is used as the base week. The input seconds may exceed one GPS week (`604800` seconds). Each whole `604800` seconds advances the resolved GPS week by one.

For example, if the input file starts in GPS week `2300`:

```bash
FastExtractor test.log 10 604810
```

resolves to:

```text
Requested seconds: 10.000 ~ 604810.000
Resolved GPST: 2300 10.000 ~ 2301 10.000
```

The same rule naturally supports multiple weeks, for example `1209610` resolves to `baseWeek + 2, sow = 10`.

Continuous seconds must satisfy:

```text
0 <= start_sec <= end_sec
```

`start_sec == end_sec` is allowed and uses the same outward-boundary extraction policy.

## Automatic output filename

When the output filename is omitted, the output is stored in the same directory as the input file. The generated filename is:

```text
<stem>_<startWeek>_<startSow>_<endWeek>_<endSow><extension>
```

The resolved GPST values are used in the filename, formatted to three decimal places. For example:

```bash
FastExtractor D:\data\test.log 10 604810
```

with base GPS week `2300` generates:

```text
D:\data\test_2300_10.000_2301_10.000.log
```

If the input file has no extension, the generated output file also has no extension.
