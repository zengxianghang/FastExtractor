# FastExtractor

High performance GPST range extractor for large NovAtel log files.

## Features

- Extract data by GPST start/end time.
- Designed for GB-scale NovAtel text logs.
- Uses file-size/time estimation to seek near target position.
- Performs local RANGEA scanning for correction.
- Copies original log content without reformatting.
- C++ implementation with CMake support.

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage

```bash
FastExtractor input.log output.log start_week start_sow end_week end_sow
```

Example:

```bash
FastExtractor test.log result.log 2300 345600 2300 346000
```
