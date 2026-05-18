#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stddef.h>
#include <stdint.h>

/*  TAD - Tipo abstracto de dato(ADT en ingles)
    CDT - Conrecte data type
    El TAD es un puntero al CDT implementado internamente, permite esconder la estructura.
    Sólo una estructura se compila, elegida con #ifdef en el Makefile(para elegir el MM).
*/
typedef struct memory_manager_CDT * memory_manager_ADT;

// Información de estado para la syscall mem_info y el comando `mem`.
typedef struct {
    size_t total_memory;     // Bytes totales administrados
    size_t used_memory;      // Bytes ocupados
    size_t free_memory;      // Bytes libres
    size_t allocated_blocks; // Bloques allocados vivos
} mem_info_t;

// Crea un MM en `start_address` que administra `size` bytes desde ahí.
// El CDT vive al inicio del bloque; el heap arranca después del CDT.
// Setea internamente el MM del kernel para que sys_malloc/sys_free lo usen.
memory_manager_ADT create_memory_manager(void *start_address, size_t size);

// API(Aplication programming interface) del allocator(la comparten los dos MM).
void       *alloc_memory(memory_manager_ADT mm, size_t size);
void        free_memory(memory_manager_ADT mm, void *ptr);
mem_info_t  get_mem_status(memory_manager_ADT mm);

// Acceso al MM del kernel (lo setea create_memory_manager). 
memory_manager_ADT get_kernel_memory_manager(void);

// Funciones que usa directamente el dispatcher de syscalls.
void       *sys_malloc(size_t size);
void        sys_free(void *ptr);
mem_info_t  sys_mem_info(void);

#endif 
