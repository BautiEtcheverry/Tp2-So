#ifndef USERLAND_BENCHMARKS_ACTUALTIME_H
#define USERLAND_BENCHMARKS_ACTUALTIME_H
#include "libc.h"

/*
    Actual times provides an implementation of a precise time measurement function. 
    It uses the CPU's Time Stamp Counter (TSC), to enssure an accurate measurment of time intervals.

    The main function, actualTime, returns the time elapsed since its first invocation in nanoseconds.

    - read_tsc: Reads the current value of the CPU's Time Stamp Counter.
    - seconds_since_midnight: Retrieves the current time in seconds since midnight using a system call.
    - calibrate_tsc_per_sec: takes a series of measurements to then calculate the average number of cicles per second.
    
*/

#define NS_PER_SEC 1000000000ULL

static inline uint64_t read_tsc(void) {
    return sys_0p(SYS_READ_TSC);
}

static inline unsigned seconds_since_midnight(void) {
    uint64_t packed = sys_3p(SYS_TIME, 0, 0, 0);
    unsigned hour = (unsigned)((packed >> 16) & 0xFF);
    unsigned minute = (unsigned)((packed >> 8) & 0xFF);
    unsigned second = (unsigned)(packed & 0xFF);
    return (hour * 60u + minute) * 60u + second;
}

long long actualTime(void);

#endif /* USERLAND_BENCHMARKS_ACTUALTIME_H */
