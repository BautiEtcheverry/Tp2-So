#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static char * cutest_strdup(const char * s) {
    size_t len = strlen(s) + 1;
    char * copy = (char *) malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

/* ---- CuString ---- */

void CuStringInit(CuString * str) {
    str->length = 0;
    str->size   = STRING_MAX;
    str->buffer = (char *) malloc(sizeof(char) * str->size);
    str->buffer[0] = '\0';
}

CuString * CuStringNew(void) {
    CuString * str = (CuString *) malloc(sizeof(CuString));
    CuStringInit(str);
    return str;
}

void CuStringDelete(CuString * str) {
    if (!str) return;
    free(str->buffer);
    free(str);
}

void CuStringResize(CuString * str, int newSize) {
    str->buffer = (char *) realloc(str->buffer, sizeof(char) * newSize);
    str->size   = newSize;
}

void CuStringAppend(CuString * str, const char * text) {
    int length = (int) strlen(text);
    if (str->length + length + 1 >= str->size)
        CuStringResize(str, str->length + length + 1 + STRING_INC);
    str->length += length;
    strcat(str->buffer, text);
}

void CuStringAppendChar(CuString * str, char ch) {
    char text[2];
    text[0] = ch;
    text[1] = '\0';
    CuStringAppend(str, text);
}

void CuStringAppendFormat(CuString * str, const char * format, ...) {
    va_list argp;
    char    buf[HUGE_STRING_LEN];
    va_start(argp, format);
    vsnprintf(buf, sizeof(buf), format, argp);
    va_end(argp);
    CuStringAppend(str, buf);
}

void CuStringInsert(CuString * str, const char * text, int pos) {
    int length = (int) strlen(text);
    if (str->length + length + 1 >= str->size)
        CuStringResize(str, str->length + length + 1 + STRING_INC);
    memmove(str->buffer + pos + length,
            str->buffer + pos,
            (size_t)(str->length - pos) + 1);
    memcpy(str->buffer + pos, text, (size_t) length);
    str->length += length;
}

/* ---- CuTest ---- */

void CuTestInit(CuTest * t, const char * name, TestFunction function) {
    t->name     = cutest_strdup(name);
    t->failed   = 0;
    t->ran      = 0;
    t->message  = NULL;
    t->jumpBuf  = NULL;
    t->function = function;
}

CuTest * CuTestNew(const char * name, TestFunction function) {
    CuTest * tc = (CuTest *) malloc(sizeof(CuTest));
    CuTestInit(tc, name, function);
    return tc;
}

void CuTestDelete(CuTest * t) {
    if (!t) return;
    free(t->name);
    free(t);
}

void CuTestRun(CuTest * tc) {
    jmp_buf buf;
    tc->jumpBuf = &buf;
    if (setjmp(buf) == 0) {
        tc->ran = 1;
        (tc->function)(tc);
    }
    tc->jumpBuf = NULL;
}

/* ---- Assertion helpers ---- */

void CuFail_Line(CuTest * tc, const char * file, int line,
                 const char * message2, const char * message) {
    CuString * string = CuStringNew();
    if (message2)
        CuStringAppendFormat(string, "%s: %d: %s: ", file, line, message2);
    else
        CuStringAppendFormat(string, "%s: %d: ", file, line);
    CuStringAppend(string, message);
    tc->failed  = 1;
    tc->message = string->buffer;
    if (tc->jumpBuf) longjmp(*(tc->jumpBuf), 0);
}

void CuAssert_Line(CuTest * tc, const char * file, int line,
                   const char * message, int condition) {
    if (condition) return;
    CuFail_Line(tc, file, line, NULL, message);
}

void CuAssertStrEquals_LineMsg(CuTest * tc, const char * file, int line,
                                const char * message,
                                const char * expected, const char * actual) {
    CuString * string;
    if ((expected == NULL && actual == NULL) ||
        (expected != NULL && actual != NULL && strcmp(expected, actual) == 0))
        return;
    string = CuStringNew();
    if (message)
        CuStringAppendFormat(string, "%s: %d: %s: ", file, line, message);
    else
        CuStringAppendFormat(string, "%s: %d: ", file, line);
    CuStringAppendFormat(string, "expected <%s> but was <%s>",
                         expected ? expected : "(null)",
                         actual   ? actual   : "(null)");
    tc->failed  = 1;
    tc->message = string->buffer;
    if (tc->jumpBuf) longjmp(*(tc->jumpBuf), 0);
}

void CuAssertIntEquals_LineMsg(CuTest * tc, const char * file, int line,
                                const char * message,
                                int expected, int actual) {
    char buf[STRING_MAX];
    if (expected == actual) return;
    snprintf(buf, sizeof(buf), "expected <%d> but was <%d>", expected, actual);
    CuFail_Line(tc, file, line, message, buf);
}

void CuAssertDblEquals_LineMsg(CuTest * tc, const char * file, int line,
                                const char * message,
                                double expected, double actual, double delta) {
    char   buf[STRING_MAX];
    double diff = expected - actual;
    if (diff < 0.0) diff = -diff;
    if (diff <= delta) return;
    snprintf(buf, sizeof(buf), "expected <%f> but was <%f>", expected, actual);
    CuFail_Line(tc, file, line, message, buf);
}

void CuAssertPtrEquals_LineMsg(CuTest * tc, const char * file, int line,
                                const char * message,
                                void * expected, void * actual) {
    char buf[STRING_MAX];
    if (expected == actual) return;
    snprintf(buf, sizeof(buf), "expected pointer <%p> but was <%p>",
             expected, actual);
    CuFail_Line(tc, file, line, message, buf);
}

/* ---- CuSuite ---- */

void CuSuiteInit(CuSuite * testSuite) {
    testSuite->count     = 0;
    testSuite->failCount = 0;
    memset(testSuite->list, 0, sizeof(testSuite->list));
}

CuSuite * CuSuiteNew(void) {
    CuSuite * suite = (CuSuite *) malloc(sizeof(CuSuite));
    CuSuiteInit(suite);
    return suite;
}

void CuSuiteDelete(CuSuite * testSuite) {
    int n;
    for (n = 0; n < testSuite->count; n++)
        CuTestDelete(testSuite->list[n]);
    free(testSuite);
}

void CuSuiteAdd(CuSuite * testSuite, CuTest * testCase) {
    testSuite->list[testSuite->count++] = testCase;
}

void CuSuiteAddSuite(CuSuite * testSuite, CuSuite * testSuite2) {
    int i;
    for (i = 0; i < testSuite2->count; i++)
        CuSuiteAdd(testSuite, testSuite2->list[i]);
}

void CuSuiteRun(CuSuite * testSuite) {
    int i;
    for (i = 0; i < testSuite->count; i++) {
        CuTest * testCase = testSuite->list[i];
        CuTestRun(testCase);
        if (testCase->failed) testSuite->failCount++;
    }
}

void CuSuiteSummary(CuSuite * testSuite, CuString * summary) {
    int i;
    for (i = 0; i < testSuite->count; i++)
        CuStringAppend(summary, testSuite->list[i]->failed ? "F" : ".");
    CuStringAppend(summary, "\n\n");
}

void CuSuiteDetails(CuSuite * testSuite, CuString * details) {
    int i, failCount = 0;
    if (testSuite->failCount == 0) {
        int passCount = testSuite->count;
        CuStringAppendFormat(details, "OK (%d %s)\n",
                             passCount, passCount == 1 ? "test" : "tests");
        return;
    }
    if (testSuite->failCount == 1)
        CuStringAppend(details, "\nThere was 1 failure:\n");
    else
        CuStringAppendFormat(details, "\nThere were %d failures:\n",
                             testSuite->failCount);
    for (i = 0; i < testSuite->count; i++) {
        CuTest * testCase = testSuite->list[i];
        if (testCase->failed) {
            failCount++;
            CuStringAppendFormat(details, "%d) %s: %s\n",
                                 failCount, testCase->name, testCase->message);
        }
    }
    CuStringAppend(details, "\n!!!FAILURES!!!\n\n");
    CuStringAppendFormat(details, "Runs: %d   Failures: %d\n",
                         testSuite->count, testSuite->failCount);
}
