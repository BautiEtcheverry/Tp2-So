#include <stdio.h>
#include <stdlib.h>
#include "CuTest.h"

CuSuite * getSchedulerTestSuite(void);

int main(void) {
    CuString * output = CuStringNew();
    CuSuite  * suite  = CuSuiteNew();

    CuSuiteAddSuite(suite, getSchedulerTestSuite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

    int failed = suite->failCount;
    CuSuiteDelete(suite);
    CuStringDelete(output);
    return failed ? 1 : 0;
}
