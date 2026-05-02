/*
 * Stub de memoria para unit tests en el host.
 * Redirige sys_malloc / sys_free a malloc / free de la libc estándar,
 * sin necesitar el buddy allocator del kernel.
 */
#include "memory_manager.h"
#include <stdlib.h>

void * sys_malloc(size_t size) { return malloc(size); }
void   sys_free(void * ptr)    { free(ptr); }

mem_info_t sys_mem_info(void) {
    mem_info_t m = {0, 0, 0, 0};
    return m;
}

memory_manager_ADT get_kernel_memory_manager(void) { return NULL; }

memory_manager_ADT create_memory_manager(void * start, size_t size) {
    (void)start; (void)size;
    return NULL;
}

void * alloc_memory(memory_manager_ADT mm, size_t size) {
    (void)mm;
    return malloc(size);
}

void free_memory(memory_manager_ADT mm, void * ptr) {
    (void)mm;
    free(ptr);
}

mem_info_t get_mem_status(memory_manager_ADT mm) {
    (void)mm;
    mem_info_t m = {0, 0, 0, 0};
    return m;
}
