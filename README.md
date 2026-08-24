# FastExtractor

High performance GPST window extractor for large GNSS ASCII log files.

## Features

- Extract data by GPST start/end time.
- Supports NovAtel OEM7 `RANGEA` observation epochs.
- Supports Unicore N4 `OBSVMA` observation epochs.
- Uses outward GPST boundaries so observation data near requested boundaries is not accidentally dropped.
- Retains selected NovAtel/Unicore ephemeris and ionosphere records that occur before the observation start boundary.
- Navigation retention does not require the navigation record's time status to be `FINE`.
- Copies the main observation window directly from the original file, preserving original bytes.
- Supports automatic output filename generation in the input file directory.
- Supports continuous GPST seconds without explicitly supplying GPS week, including cross-week intervals.
- C++17 implementation with CMake/CTest support.

## Observation boundary policy

`RANGEA` and `OBSVMA` are both treated as observation-epoch records and use the same outward-boundary policy.

For a requested interval `[start, end]`:

- Start boundary: use the nearest observation epoch whose GPST is `<= start`.
- If multiple `RANGEA`/`OBSVMA` records share the selected start GPST, start at the first record of that epoch.
- If no earlier observation exists but the request overlaps the file, start from the first observation in the file.
- End boundary: use the first observation epoch whose GPST is `>= end`.
- Keep every `RANGEA`/`OBSVMA` record sharing the selected end GPST.
- Keep following non-observation records and stop immediately before the first observation whose GPST is strictly later than the selected end epoch.
- If there is no later observation, extraction continues to EOF.
- If the requested interval has no overlap with the file observation GPST range, extraction fails instead of creating a misleading output file.

Example, if observation epochs are `100.0, 101.0, 102.0, 103.0, 104.0` and the request is `100.4 ~ 102.3`, the effective observation bounds are `100.0 ~ 103.0`.

## Navigation retention policy

Before the selected observation start boundary, FastExtractor keeps only the configured navigation record types. Once the observation window starts, the existing raw-window behavior is preserved: records in the selected byte range remain in their original order and original bytes.

Navigation records before `start` are retained solely by message type and file position. Their header time status does **not** need to be `FINE`; this intentionally preserves navigation information emitted during receiver startup or other non-FINE periods.

Supported NovAtel OEM7 navigation records:

- `GLOEPHEMERISA`
- `QZSSEPHEMERISA`
- `GALEPHEMERISA`
- `GPSEPHEMA`
- `BD2EPHEMA`
- `IONUTCA`
- `BD2IONUTCA`

Supported Unicore N4 navigation records (ASCII log records normally carry the trailing `A`, e.g. `GPSEPHA`):

- `GPSION`
- `BD3ION`
- `BDSION`
- `GALION`
- `GPSEPH`
- `QZSSEPH`
- `BD3EPH`
- `BDSEPH`
- `GLOEPH`
- `GALEPH`
- `IRNSSEPH`

Because the requirement is to retain **all** matching navigation records before the selected observation start, a first-time extraction must scan the file prefix up to the start boundary. The observation-boundary locator still uses estimated seeking, but the navigation-prefix requirement adds prefix-read I/O. A future metadata/index cache can remove most of this repeated-scan cost for repeated extractions from the same file.

The program prints the requested range, effective observation bounds, and the number of navigation records retained before start:

```text
Requested: 2300 100.400 ~ 2300 102.300
Extracted observation bounds: 2300 100.000 ~ 2300 103.000
Navigation records kept before start: 42
```

## Unicore OBSVMA time handling

For Unicore N4 `OBSVMA`, the observation GPST is read from the common ASCII message header. The GPS week is taken from the header week field, and the header milliseconds-of-week value is converted to seconds-of-week. The header time-status field is not required to be `FINE` for parsing the observation epoch.

For example:

```text
#OBSVMA,94,GPS,FINE,2190,117395000,...
```

resolves to:

```text
week = 2190
sow  = 117395.000
```

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
ctest --output-on-failure
```

## Usage

FastExtractor supports four CLI forms.

### Explicit GPS week, explicit output filename

```bash
FastExtractor input.log output.log start_week start_sow end_week end_sow
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

In continuous-seconds mode, the GPS week of the first valid `RANGEA` **or** `OBSVMA` observation record is used as the base week. The input seconds may exceed one GPS week (`604800` seconds). Each whole `604800` seconds advances the resolved GPS week by one.

For example, if the input file starts in GPS week `2300`:

```bash
FastExtractor test.log 10 604810
```

resolves to:

```text
Requested seconds: 10.000 ~ 604810.000
Resolved GPST: 2300 10.000 ~ 2301 10.000
```

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
