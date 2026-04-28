#include "../include/benchUtil.h"
#include "../include/actualTime.h"

/* 
    The following benchmark measures memory latency using pointer-chasing technique.
    
    - build_chain: Constructs an array of pointers with a specified stride to create a pointer-chase pattern.
    - run_chain: Follows the pointer chain for a given number of iterations to measure memory access latency.
    - benchmark_memlat: benchmarking function that sets up the test parameters and runs the test.

*/

// Space reserved for structure allocations
#define ALLOC_SIZE (4 * 1024 * 1024)

static unsigned char bench_pool[ALLOC_SIZE];
static size_t bench_pool_used = 0;

// Allocator for benchmark structures
static void *bench_alloc(size_t bytes) {
    if (bench_pool_used + bytes > ALLOC_SIZE)
        return (void *)0;
    void *ptr = &bench_pool[bench_pool_used];
    bench_pool_used += bytes;
    return ptr;
}

static void bench_reset_alloc(void) {
    bench_pool_used = 0;
}

// Builds an array in which each element points to the next element in the chain
size_t build_chain(size_t n_elems, size_t stride, size_t **arr_out) {
    size_t size = n_elems;
    size_t *arr = (size_t *)bench_alloc(sizeof(size_t) * size);
    if (!arr) return 0;
    for (size_t i = 0; i < size; ++i) arr[i] = (i + stride) % size;
    *arr_out = arr;
    return size;
}

// Runs the chain for a specified number of iterations
void run_chain(size_t *arr, size_t size, size_t iterations) {
    
    size_t idx = 0;
    
    for (size_t i = 0; i < iterations; ++i) {
        idx = arr[idx];
    }
    
    // volatile prefix prevents compiler optimization
    volatile size_t sink = idx;
    (void)sink;
}

int benchmark_memlat(int argc, char **argv) {
    
    //number of times the chain will be run
    unsigned long iterations = 1000000;

    size_t size_kb = 64;
    // delta distance between the actual position and the position that the pointer stores
    size_t stride = 16; 

    //Calculates how many elements are needed to fill size_kb kilobytes
    size_t n_elems = (size_kb * 1024) / sizeof(size_t);
    bench_reset_alloc();

    size_t *chain = NULL;
    if (build_chain(n_elems, stride, &chain) == 0 || !chain) {
        printf("error,0,0,0\n");
        return 1;
    }

    printf("starting test...\n");

    // number of times it will run the whole benchmark
    size_t runs = 10;

    for (size_t r = 0; r < runs; ++r) {
    
        // Measure elapsed time of run_chain
        long long t0 = actualTime();
        run_chain(chain, n_elems, iterations);
        long long t1 = actualTime();
        long long elapsed = t1 - t0;
    
        if (elapsed < 0){elapsed = -elapsed;}
        
        printf("size = ");
        bench_print_u64((unsigned long long)size_kb);
        printf("    |   stride = ");
        bench_print_u64((unsigned long long)stride);
        printf("    |   iteration = ");
        bench_print_u64((unsigned long long)r);
        printf("    |   elapsed_ns = ");
        bench_print_u64((unsigned long long)elapsed);
        printf("\n");
    }
    return 0;
}
