#include "../include/benchUtil.h"
#include "../include/actualTime.h"

/*  The following benchmark measures I/O latency, This is tested via two methods:
    
    - measure_write: this functions test the latency of write system call with zero bytes.
    - measure_kbd: this function tests the latency of checking keyboard availability. 

    Both functions repete the tests for a number of iterations defined by an default value. 
    The functions are called by benchmark_iolat. 
*/

#define ITERATIONS 10

// Simple string cmp to avoid including string.h
static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }
    return (*a == 0 && *b == 0);
}


static void measure_write(unsigned long iterations) {
    
    // Dummy variable to write
    char dummy = 0;

    for (unsigned long i = 0; i < iterations; ++i) {
    
        // Measure elapsed time between write calls
        long long t0 = actualTime();
        (void)write(1, &dummy, 0);
        long long t1 = actualTime();        
        long long elapsed = t1 - t0;
    
        if (elapsed < 0){elapsed = -elapsed;}       
        
        printf("write,");
        printf(" |   iteration = ");
        bench_print_u64((unsigned long long)i);
        printf("   |   elapsed_ns = ");
        bench_print_u64((unsigned long long)elapsed);
        printf("\n");
    }
}

static void measure_kbd(unsigned long iterations) {
    
    for (unsigned long i = 0; i < iterations; ++i) {

        // Measure elapsed time between kbd_available calls
        long long t0 = actualTime();
        (void)kbd_available();
        long long t1 = actualTime();
        long long elapsed = t1 - t0;
       
        if (elapsed < 0){elapsed = -elapsed;}
        
        printf("kbd,");
        printf(" |  iteration = ");
        bench_print_u64((unsigned long long)i);
        printf("    |  elapsed_ns = ");
        bench_print_u64((unsigned long long)elapsed);
        printf("\n");
    }
}

int benchmark_iolat(int argc, char *argv[]) {
    // Default run count and mode; mode can be overridden via the first argument.
    unsigned long iterations = ITERATIONS;
    const char *mode = NULL;

    if (argc > 1 && argv[1] && argv[1][0]) {
        mode = argv[1];
    }else{
        printf("error, Please insert \"write\" or \"kbd\" as first argument to select the test mode.\n");
        return 1;
    }

    printf("starting test...\n");

    if (str_eq(mode, "write")) {
        measure_write(iterations);
    } else if (str_eq(mode, "kbd")) {
        measure_kbd(iterations);
    }
    return 0;
}
