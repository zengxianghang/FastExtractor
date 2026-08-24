#include "FastExtractByGPST.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

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
    expect(result == 0, "mixed extraction succeeds");

    const std::string expected =
        nav0 + obs99 + nav99 + obs100 + nav100 +
        obs101r + obs101u + afterEnd;

    expect(readFile(output) == expected,
           "navigation prefix and mixed outward window preserve raw bytes");

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
    expect(result == 0, "OBSVMA-only extraction succeeds");
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

int main()
{
    testMixedObservationAndNavigation();
    testObsvmaOnly();
    testEqualBoundaryKeepsWholeEpoch();
    testNoOverlapDoesNotCreateOutput();

    if (failures != 0)
    {
        std::fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    std::printf("ExtractionTests passed\n");
    return 0;
}
