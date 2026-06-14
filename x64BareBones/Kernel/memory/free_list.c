// Memory manager con Free List explícita + first-fit + coalescing.
// Mantenemos una lista doblemente enlazada en orden físico de TODOS los bloques (libres y ocupados).
// El recorrido es lineal por la lista; coalesce se hace en O(1) mirando prev y next del bloque liberado.
// Cada bloque vive con su header al principio; el usuario recibe el puntero al payload (header + 1).

#include "memory_manager.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT 8
#define ALIGN_UP(x) (((x) + (ALIGNMENT - 1)) & ~((size_t) (ALIGNMENT - 1)))
#define MIN_PAYLOAD 8 // payload mínimo de un bloque tras split (para no dejar headers sin uso)

// Header de cada bloque. Enlazado en orden físico (next/prev son los vecinos en memoria).
typedef struct fl_node_t {
	size_t size;			 // bytes de payload (sin contar este header)
	bool free;				 // ¿está libre?
	struct fl_node_t *next;	 // siguiente bloque en memoria (NULL si es el último)
	struct fl_node_t *prev;	 // anterior bloque en memoria (NULL si es el primero)
} fl_node_t;

struct memory_manager_CDT {
	void *base_address;		 // inicio del primer bloque
	size_t total_size;		 // bytes bajo administración (sin contar el CDT)
	fl_node_t *first;		 // primer bloque (cabeza de la lista física)
	size_t allocated_blocks; // bloques entregados vivos
	size_t total_allocated;	 // bytes de payload entregados
};

static memory_manager_ADT kernel_mm = NULL;

memory_manager_ADT create_memory_manager(void *start_address, size_t size) {
	if (start_address == NULL || size < sizeof(struct memory_manager_CDT) + sizeof(fl_node_t) + MIN_PAYLOAD) {
		return NULL;
	}

	memory_manager_ADT mm = (memory_manager_ADT) start_address;
	mm->base_address = (char *) start_address + sizeof(struct memory_manager_CDT);
	mm->total_size = size - sizeof(struct memory_manager_CDT);
	mm->allocated_blocks = 0;
	mm->total_allocated = 0;

	// Único bloque libre inicial que cubre todo el heap administrado.
	fl_node_t *head = (fl_node_t *) mm->base_address;
	head->size = mm->total_size - sizeof(fl_node_t);
	head->free = true;
	head->next = NULL;
	head->prev = NULL;
	mm->first = head;

	kernel_mm = mm;
	return mm;
}

void *alloc_memory(memory_manager_ADT mm, size_t size) {
	(void) mm;
	(void) size;
	return NULL; // se implementa en el próximo commit
}

void free_memory(memory_manager_ADT mm, void *ptr) {
	(void) mm;
	(void) ptr;
}

mem_info_t get_mem_status(memory_manager_ADT mm) {
	mem_info_t status = {0};
	(void) mm;
	return status;
}

memory_manager_ADT get_kernel_memory_manager(void) {
	return kernel_mm;
}

void *sys_malloc(size_t size) {
	return alloc_memory(kernel_mm, size);
}

void sys_free(void *ptr) {
	free_memory(kernel_mm, ptr);
}

mem_info_t sys_mem_info(void) {
	return get_mem_status(kernel_mm);
}
