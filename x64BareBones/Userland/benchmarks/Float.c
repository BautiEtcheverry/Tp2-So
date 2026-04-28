#include "../include/benchUtil.h"
#include "../include/actualTime.h"

/* 
    The following benchmark measures floating-point operation: 

    - fp_workload: Function to emulate known mathematical functions to create intensive floating-point workload
    - benchmark_float: Benchmarking function to measure performance of floating-point operations
*/

#define ITERATIONS 10UL
#define OPS        10UL
#define WARMUP     5UL

static void fp_workload(unsigned long ops) {
    double x = 0.123456789;
    for (unsigned long i = 0; i < ops; ++i) {
       
        double angle = x;
        
        const double two_pi = 6.28318530717958647693;
        const double pi = 3.14159265358979323846;
        
        while (angle > pi){angle -= two_pi;}
        while (angle < -pi){angle += two_pi;}

        /* aproximation of tringonometric and quadratic functions */
        double a2 = angle * angle;
        double sin_approx = angle - (angle * a2) / 6.0 + (angle * a2 * a2) / 120.0 - (angle * a2 * a2 * a2) / 5040.0;
        double cos_approx = 1.0 - (a2 / 2.0) + (a2 * a2) / 24.0 - (a2 * a2 * a2) / 720.0;

        x = x * 1.0000001 + sin_approx * cos_approx;

        /* Aproximation of sqrt */
        double positive = (x < 0.0) ? -x : x;
        double target = positive + 1.0;
        double guess = (target > 1.0) ? target : 1.0;

        for (int step = 0; step < 6; ++step)
            guess = 0.5 * (guess + target / guess);
        x = guess;

        /* fmod(x * 1.0001, 1000.0) */
        double scaled = x * 1.0001;
        double period = 1000.0;

        long long q = (long long)(scaled / period);
        double rem = scaled - (double)q * period;
       
        if (rem >= period){rem -= period;}
        if (rem < 0.0){ rem += period;}   
        x = rem;
    }
    volatile double sink = x;
    (void)sink;
}

int benchmark_float(void) {
    //number of times fp_workload is going to be called
    unsigned long iterations = ITERATIONS;

    //numer of times fp_workload will perform floating point operations per iteration
    unsigned long ops = OPS;

    // number of iterations to prepare the system before the benchmark test
    unsigned long warmup = WARMUP;

    printf("starting test...\n");
    
    //warmup phase
    for (unsigned long w = 0; w < warmup; ++w){fp_workload(ops);}

    //benchmarking phase
    for (unsigned long it = 0; it < iterations; ++it){
        
        long long t0 = actualTime();
        
        fp_workload(ops);
        
        long long t1 = actualTime();
        long long elapsed = t1 - t0;
        
        if (elapsed < 0){elapsed = -elapsed;}
        
        printf("iteration = ");
        bench_print_u64((unsigned long long)it);
        printf("    |   ops = ");
        bench_print_u64((unsigned long long)ops);
        printf("    |   elapsed_ns = ");
        bench_print_u64((unsigned long long)elapsed);
        printf("\n");
    }

    return 0;
}
