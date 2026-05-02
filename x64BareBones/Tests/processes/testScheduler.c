#include "../CuTest.h"
#include "process.h"
#include "scheduler.h"

static int dummyMain(int argc, char ** argv) { (void)argc; (void)argv; return 0; }

void testNewProcessIsReady(CuTest * tc) {
    PCB * p = createProcess("test", dummyMain, 0, NULL, 0, 0);
    CuAssertIntEquals(tc, READY, p->state);
    destroyProcess(p);
}

void testBlockedProcessNotScheduled(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * p1 = createProcess("p1", dummyMain, 0, NULL, 0, 0);
    PCB * p2 = createProcess("p2", dummyMain, 0, NULL, 0, 0);
    addProcess(p1);
    addProcess(p2);
    blockProcess(p1->pid);
    for (int i = 0; i < 10; i++) {
        schedule(0);
        CuAssertTrue(tc, getCurrentProcess()->pid != p1->pid);
    }
}

void testRoundRobinFairness(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * p1 = createProcess("p1", dummyMain, 0, NULL, 0, 0);
    PCB * p2 = createProcess("p2", dummyMain, 0, NULL, 0, 0);
    addProcess(p1);
    addProcess(p2);
    int c1 = 0, c2 = 0;
    for (int i = 0; i < 20; i++) {
        schedule(0);
        if (getCurrentProcess()->pid == p1->pid) c1++;
        else if (getCurrentProcess()->pid == p2->pid) c2++;
    }
    CuAssertTrue(tc, c1 > 0 && c2 > 0);
}

void testKilledProcessNotScheduled(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * p = createProcess("p", dummyMain, 0, NULL, 0, 0);
    uint64_t pid = p->pid;
    addProcess(p);
    killProcess(pid);
    for (int i = 0; i < 5; i++) {
        schedule(0);
        PCB * cur = getCurrentProcess();
        CuAssertTrue(tc, cur == NULL || cur->pid != pid);
    }
}

void testHighPriorityRunsMore(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * hi = createProcess("hi", dummyMain, 0, NULL, 0, 0); /* prioridad 0 */
    PCB * lo = createProcess("lo", dummyMain, 0, NULL, 2, 0); /* prioridad 2 */
    addProcess(hi);
    addProcess(lo);
    int hiCount = 0, loCount = 0;
    for (int i = 0; i < 30; i++) {
        schedule(0);
        if (getCurrentProcess()->pid == hi->pid) hiCount++;
        else if (getCurrentProcess()->pid == lo->pid) loCount++;
    }
    CuAssertTrue(tc, hiCount > loCount);
}

CuSuite * getSchedulerTestSuite(void) {
    CuSuite * suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, testNewProcessIsReady);
    SUITE_ADD_TEST(suite, testBlockedProcessNotScheduled);
    SUITE_ADD_TEST(suite, testRoundRobinFairness);
    SUITE_ADD_TEST(suite, testKilledProcessNotScheduled);
    SUITE_ADD_TEST(suite, testHighPriorityRunsMore);
    return suite;
}
