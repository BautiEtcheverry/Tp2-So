#include "../include/actualTime.h"

static uint64_t tsc_per_sec = 0;
static uint64_t tsc_origin = 0;
static uint64_t ticks_to_ns_fixed = 0;

#define FIXED_POINT_SCALE 1000000ULL 

/*calculates the amount of cicles per second using tsc, does the operation 3 times and takes the avarage*/
static uint64_t calibrate_tsc_per_sec(void) {
    const int samples = 3;
    uint64_t total = 0;
    
    for (int i = 0; i < samples; ++i) { 
        unsigned prev = seconds_since_midnight();
        unsigned curr = prev;
        
        while ((curr = seconds_since_midnight()) == prev) {
            for (volatile int j = 0; j < 100; j++);
        }
        
        uint64_t t0 = read_tsc();
        
        prev = curr;
        while ((curr = seconds_since_midnight()) == prev) {
            for (volatile int j = 0; j < 100; j++);
        }
        
        uint64_t t1 = read_tsc();
        
        uint64_t delta = t1 - t0;
        if (delta > 1000000ULL && delta < 10000000000ULL) {
            total += delta;
        } else {
            i--;
        }
    }
    
    uint64_t avg = total / samples;
    return avg ? avg : 2000000000ULL;
}

//returns the actual time in nanoseconds since origin
long long actualTime(void) {
    if (tsc_per_sec == 0) {
        tsc_per_sec = calibrate_tsc_per_sec();
        if (tsc_per_sec == 0)
            tsc_per_sec = 2000000000ULL;
        
        tsc_origin = read_tsc();
        ticks_to_ns_fixed = (NS_PER_SEC * FIXED_POINT_SCALE) / tsc_per_sec;
    }
    
    uint64_t delta = read_tsc() - tsc_origin; 
    uint64_t ns_fixed = (delta * ticks_to_ns_fixed) / FIXED_POINT_SCALE;
    
    if (ns_fixed > 9220000000000000000ULL) {
        ns_fixed = 9220000000000000000ULL;
    }
    
    return (long long)ns_fixed;
}