#include "ObservationParser.h"
#include "SentenceClassifier.h"

#include <cmath>
#include <cstdio>
#include <cstring>

static int failures = 0;

static void expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}

static void testObsvmaTime()
{
    const char* fine =
        "#OBSVMA,94,GPS,FINE,2190,117395000,0,0,18,17;18,0,26";
    GPST t{};

    expect(parseObservationTimeFast(fine, std::strlen(fine), t),
           "parse OBSVMA FINE header");
    expect(t.week == 2190, "OBSVMA GPS week");
    expect(std::fabs(t.sow - 117395.0) < 1e-9, "OBSVMA milliseconds to SOW");

    const char* nonFine =
        "#OBSVMA,94,GPS,COARSE,2190,117395000,0,0,18,17;18,0,26";
    GPST nonFineTime{};

    expect(parseObservationTimeFast(
               nonFine, std::strlen(nonFine), nonFineTime),
           "parse OBSVMA non-FINE header");
    expect(nonFineTime.week == 2190, "non-FINE OBSVMA GPS week");
    expect(std::fabs(nonFineTime.sow - 117395.0) < 1e-9,
           "non-FINE OBSVMA SOW");
}

static void testSentenceClassification()
{
    const char* range = "#RANGEA,COM1,0,0,FINE,2300,100.0";
    const char* obsvma = "#OBSVMA,94,GPS,FINE,2190,117395000,0,0,18,17;";
    const char* novatelEph = "#GLOEPHEMERISA,COM1,0,0,FINE,2300,50.0";
    const char* unicoreEph = "#GPSEPHA,97,GPS,FINE,2190,362528000,0,0,18,1;";
    const char* unicoreIon = "#GPSIONA,60,GPS,COARSE,2190,362000000,0,0,18,1;";
    const char* other = "#BESTPOSA,COM1,0,0,FINE,2300,100.0";

    expect(classifySentence(range, std::strlen(range)) ==
               SentenceClass::OBSERVATION,
           "classify RANGEA as observation");
    expect(classifySentence(obsvma, std::strlen(obsvma)) ==
               SentenceClass::OBSERVATION,
           "classify OBSVMA as observation");
    expect(classifySentence(novatelEph, std::strlen(novatelEph)) ==
               SentenceClass::NAVIGATION,
           "classify NovAtel ephemeris");
    expect(classifySentence(unicoreEph, std::strlen(unicoreEph)) ==
               SentenceClass::NAVIGATION,
           "classify Unicore ephemeris ASCII suffix");
    expect(classifySentence(unicoreIon, std::strlen(unicoreIon)) ==
               SentenceClass::NAVIGATION,
           "classify non-FINE Unicore ionosphere record");
    expect(classifySentence(other, std::strlen(other)) ==
               SentenceClass::UNKNOWN,
           "do not classify unrelated record");
}

int main()
{
    testObsvmaTime();
    testSentenceClassification();

    if (failures != 0)
    {
        std::fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }

    std::printf("ParserTests passed\n");
    return 0;
}
