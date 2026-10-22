#include "BaseWeekDetector.h"
#include "FastExtractByGPST.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int extractRange(
    const char*, const char*, const GPST&, const GPST&, bool keepNavigation);

static int failures = 0;

static void expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

static bool writeFile(const char* path, const std::string& content)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
        return false;
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    return out.good();
}

static std::string readFile(const char* path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream content;
    content << in.rdbuf();
    return content.str();
}

static bool fileExists(const char* path)
{
    std::ifstream in(path, std::ios::binary);
    return in.good();
}

static void testMixedObservationAndNavigation()
{
    const char* input = "fast_extractor_mixed_input.log";
    const char* output = "fast_extractor_mixed_output.log";
    std::remove(input);
    std::remove(output);

    const std::string nav0 =
        "#GPSEPHEMA,COM1,0,0,COARSE,2300,50.0;NAV0*00000000\r\n";
    const std::string other0 =
        "#BESTPOSA,COM1,0,0,FINE,2300,80.0;OTHER0*00000000\r\n";
    const std::string obs99 =
        "#RANGEA,COM1,0,0,FINE,2300,99.0;OBS99*00000000\r\n";
    const std::string nav99 =
        "#GPSIONA,60,GPS,COARSE,2300,99500,0,0,18,1;NAV99*00000000\r\n";
    const std::string obs100 =
        "#OBSVMA,94,GPS,FINE,2300,100000,0,0,18,1;OBS100*00000000\r\n";
    const std::string nav100 =
        "#GPSEPHA,97,GPS,FINE,2300,100500,0,0,18,1;NAV100*00000000\r\n";
    const std::string obs101r =
        "#RANGEA,COM1,0,0,FINE,2300,101.0;OBS101R*00000000\r\n";
    const std::string obs101u =
        "#OBSVMA,94,GPS,FINE,2300,101000,0,0,18,1;OBS101U*00000000\r\n";
    const std::string afterEnd =
        "#BESTPOSA,COM1,0,0,FINE,2300,101.2;AFTEREND*00000000\r\n";
    const std::string obs102 =
        "#RANGEA,COM1,0,0,FINE,2300,102.0;OBS102*00000000\r\n";

    std::string largeUnknown = "#UNRELATEDA,";
    largeUnknown.append(512 * 1024, 'X');
    largeUnknown += "\r\n";

    const std::string inputData =
        nav0 + other0 + largeUnknown + obs99 + nav99 + obs100 +
        nav100 + obs101r + obs101u + afterEnd + obs102;

    expect(writeFile(input, inputData), "write mixed extraction input");

    const GPST start{2300, 99.5};
    const GPST end{2300, 100.5};
    const int result = fastExtractByGPST(input, output, start, end);
    expect(result == 0, "keep-NAV mixed extraction succeeds");

    const std::string expected =
        nav0 + obs99 + nav99 + obs100 + nav100 +
        obs101r + obs101u + afterEnd;

    expect(readFile(output) == expected,
           "keep-NAV mode preserves navigation prefix and raw outward window");

    std::remove(input);
    std::remove(output);
}

static void testNoNavFastPath()
{
    const char* input = "fast_extractor_no_nav_input.log";
    const char* output = "fast_extractor_no_nav_output.log";
    std::remove(input);
    std::remove(output);

    const std::string nav0 =
        "#GPSEPHEMA,COM1,0,0,COARSE,2300,50.0;NAV0*00000000\r\n";
    const std::string other0 =
        "#BESTPOSA,COM1,0,0,FINE,2300,80.0;OTHER0*00000000\r\n";
    const std::string obs99 =
        "#RANGEA,COM1,0,0,FINE,2300,99.0;OBS99*00000000\r\n";
    const std::string nav99 =
        "#GPSIONA,60,GPS,COARSE,2300,99500,0,0,18,1;NAV99*00000000\r\n";
    const std::string obs100 =
        "#OBSVMA,94,GPS,FINE,2300,100000,0,0,18,1;OBS100*00000000\r\n";
    const std::string obs101 =
        "#RANGEA,COM1,0,0,FINE,2300,101.0;OBS101*00000000\r\n";
    const std::string trailing =
        "#BESTPOSA,COM1,0,0,FINE,2300,101.2;TRAIL*00000000\r\n";
    const std::string obs102 =
        "#RANGEA,COM1,0,0,FINE,2300,102.0;OBS102*00000000\r\n";

    expect(writeFile(
               input,
               nav0 + other0 + obs99 + nav99 + obs100 +
               obs101 + trailing + obs102),
           "write no-NAV input");

    const GPST start{2300, 99.5};
    const GPST end{2300, 100.5};
    const int result = fastExtractByGPSTNoNav(input, output, start, end);
    expect(result == 0, "no-NAV fast extraction succeeds");

    const std::string expected =
        obs99 + nav99 + obs100 + obs101 + trailing;
    expect(readFile(output) == expected,
           "no-NAV mode omits pre-start NAV but preserves raw window contents");

    std::remove(input);
    std::remove(output);
}

static void testGpsCnavRetentionModes()
{
    const char* input = "fast_extractor_gpscnav_input.log";
    const char* output = "fast_extractor_gpscnav_output.log";
    std::remove(input);
    std::remove(output);

    // GPSEPH and both GPSCNAVEPH spellings receive the same NAV treatment.
    const std::string cnavBare =
        "#GPSCNAVEPH,94,GPS,COARSE,2300,40000,0,0,18,1;CNAV_BARE*00000000\r\n";
    const std::string legacy =
        "#GPSEPHA,94,GPS,FINE,2300,41000,0,0,18,1;LEGACY*00000000\r\n";
    const std::string cnavAscii =
        "#GPSCNAVEPHA,94,GPS,UNKNOWN,2300,42000,0,0,18,1;CNAV_A*00000000\r\n";
    const std::string unknown =
        "#GPSCNAVEPH_EXTRA,94,GPS,FINE,2300,43000,0,0,18,1;UNKNOWN*00000000\r\n";
    const std::string obs99 =
        "#RANGEA,COM1,0,0,FINE,2300,99.0;OBS99*00000000\r\n";
    const std::string cnavWindow =
        "#GPSCNAVEPHA,94,GPS,FINE,2300,99500,0,0,18,1;IN_WINDOW*00000000\r\n";
    const std::string obs100 =
        "#OBSVMA,94,GPS,FINE,2300,100000,0,0,18,1;OBS100*00000000\r\n";
    const std::string obs101 =
        "#RANGEA,COM1,0,0,FINE,2300,101.0;OBS101*00000000\r\n";
    const std::string trailing =
        "#BESTPOSA,COM1,0,0,FINE,2300,101.2;TRAIL*00000000\r\n";
    const std::string obs102 =
        "#RANGEA,COM1,0,0,FINE,2300,102.0;OBS102*00000000\r\n";

    expect(writeFile(input, cnavBare + legacy + cnavAscii + unknown +
                            obs99 + cnavWindow + obs100 + obs101 +
                            trailing + obs102),
           "write GPSCNAVEPH extraction input");

    const GPST start{2300, 99.5};
    const GPST end{2300, 100.5};
    const std::string window = obs99 + cnavWindow + obs100 + obs101 + trailing;

    expect(fastExtractByGPST(input, output, start, end) == 0,
           "GPSCNAVEPH keep-NAV extraction succeeds");
    expect(readFile(output) == cnavBare + legacy + cnavAscii + window,
           "keep-NAV retains both pre-start CNAV variants and GPSEPH, "
           "including non-FINE status, excludes unknown prefixes, "
           "and preserves CRLF and raw window ordering");

    std::remove(output);
    expect(fastExtractByGPSTNoNav(input, output, start, end) == 0,
           "GPSCNAVEPH default extraction succeeds");
    expect(readFile(output) == window,
           "default mode omits pre-start CNAV and GPSEPH but keeps CNAV in window");

    std::remove(input);
    std::remove(output);
}

static void testObsvmaOnly()
{
    const char* input = "fast_extractor_obsvma_input.log";
    const char* output = "fast_extractor_obsvma_output.log";
    std::remove(input);
    std::remove(output);

    const std::string nav =
        "#GALIONA,60,GPS,COARSE,2300,5000,0,0,18,1;NAV*00000000\n";
    const std::string obs10 =
        "#OBSVMA,94,GPS,FINE,2300,10000,0,0,18,1;O10*00000000\n";
    const std::string obs11 =
        "#OBSVMA,94,GPS,FINE,2300,11000,0,0,18,1;O11*00000000\n";
    const std::string obs12 =
        "#OBSVMA,94,GPS,FINE,2300,12000,0,0,18,1;O12*00000000\n";

    expect(writeFile(input, nav + obs10 + obs11 + obs12),
           "write OBSVMA-only input");

    const GPST start{2300, 10.2};
    const GPST end{2300, 10.8};
    const int result = fastExtractByGPST(input, output, start, end);
    expect(result == 0, "OBSVMA-only keep-NAV extraction succeeds");
    expect(readFile(output) == nav + obs10 + obs11,
           "OBSVMA-only outward bounds are correct");

    std::remove(input);
    std::remove(output);
}

static void testEqualBoundaryKeepsWholeEpoch()
{
    const char* input = "fast_extractor_equal_input.log";
    const char* output = "fast_extractor_equal_output.log";
    std::remove(input);
    std::remove(output);

    const std::string nav50 =
        "#GPSEPHA,97,GPS,FINE,2300,50000,0,0,18,1;N50*00000000\n";
    const std::string obs100 =
        "#RANGEA,COM1,0,0,FINE,2300,100.0;O100*00000000\n";
    const std::string nav100 =
        "#GPSIONA,60,GPS,COARSE,2300,100500,0,0,18,1;N100*00000000\n";
    const std::string obs101r =
        "#RANGEA,COM1,0,0,FINE,2300,101.0;O101R*00000000\n";
    const std::string obs101u =
        "#OBSVMA,94,GPS,FINE,2300,101000,0,0,18,1;O101U*00000000\n";
    const std::string trailing =
        "#BESTPOSA,COM1,0,0,FINE,2300,101.2;TRAIL*00000000\n";
    const std::string obs102 =
        "#RANGEA,COM1,0,0,FINE,2300,102.0;O102*00000000\n";

    expect(writeFile(
               input,
               nav50 + obs100 + nav100 + obs101r +
               obs101u + trailing + obs102),
           "write equal-boundary input");

    const GPST start{2300, 101.0};
    const GPST end{2300, 101.0};
    const int result = fastExtractByGPST(input, output, start, end);
    expect(result == 0, "equal-boundary extraction succeeds");

    const std::string expected =
        nav50 + nav100 + obs101r + obs101u + trailing;
    expect(readFile(output) == expected,
           "equal boundary keeps all same-epoch observations and NAV history");

    std::remove(input);
    std::remove(output);
}

static void testNoOverlapDoesNotCreateOutput()
{
    const char* input = "fast_extractor_no_overlap_input.log";
    const char* output = "fast_extractor_no_overlap_output.log";
    std::remove(input);
    std::remove(output);

    const std::string data =
        "#GPSEPHA,97,GPS,FINE,2300,5000,0,0,18,1;NAV*00000000\n"
        "#OBSVMA,94,GPS,FINE,2300,10000,0,0,18,1;O10*00000000\n";

    expect(writeFile(input, data), "write no-overlap input");

    const GPST earlyStart{2300, 1.0};
    const GPST earlyEnd{2300, 2.0};
    int result = fastExtractByGPST(
        input, output, earlyStart, earlyEnd);
    expect(result == -3, "early no-overlap extraction returns -3");
    expect(!fileExists(output), "early no-overlap creates no output");

    const GPST lateStart{2300, 20.0};
    const GPST lateEnd{2300, 21.0};
    result = fastExtractByGPST(input, output, lateStart, lateEnd);
    expect(result == -3, "late no-overlap extraction returns -3");
    expect(!fileExists(output), "late no-overlap creates no output");

    std::remove(input);
    std::remove(output);
}

static void testNonFineObservationsDoNotDefineBounds()
{
    const char* input = "fast_extractor_non_fine_bounds_input.log";
    const char* output = "fast_extractor_non_fine_bounds_output.log";
    std::remove(input);
    std::remove(output);

    const std::string fine100 =
        "#RANGEA,COM1,0,0,FINE,2300,100.0;FINE100*00000000\n";
    const std::string coarseRange =
        "#RANGEA,COM1,0,0,COARSE,2300,100.4;COARSE_R*00000000\n";
    const std::string coarseObsvma =
        "#OBSVMA,94,GPS,COARSE,2300,100500,0,0,18,1;COARSE_U*00000000\n";
    const std::string fine101 =
        "#OBSVMA,94,GPS,FINE,2300,101000,0,0,18,1;FINE101*00000000\n";
    const std::string fine102 =
        "#RANGEA,COM1,0,0,FINE,2300,102.0;FINE102*00000000\n";

    expect(writeFile(
               input,
               fine100 + coarseRange + coarseObsvma + fine101 + fine102),
           "write non-FINE boundary input");

    const GPST start{2300, 100.6};
    const GPST end{2300, 100.8};
    const int result = fastExtractByGPST(input, output, start, end);
    expect(result == 0, "non-FINE boundary extraction succeeds");

    const std::string expected =
        fine100 + coarseRange + coarseObsvma + fine101;
    expect(readFile(output) == expected,
           "non-FINE RANGEA/OBSVMA do not define outward bounds");

    std::remove(input);
    std::remove(output);
}

static void testBaseWeekRequiresFineObservation()
{
    const char* rangeInput = "fast_extractor_base_week_range.log";
    const char* obsvmaInput = "fast_extractor_base_week_obsvma.log";
    const char* invalidInput = "fast_extractor_base_week_invalid.log";
    std::remove(rangeInput);
    std::remove(obsvmaInput);
    std::remove(invalidInput);

    const std::string rangeData =
        "#RANGEA,COM1,0,0,COARSE,2299,5.0;COARSE*00000000\n"
        "#RANGEA,COM1,0,0,FINE,2300,10.0;VALID*00000000\n";
    expect(writeFile(rangeInput, rangeData),
           "write RANGEA base-week input");

    int week = 0;
    expect(detectBaseWeek(rangeInput, week),
           "RANGEA base-week detection succeeds after COARSE record");
    expect(week == 2300,
           "RANGEA base-week detection uses first FINE week");

    const std::string obsvmaData =
        "#OBSVMA,94,GPS,UNKNOWN,2299,5000,0,0,18,1;UNKNOWN*00000000\n"
        "#OBSVMA,94,GPS,FINE,2301,10000,0,0,18,1;VALID*00000000\n";
    expect(writeFile(obsvmaInput, obsvmaData),
           "write OBSVMA base-week input");

    week = 0;
    expect(detectBaseWeek(obsvmaInput, week),
           "OBSVMA base-week detection succeeds after UNKNOWN record");
    expect(week == 2301,
           "OBSVMA base-week detection uses first FINE week");

    const std::string invalidData =
        "#RANGEA,COM1,0,0,COARSE,2300,5.0;COARSE*00000000\n"
        "#OBSVMA,94,GPS,UNKNOWN,2300,5000,0,0,18,1;UNKNOWN*00000000\n";
    expect(writeFile(invalidInput, invalidData),
           "write non-FINE-only base-week input");

    week = 0;
    expect(!detectBaseWeek(invalidInput, week),
           "base-week detection rejects files containing no FINE observation");

    std::remove(rangeInput);
    std::remove(obsvmaInput);
    std::remove(invalidInput);
}

static void testNoOverlapDiagnosticsBothModes()
{
    const char* input = "fast_extractor_diagnostic_input.log";
    const char* output = "fast_extractor_diagnostic_output.log";
    std::remove(input);
    std::remove(output);

    const std::string data =
        "#GPSEPHEMA,97,GPS,FINE,2300,5000,0,0,18,1;NAV*00000000\n"
        "#OBSVMA,94,GPS,FINE,2300,10000,0,0,18,1;O10*00000000\n"
        "#RANGEA,COM1,0,0,FINE,2300,11.0;O11*00000000\n"
        "#RANGEA,COM1,0,0,FINE,2300,12.0;O12*00000000\n";
    expect(writeFile(input, data), "write diagnostic input");

    struct Scenario
    {
        GPST start;
        GPST end;
        const char* expectedRequest;
    };

    const Scenario scenarios[] = {
        {{2300, 1.0}, {2300, 2.0}, "2300 1.000 ~ 2300 2.000"},
        {{2300, 20.0}, {2300, 21.0}, "2300 20.000 ~ 2300 21.000"}
    };

    for (int mode = 0; mode < 2; ++mode)
    {
        const bool keepNavigation = mode != 0;
        for (const Scenario& scenario : scenarios)
        {
            std::remove(output);
            std::ostringstream captured;
            std::streambuf* previous = std::cerr.rdbuf(captured.rdbuf());
            const int result = extractRange(
                input, output, scenario.start, scenario.end, keepNavigation);
            std::cerr.rdbuf(previous);

            expect(result == -3, "no overlap returns -3 in both modes");
            expect(!fileExists(output), "no overlap does not create a file");

            const std::string diagnostic = captured.str();
            expect(diagnostic.find("[ERROR] Requested GPST range does not overlap") !=
                   std::string::npos, "diagnostic explains no overlap");
            expect(diagnostic.find("2300 10.000 ~ 2300 12.000") !=
                   std::string::npos, "diagnostic shows input observation range");
            expect(diagnostic.find(scenario.expectedRequest) !=
                   std::string::npos, "diagnostic shows requested range");
            expect(diagnostic.find("No output file was created or modified.") !=
                   std::string::npos, "diagnostic describes output behavior");
        }

        const std::string original = "EXISTING_OUTPUT_MUST_BE_PRESERVED\n";
        expect(writeFile(output, original), "write preexisting output");
        std::ostringstream captured;
        std::streambuf* previous = std::cerr.rdbuf(captured.rdbuf());
        const int result = extractRange(
            input, output, scenarios[0].start, scenarios[0].end,
            keepNavigation);
        std::cerr.rdbuf(previous);

        expect(result == -3, "preexisting output and no overlap returns -3");
        expect(readFile(output) == original,
               "preexisting output remains unchanged after no overlap");
    }

    std::remove(input);
    std::remove(output);
}

int main()
{
    testMixedObservationAndNavigation();
    testNoNavFastPath();
    testGpsCnavRetentionModes();
    testObsvmaOnly();
    testEqualBoundaryKeepsWholeEpoch();
    testNoOverlapDoesNotCreateOutput();
    testNoOverlapDiagnosticsBothModes();
    testNonFineObservationsDoNotDefineBounds();
    testBaseWeekRequiresFineObservation();

    if (failures != 0)
    {
        std::fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    std::printf("ExtractionTests passed\n");
    return 0;
}
