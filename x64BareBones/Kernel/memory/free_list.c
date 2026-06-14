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
	if (mm == NULL || size == 0) {
		return NULL;
	}
	size = ALIGN_UP(size);
	if (size < MIN_PAYLOAD) {
		size = MIN_PAYLOAD;
	}

	// First-fit: recorro la lista física desde el primer bloque y agarro el primer libre que entre.
	fl_node_t *curr = mm->first;
	while (curr != NULL) {
		if (curr->free && curr->size >= size) {
			break;
		}
		curr = curr->next;
	}
	if (curr == NULL) {
		return NULL; // no hay bloque libre suficientemente grande
	}

	// Split: si sobra espacio para otro header + payload mínimo, parto el bloque en dos.
	if (curr->size >= size + sizeof(fl_node_t) + MIN_PAYLOAD) {
		fl_node_t *resto = (fl_node_t *) ((char *) (curr + 1) + size);
		resto->size = curr->size - size - sizeof(fl_node_t);
		resto->free = true;
		resto->next = curr->next;
		resto->prev = curr;
		if (curr->next != NULL) {
			curr->next->prev = resto;
		}
		curr->next = resto;
		curr->size = size;
	}

	curr->free = false;
	mm->allocated_blocks++;
	mm->total_allocated += curr->size;
	return (void *) (curr + 1);
}

void free_memory(memory_manager_ADT mm, void *ptr) {
	if (mm == NULL || ptr == NULL) {
		return;
	}
	fl_node_t *block = ((fl_node_t *) ptr) - 1;
	if (block->free) {
		return; // double free, ignorar
	}
	block->free = true;
	mm->allocated_blocks--;
	mm->total_allocated -= block->size;

	// Coalesce con el vecino derecho si está libre.
	fl_node_t *next = block->next;
	if (next != NULL && next->free) {
		block->size += sizeof(fl_node_t) + next->size;
		block->next = next->next;
		if (next->next != NULL) {
			next->next->prev = block;
		}
	}
	// Coalesce con el vecino izquierdo si está libre (el bloque resultante queda en prev).
	fl_node_t *prev = block->prev;
	if (prev != NULL && prev->free) {
		prev->size += sizeof(fl_node_t) + block->size;
		prev->next = block->next;
		if (block->next != NULL) {
			block->next->prev = prev;
		}
	}
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
