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

Explicit output filename:

```bash
FastExtractor input.log output.log start_week start_sow end_week end_sow
```

Example:

```bash
FastExtractor test.log result.log 2300 345600 2300 346000
```

Automatic output filename:

```bash
FastExtractor input.log start_week start_sow end_week end_sow
```

The output is stored in the same directory as the input file. The generated filename is:

```text
<stem>_<startWeek>_<startSow>_<endWeek>_<endSow><extension>
```

The requested GPST values are used in the filename, formatted to three decimal places. For example:

```bash
FastExtractor D:\data\test.log 2300 100.4 2300 200.3
```

generates:

```text
D:\data\test_2300_100.400_2300_200.300.log
```

If the input file has no extension, the generated output file also has no extension.
