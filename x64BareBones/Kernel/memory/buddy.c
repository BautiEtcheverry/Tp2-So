// Memory manager con Buddy System(ref:
// https://www.geeksforgeeks.org/operating-systems/buddy-system-memory-allocation-technique/).
// La memoria se divide en bloques de tamaño 2^k bytes. Para entregar un bloque chico se van dividiendo a la mitad los
// bloques mas grandes, para dar cantidad de memoria mas parecida a lo requerido(para evitar la fragmentación
// interna[dar mas mem de la pedida]); Al liberar se intenta fusionar con su "buddy" (la otra mitad) para reconstruir
// bloques grandes.

#include "memory_manager.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MIN_ORDER 5							   // 2^5  = 32 bytes (mínimo)
#define MAX_ORDER 25						   // 2^25 = 32 MB    (máximo de bloque)
#define NUM_ORDERS (MAX_ORDER - MIN_ORDER + 1) // 21 niveles
#define INVALID_ORDER                                                                                                  \
	0xFF // Valor a retornar si el pedido no entra en ningún bloque, 0xFF=255>MAX_ORDER, no genera problemas.

// Header de cada bloque administrado. Vive al inicio del bloque mismo.
// Se usa double linked list porque remove_from_free_list necesita sacar nodos del medio de la lista en O(1) — pasa todo
// el tiempo en coalesce cuando un buddy aleatorio se libera. Con lista simple tendrías que recorrerla buscando el
// predecesor, O(n). Con prev se hace una operación de punteros y listo.
typedef struct buddy_node_t {
	struct buddy_node_t *next; // 8B-Siguiente en la lista libre
	struct buddy_node_t *prev; // 8B-Anterior en la lista libre
	bool free;				   // 1B-¿Está libre?
	uint8_t order;			   // 1B-Orden del bloque (2^order bytes)
} buddy_node_t; // Tamaño de 24 bytes, 18 del struct + 6 bytes de padding(que pone c para que quede alineado a 8)

struct memory_manager_CDT {
	void *base_address;					  // Inicio del heap administrado
	size_t total_size;					  // Bytes bajo administración
	buddy_node_t *free_lists[NUM_ORDERS]; // Listas libres por orden
	size_t allocated_blocks;			  // Bloques entregados (vivos)
	size_t total_allocated;				  // Bytes entregados (sin headers)
};

static memory_manager_ADT kernel_mm = NULL;

/*-------------------Helpers---------------------*/
// Retorna  2^order — tamaño en bytes de un bloque de orden `order`.
static size_t order_to_size(uint8_t order) {
	// left shift para mover el 1 al bit nro `order`(todo el resto queda en 0).
	return (1ULL << order); // 1ULL es una cte que representa el int 1 como long long
}

// Busca el orden mínimo que cubre `size` bytes (incluyendo el header del nodo).
// On succes retorna el orden, on failure retorna 0xFF(255)
static uint8_t size_to_order(size_t size) {
	// Hay que sumar lo que va a ocupar el header del bloque
	size += sizeof(buddy_node_t);
	uint8_t order = MIN_ORDER;
	size_t block_size = order_to_size(order);
	while (block_size < size && order < MAX_ORDER) {
		order++;
		block_size = order_to_size(order);
	}
	if (order > MAX_ORDER) {
		return INVALID_ORDER;
	}
	return order;
}
/*----------------------------------------*/

/*-------------------API---------------------*/
memory_manager_ADT create_memory_manager(void *start_address, size_t size) {
	if (start_address == NULL || size < sizeof(struct memory_manager_CDT) + (1ULL << MIN_ORDER)) {
		return NULL;
	}

	memory_manager_ADT mm = (memory_manager_ADT) start_address;
	mm->base_address = (char *) start_address + sizeof(struct memory_manager_CDT);
	mm->total_size = size - sizeof(struct memory_manager_CDT);
	mm->allocated_blocks = 0;
	mm->total_allocated = 0;
	for (int i = 0; i < NUM_ORDERS; i++) {
		mm->free_lists[i] = NULL;
	}

	// LLenar el heap con bloques del mayor orden posible.
	size_t remaining = mm->total_size;
	void *current_address = mm->base_address;
	uint8_t order = MAX_ORDER;
	size_t block_size = order_to_size(order);

	while (remaining >= (1ULL << MIN_ORDER)) {
		while (block_size > remaining && order > MIN_ORDER) {
			order--;
			block_size = order_to_size(order);
		}
		buddy_node_t *node = (buddy_node_t *) current_address;
		add_to_free_list(mm, node, order);
		current_address = (char *) current_address + block_size;
		remaining -= block_size;
	}

	kernel_mm = mm;
	return mm;
}

// Calcula la dirección del buddy haciendo XOR con el tamaño del bloque
// (flippea el bit correspondiente al orden).
static void *get_buddy_address(memory_manager_ADT mm, void *block_addr, uint8_t order) {
	// El offset es la distancia respecto de base, es decir donde empieza la mem manejada.
	size_t offset = (char *) block_addr - (char *) mm->base_address;
	size_t buddy_offset =
		offset ^ order_to_size(order); // Los offsets de dos buddies difieren de un solo bit(el del orden)

	// Ahora que tengo el offset del buddy le sumo el base y devuelvo la dirección real
	return (char *) mm->base_address + buddy_offset;
}

void *alloc_memory(memory_manager_ADT mm, size_t size) {
	if (mm == NULL || size == 0) {
		return NULL;
	}
	uint8_t order = size_to_order(size);
	if (order > MAX_ORDER) {
		return NULL;
	}
	uint8_t index = order - MIN_ORDER;

	if (mm->free_lists[index] == NULL && order < MAX_ORDER) {
		if (split_block(mm, order + 1) == NULL) {
			return NULL;
		}
	}
	if (mm->free_lists[index] == NULL) {
		return NULL;
	}

	buddy_node_t *block = mm->free_lists[index];
	remove_from_free_list(mm, block, order);
	block->free = false;
	block->order = order;

	mm->allocated_blocks++;
	mm->total_allocated += order_to_size(order) - sizeof(buddy_node_t);

	return (char *) block + sizeof(buddy_node_t);
}

void free_memory(memory_manager_ADT mm, void *ptr) {
	if (mm == NULL || ptr == NULL) {
		return;
	}
	buddy_node_t *block = (buddy_node_t *) ((char *) ptr - sizeof(buddy_node_t));
	if (block->free) {
		return; // Double free — ignorar
	}
	block->free = true;
	mm->allocated_blocks--;
	mm->total_allocated -= order_to_size(block->order) - sizeof(buddy_node_t);

	add_to_free_list(mm, block, block->order);
	coalesce(mm, block);
}
mem_info_t get_mem_status(memory_manager_ADT mm) {
	mem_info_t status = {0};
	if (mm != NULL) {
		status.total_memory = mm->total_size;
		status.used_memory = mm->total_allocated;
		status.free_memory = status.total_memory - status.used_memory;
		status.allocated_blocks = mm->allocated_blocks;
	}
	return status;
}
/*----------------------------------------*/

/*-------------------Ops sobre los bloques---------------------*/
// Divide un bloque de orden `order` en dos buddies de orden `order-1`.
// Si no hay bloques de ese orden, intenta conseguir uno dividiendo recursivamente uno más grande.
static buddy_node_t *split_block(memory_manager_ADT mm, uint8_t order) {
	if (order == MIN_ORDER || order > MAX_ORDER) {
		return NULL;
	}
	uint8_t index = order - MIN_ORDER;

	if (mm->free_lists[index] == NULL && order < MAX_ORDER) {
		// Busco dividir bloques mas grandes recursivamente
		if (split_block(mm, order + 1) == NULL) {
			return NULL;
		}
	}
	if (mm->free_lists[index] == NULL) {
		return NULL;
	}

	buddy_node_t *block = mm->free_lists[index];
	remove_from_free_list(mm, block, order);

	uint8_t new_order = order - 1;
	size_t half_size = order_to_size(new_order);

	buddy_node_t *buddy1 = block;
	buddy_node_t *buddy2 = (buddy_node_t *) ((char *) block + half_size);

	add_to_free_list(mm, buddy1, new_order);
	add_to_free_list(mm, buddy2, new_order);
	return buddy1;
}

// Fusiona un bloque libre con su buddy si ambos están libres, recursivamente hacia arriba.
static void coalesce(memory_manager_ADT mm, buddy_node_t *block) {
	uint8_t order = block->order;
	if (order >= MAX_ORDER) {
		return;
	}
	buddy_node_t *buddy = (buddy_node_t *) get_buddy_address(mm, block, order);
	if (!buddy->free || buddy->order != order) {
		return;
	}

	remove_from_free_list(mm, block, order);
	remove_from_free_list(mm, buddy, order);

	// El bloque con dirección más baja queda como representante del fusionado
	buddy_node_t *merged = (block < buddy) ? block : buddy;
	add_to_free_list(mm, merged, order + 1);
	coalesce(mm, merged);
}
/*----------------------------------------*/

/*-------------------Ops sobre free_lists---------------------*/
// Saca un nodo de su lista libre.
static void remove_from_free_list(memory_manager_ADT mm, buddy_node_t *node, uint8_t order) {
	uint8_t index = order - MIN_ORDER;
	if (node->prev) {
		node->prev->next = node->next;
	}
	else {
		mm->free_lists[index] = node->next;
	}
	if (node->next) {
		node->next->prev = node->prev;
	}
	node->next = NULL;
	node->prev = NULL;
}

// Mete un nodo al inicio de la lista libre del orden indicado.
static void add_to_free_list(memory_manager_ADT mm, buddy_node_t *node, uint8_t order) {
	uint8_t index = order - MIN_ORDER;
	node->next = mm->free_lists[index];
	node->prev = NULL;
	node->order = order;
	node->free = true;
	if (mm->free_lists[index]) {
		mm->free_lists[index]->prev = node;
	}
	mm->free_lists[index] = node;
}
/*----------------------------------------*/

/*-------------------Wrappers---------------------*/
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
/*----------------------------------------*/
