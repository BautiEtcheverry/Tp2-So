# TP2 — Guía de Pasos para la Construcción del Kernel
## Partiendo del repo real: `x64BareBones/`

> Este documento describe el orden de desarrollo para el TP2 tomando como base el estado **exacto** del repo actual. Cada paso indica qué ya existe `[EXISTE]`, qué se modifica `[MODIFICA]` y qué se crea desde cero `[NUEVO]`. Los unit tests (CuTest) viven en `x64BareBones/Tests/`. Los tests de integración se corren dentro del kernel como comandos de la shell.

---

## Estado real del repo (base para este documento)

| Archivo | Descripción | Estado para TP2 |
|---|---|---|
| `x64BareBones/Kernel/kernel.c` | Entry point: carga módulos, inicia IRQs, salta a shell | `[MODIFICA]` — agregar init MM y scheduler |
| `x64BareBones/Kernel/syscall.c` | Dispatcher de 24 syscalls (I/O, video, audio, RTC, TSC) | `[MODIFICA]` — agregar nuevas syscalls |
| `x64BareBones/Kernel/include/syscall.h` | Números de syscall 1–24 | `[MODIFICA]` — agregar nuevos números desde 25 |
| `x64BareBones/Kernel/asm/idt.asm` | IDT: gates para 0x80 (syscall), 0x21 (teclado IRQ1), #DE, #UD | `[MODIFICA]` — agregar IRQ0 (timer) para el scheduler |
| `x64BareBones/Kernel/asm/libasm.asm` | cpuVendor, isr_syscall_80, inb/outb, picMasterMask, sti | `[EXISTE]` — no se toca |
| `x64BareBones/Kernel/kernel.ld` | Linker script — ya define `endOfKernel` y `endOfKernelBinary` | `[EXISTE]` — no se toca |
| `x64BareBones/Kernel/lib.c` | memset, memcpy, memmove, memcmp | `[EXISTE]` — no se toca |
| `x64BareBones/Kernel/cdrivers/keyboard.c/h` | Driver de teclado, IRQ1 | `[EXISTE]` — no se toca |
| `x64BareBones/Kernel/cdrivers/videoDriver.c/h` | Driver de video LFB | `[EXISTE]` — no se toca |
| `x64BareBones/Kernel/cdrivers/gfxConsole.c/h` | Consola gráfica sobre LFB | `[EXISTE]` — no se toca |
| `x64BareBones/Userland/asm/syscalls.asm` | Stubs: sys_0p, sys_1p, sys_3p, sys_raw (int 0x80) | `[EXISTE]` — no se toca |
| `x64BareBones/Userland/include/libc.h` | Wrappers C de syscalls para userland | `[MODIFICA]` — agregar nuevas syscalls |
| `x64BareBones/Userland/Shell/shell.c` | Shell actual con comandos: help, ls, echo, time, clear, regs, etc. | `[MODIFICA]` — reemplazar por shell TP2 |
| `x64BareBones/Userland/Shell/_loader.c` | Entry point del módulo userland, limpia BSS, llama a `main()` | `[EXISTE]` — no se toca |
| `x64BareBones/Userland/Shell/libc.c` | printf, scanf, utilidades de userland | `[EXISTE]` — no se toca |

**Lo que NO existe y hay que construir:**
- Memory Manager (ninguno de los dos)
- IRQ0 handler (timer) — necesario para context switching
- Procesos, context switching, scheduler
- Semáforos
- Pipes y file descriptors
- Shell TP2 (`sh`) con foreground/background/pipes
- Todos los comandos del TP2 (`ps`, `kill`, `nice`, `loop`, `cat`, `wc`, `filter`, `mvar`, etc.)
- Tests unitarios (carpeta `Tests/` entera)

---

## Estructura de Carpetas — Estado Final del TP2

```
x64BareBones/
│
├── Bootloader/                                [EXISTE]
│
├── Kernel/
│   ├── kernel.ld                              [EXISTE] ya tiene endOfKernel y endOfKernelBinary
│   ├── Makefile                               [MODIFICA] agregar nuevos subdirectorios
│   ├── Makefile.inc                           [EXISTE]
│   │
│   ├── asm/
│   │   ├── idt.asm                            [MODIFICA] agregar IRQ0 handler para el scheduler
│   │   ├── libasm.asm                         [EXISTE]
│   │   └── loader.asm                         [EXISTE]
│   │
│   ├── include/
│   │   ├── syscall.h                          [MODIFICA] agregar números desde 25
│   │   ├── lib.h                              [EXISTE]
│   │   ├── libasm.h                           [EXISTE]
│   │   ├── naiveConsole.h                     [EXISTE]
│   │   ├── moduleLoader.h                     [EXISTE]
│   │   ├── keyboard.h                         [EXISTE]
│   │   ├── videoDriver.h                      [EXISTE]
│   │   ├── gfxConsole.h                       [EXISTE]
│   │   ├── memoryManager.h                    [NUEVO]
│   │   ├── process.h                          [NUEVO]
│   │   ├── scheduler.h                        [NUEVO]
│   │   ├── semaphore.h                        [NUEVO]
│   │   └── pipe.h                             [NUEVO]
│   │
│   ├── memory/
│   │   ├── dummyMM.c                          [NUEVO] bump allocator — Paso 2
│   │   └── buddyMM.c                          [NUEVO] buddy system — Paso 9
│   │
│   ├── processes/
│   │   ├── process.c                          [NUEVO]
│   │   └── scheduler.c                        [NUEVO]
│   │
│   ├── sync/
│   │   └── semaphore.c                        [NUEVO]
│   │
│   ├── ipc/
│   │   └── pipe.c                             [NUEVO]
│   │
│   ├── kernel.c                               [MODIFICA] init MM + init scheduler + idle process
│   ├── syscall.c                              [MODIFICA] agregar nuevas syscalls
│   ├── cdrivers/                              [EXISTE]
│   ├── lib.c                                  [EXISTE]
│   ├── exceptions.c                           [EXISTE]
│   ├── moduleLoader.c                         [EXISTE]
│   └── naiveConsole.c                         [EXISTE]
│
├── Userland/
│   ├── Makefile                               [MODIFICA] agregar compilación de apps
│   ├── Makefile.inc                           [EXISTE]
│   │
│   ├── asm/
│   │   └── syscalls.asm                       [EXISTE] no se toca
│   │
│   ├── include/
│   │   └── libc.h                             [MODIFICA] agregar wrappers de nuevas syscalls
│   │
│   ├── Shell/
│   │   ├── Makefile                           [MODIFICA] compilar también apps/
│   │   ├── shell.c                            [MODIFICA] reemplazar por shell TP2
│   │   ├── _loader.c                          [EXISTE]
│   │   ├── libc.c                             [EXISTE]
│   │   └── shell.ld                           [EXISTE]
│   │
│   └── apps/
│       ├── help.c                             [NUEVO]
│       ├── mem.c                              [NUEVO]
│       ├── ps.c                               [NUEVO]
│       ├── loop.c                             [NUEVO]
│       ├── kill.c                             [NUEVO]
│       ├── nice.c                             [NUEVO]
│       ├── block.c                            [NUEVO]
│       ├── cat.c                              [NUEVO]
│       ├── wc.c                               [NUEVO]
│       ├── filter.c                           [NUEVO]
│       ├── mvar.c                             [NUEVO]
│       ├── test_mm.c                          [NUEVO] test cátedra
│       ├── test_proc.c                        [NUEVO] test cátedra
│       └── test_sync.c                        [NUEVO] test cátedra
│
├── Tests/                                     [NUEVO] unit tests — corren en host, sin kernel
│   ├── Makefile                               [NUEVO]
│   ├── CuTest.c                               [NUEVO]
│   ├── CuTest.h                               [NUEVO]
│   ├── AllTests.c                             [NUEVO]
│   ├── memory/
│   │   ├── testDummyMM.c                      [NUEVO]
│   │   └── testBuddyMM.c                      [NUEVO]
│   ├── processes/
│   │   └── testScheduler.c                    [NUEVO]
│   └── sync/
│       └── testSemaphore.c                    [NUEVO]
│
├── Image/                                     [EXISTE]
├── Bootloader/                                [EXISTE]
├── Toolchain/                                 [EXISTE]
└── Makefile                                   [MODIFICA] agregar regla buddy
```

### Nota sobre la arquitectura de userland

Todos los apps del TP2 (`help`, `ps`, `kill`, etc.) se compilan **dentro del mismo binario** que la shell (`0000-shell.bin`). La shell mantiene una tabla de funciones por nombre y el kernel los instancia como procesos independientes con su propio stack (alocado vía `mallocMM`). Este es el patrón estándar de las implementaciones estudiantiles de este TP.

---

## PASO 1 — Preparar el build system para el TP2

### ¿Qué se hace?
Agregar los nuevos números de syscall en `Kernel/include/syscall.h`, actualizar el `Kernel/Makefile` para compilar los nuevos subdirectorios (`memory/`, `processes/`, `sync/`, `ipc/`), agregar la regla `buddy` en el `Makefile` raíz, y agregar la estructura `Userland/apps/`.

### Archivos modificados

```
x64BareBones/Kernel/include/syscall.h    [MODIFICA]
x64BareBones/Kernel/Makefile             [MODIFICA]
x64BareBones/Makefile                    [MODIFICA]
x64BareBones/Userland/Shell/Makefile     [MODIFICA]
```

### Agregar números de syscall en `Kernel/include/syscall.h`

El repo tiene syscalls 1–24. **No tocar ninguna existente.** Agregar desde 25:

```c
// ── Syscalls nuevas del TP2 ─────────────────────────────────────────────────
// Los existentes llegan hasta 24 (SYS_READ_TSC)
#define SYS_MALLOC       25
#define SYS_FREE         26
#define SYS_MEM_STATE    27
#define SYS_GET_PID      28
#define SYS_CREATE_PROC  29
#define SYS_EXIT_PROC    30   // terminar el proceso actual (≠ SYS_EXIT=4 que no está en kernel)
#define SYS_KILL         31
#define SYS_BLOCK        32
#define SYS_UNBLOCK      33
#define SYS_YIELD        34
#define SYS_WAITPID      35
#define SYS_NICE         36
#define SYS_PS           37
#define SYS_SEM_OPEN     38
#define SYS_SEM_CLOSE    39
#define SYS_SEM_WAIT     40
#define SYS_SEM_POST     41
#define SYS_PIPE_CREATE  42
#define SYS_PIPE_OPEN    43
#define SYS_PIPE_READ    44
#define SYS_PIPE_WRITE   45
#define SYS_PIPE_CLOSE   46
```

### Actualizar `Kernel/Makefile` para compilar subdirectorios nuevos

Reemplazar la línea de `SOURCES` por:

```makefile
SOURCES=$(wildcard *.c cdrivers/*.c memory/*.c processes/*.c sync/*.c ipc/*.c)
```

### Agregar regla `buddy` al `Makefile` raíz (`x64BareBones/Makefile`)

```makefile
buddy:
	cd Kernel && make buddy
	cd Userland && make all
	cd Image && make all
```

Y en `Kernel/Makefile` agregar la regla `buddy`:

```makefile
buddy: GCCFLAGS += -DUSE_BUDDY
buddy: $(KERNEL)
```

Esto pasa el flag `-DUSE_BUDDY` al compilador, que permite usar `#ifdef USE_BUDDY` en el código para elegir la implementación.

### Actualizar `Userland/Shell/Makefile` para incluir `apps/`

```makefile
SOURCES=$(wildcard [^_]*.c) $(wildcard ../apps/*.c)
```

Esto compila todos los archivos de `Userland/apps/` junto con la shell en el mismo binario `0000-shell.bin`.

### Verificación

```bash
./dev.sh   # entrar al container
make       # debe compilar sin errores — el kernel bootea normalmente
# La shell existente debe seguir funcionando (comandos: help, echo, time, etc.)
```

---

## PASO 2 — Memory Manager Dummy (Bump Allocator)

### ¿Qué se hace?
Se implementa el memory manager más simple posible. Es suficiente para que el kernel funcione y desbloquea todos los pasos siguientes.

**Puntos críticos:**
- `kernel.ld` ya expone `endOfKernel` — no hay que tocar nada en el linker script.
- La estructura del MM vive justo después del kernel (en `&endOfKernel`). La heap administrada empieza después del CDT.
- La interfaz definida acá es el **contrato exacto** que el Buddy System también debe respetar.

### Archivos nuevos/modificados

```
Kernel/include/memoryManager.h       [NUEVO]
Kernel/memory/dummyMM.c             [NUEVO]
Kernel/kernel.c                      [MODIFICA] agregar initKernelMemory()
Kernel/syscall.c                     [MODIFICA] casos SYS_MALLOC, SYS_FREE, SYS_MEM_STATE
Userland/include/libc.h             [MODIFICA] agregar mem_alloc, mem_free, mem_state
Tests/CuTest.c                       [NUEVO]
Tests/CuTest.h                       [NUEVO]
Tests/AllTests.c                     [NUEVO]
Tests/Makefile                       [NUEVO]
Tests/memory/testDummyMM.c          [NUEVO]
```

### `Kernel/include/memoryManager.h`

```c
#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>

typedef struct MemoryManagerCDT * MemoryManagerADT;

MemoryManagerADT createMemoryManager(void * restrict memoryForMM,
                                     void * restrict managedMemory,
                                     uint64_t managedMemorySize);

void *   mallocMM(MemoryManagerADT mm, size_t size);
void     freeMM(MemoryManagerADT mm, void * ptr);
uint64_t getTotalMemory(MemoryManagerADT mm);
uint64_t getFreeMemory(MemoryManagerADT mm);
uint64_t getUsedMemory(MemoryManagerADT mm);

#endif
```

### `Kernel/memory/dummyMM.c`

```c
#include "memoryManager.h"

typedef struct MemoryManagerCDT {
    char *   nextAddress;
    char *   memoryStart;
    char *   memoryEnd;
    uint64_t usedMemory;
} MemoryManagerCDT;

MemoryManagerADT createMemoryManager(void * restrict memoryForMM,
                                     void * restrict managedMemory,
                                     uint64_t managedMemorySize) {
    MemoryManagerADT mm = (MemoryManagerADT) memoryForMM;
    mm->nextAddress = (char *) managedMemory;
    mm->memoryStart = (char *) managedMemory;
    mm->memoryEnd   = (char *) managedMemory + managedMemorySize;
    mm->usedMemory  = 0;
    return mm;
}

void * mallocMM(MemoryManagerADT mm, size_t size) {
    size = (size + 7) & ~7;  // alinear a 8 bytes
    if (mm->nextAddress + size > mm->memoryEnd) return NULL;
    void * allocation  = mm->nextAddress;
    mm->nextAddress   += size;
    mm->usedMemory    += size;
    return allocation;
}

void freeMM(MemoryManagerADT mm, void * ptr) {
    (void)mm; (void)ptr;  // stub — el dummy no libera
}

uint64_t getTotalMemory(MemoryManagerADT mm) {
    return (uint64_t)(mm->memoryEnd - mm->memoryStart);
}
uint64_t getUsedMemory(MemoryManagerADT mm) { return mm->usedMemory; }
uint64_t getFreeMemory(MemoryManagerADT mm) {
    return getTotalMemory(mm) - mm->usedMemory;
}
```

### Modificar `Kernel/kernel.c`

```c
#include "include/memoryManager.h"

extern uint8_t endOfKernel;
static uint32_t * const ramAmountMiB = (uint32_t *) 0x5020; // Pure64 RAMAMOUNT

static MemoryManagerADT kernelMM;

void initKernelMemory() {
    void *   mmStruct  = (void *) &endOfKernel;
    void *   heapStart = (char *) mmStruct + sizeof(MemoryManagerCDT);
    uint64_t ramBytes  = (uint64_t)(*ramAmountMiB) * 1024 * 1024;
    uint64_t heapSize  = ramBytes - (uint64_t) heapStart;
    kernelMM = createMemoryManager(mmStruct, heapStart, heapSize);
}

MemoryManagerADT getKernelMM() { return kernelMM; }
```

Llamar a `initKernelMemory()` **después** de `clearBSS` en `initializeKernelBinary()`.

### Modificar `Kernel/syscall.c`

```c
// Incluir al principio:
#include "include/memoryManager.h"
extern MemoryManagerADT getKernelMM();

// Agregar al switch en syscall_dispatch():
case SYS_MALLOC:
    return (uint64_t) mallocMM(getKernelMM(), (size_t) a1);
case SYS_FREE:
    freeMM(getKernelMM(), (void *) a1);
    return 0;
case SYS_MEM_STATE: {
    uint64_t * out = (uint64_t *) a1;
    out[0] = getTotalMemory(getKernelMM());
    out[1] = getUsedMemory(getKernelMM());
    out[2] = getFreeMemory(getKernelMM());
    return 0;
}
```

### Agregar wrappers en `Userland/include/libc.h`

```c
// Agregar al enum:
SYS_MALLOC      = 25,
SYS_FREE        = 26,
SYS_MEM_STATE   = 27,
// ... resto de nuevas

static inline void * mem_alloc(size_t size) {
    return (void *) sys_1p(SYS_MALLOC, (uint64_t) size);
}
static inline void mem_free(void * ptr) {
    sys_1p(SYS_FREE, (uint64_t) ptr);
}
static inline void mem_state(uint64_t * total, uint64_t * used, uint64_t * free) {
    uint64_t buf[3];
    sys_1p(SYS_MEM_STATE, (uint64_t) buf);
    *total = buf[0]; *used = buf[1]; *free = buf[2];
}
```

### `Tests/Makefile`

```makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -g -I../Kernel/include -I../Kernel/memory -I.

SOURCES = AllTests.c CuTest.c \
          memory/testDummyMM.c \
          ../Kernel/memory/dummyMM.c

TARGET  = runTests

all: $(TARGET)
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
```

### `Tests/memory/testDummyMM.c`

```c
#include "../CuTest.h"
#include "memoryManager.h"
#include <string.h>
#include <stdint.h>

#define TEST_HEAP_SIZE (1024 * 1024)

static char mmBuffer[64];
static char heapBuffer[TEST_HEAP_SIZE];

static MemoryManagerADT setupMM() {
    return createMemoryManager(mmBuffer, heapBuffer, TEST_HEAP_SIZE);
}

void testAllocReturnsNonNull(CuTest * tc) {
    CuAssertPtrNotNull(tc, mallocMM(setupMM(), 128));
}

void testAllocsDoNotOverlap(CuTest * tc) {
    MemoryManagerADT mm = setupMM();
    char * p1 = mallocMM(mm, 256);
    char * p2 = mallocMM(mm, 256);
    CuAssertTrue(tc, p2 >= p1 + 256);
}

void testAllocBeyondHeapReturnsNull(CuTest * tc) {
    CuAssertTrue(tc, mallocMM(setupMM(), TEST_HEAP_SIZE + 1) == NULL);
}

void testStatsCoherent(CuTest * tc) {
    MemoryManagerADT mm = setupMM();
    mallocMM(mm, 512);
    CuAssertIntEquals(tc,
        (int) getTotalMemory(mm),
        (int)(getFreeMemory(mm) + getUsedMemory(mm)));
}

void testAllocAlignedTo8(CuTest * tc) {
    MemoryManagerADT mm = setupMM();
    CuAssertTrue(tc, ((uintptr_t) mallocMM(mm, 1)) % 8 == 0);
    CuAssertTrue(tc, ((uintptr_t) mallocMM(mm, 3)) % 8 == 0);
    CuAssertTrue(tc, ((uintptr_t) mallocMM(mm, 7)) % 8 == 0);
}

void testAllocatedMemoryIsWritable(CuTest * tc) {
    MemoryManagerADT mm = setupMM();
    char * buf = mallocMM(mm, 64);
    CuAssertPtrNotNull(tc, buf);
    memset(buf, 0xAB, 64);
    CuAssertTrue(tc, (unsigned char) buf[0]  == 0xAB);
    CuAssertTrue(tc, (unsigned char) buf[63] == 0xAB);
}

void testMultipleAllocsTrackUsed(CuTest * tc) {
    MemoryManagerADT mm = setupMM();
    mallocMM(mm, 100);
    mallocMM(mm, 200);
    CuAssertTrue(tc, getUsedMemory(mm) >= 300);
}

CuSuite * getDummyMMTestSuite() {
    CuSuite * suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, testAllocReturnsNonNull);
    SUITE_ADD_TEST(suite, testAllocsDoNotOverlap);
    SUITE_ADD_TEST(suite, testAllocBeyondHeapReturnsNull);
    SUITE_ADD_TEST(suite, testStatsCoherent);
    SUITE_ADD_TEST(suite, testAllocAlignedTo8);
    SUITE_ADD_TEST(suite, testAllocatedMemoryIsWritable);
    SUITE_ADD_TEST(suite, testMultipleAllocsTrackUsed);
    return suite;
}
```

### Cómo correr los tests

```bash
# En el host (no en el container — el container es solo para compilar el kernel)
cd x64BareBones/Tests && make run
# Salida esperada: OK (7 tests)
```

### Verificación en el kernel

```bash
./dev.sh   # container
make && ./run.sh
# Agregar un comando temporal en shell.c que llame a mem_state e imprima los valores
# Debe mostrar: total > 0, free > 0, used == 0 al inicio
# Más adelante esto se convierte en el comando oficial `mem`
```

---

## PASO 3 — Context Switching y Scheduler Round Robin

### ¿Qué se hace?
Se implementa el **context switching** y el **scheduler Round Robin con prioridades**. El IRQ0 del timer (que actualmente está **maskeado** — `picMasterMask(0xF9)` no lo desmaskea) pasa a disparar el cambio de proceso.

Reglas críticas:
- Proceso **bloqueado = cero quantums**. Si 18 de 20 procesos están bloqueados, el CPU se divide solo entre los 2 activos.
- **Round Robin simple** — correctitud sobre sofisticación.
- Necesita `mallocMM` para alocar PCBs — por eso el Paso 2 va antes.

### Archivos nuevos/modificados

```
Kernel/include/process.h              [NUEVO]
Kernel/include/scheduler.h            [NUEVO]
Kernel/processes/process.c            [NUEVO]
Kernel/processes/scheduler.c          [NUEVO]
Kernel/asm/idt.asm                    [MODIFICA] agregar _irq00Handler, instalar gate 0x20
Kernel/kernel.c                       [MODIFICA] desmascarear IRQ0, crear proceso idle
Kernel/syscall.c                      [MODIFICA] agregar syscalls de procesos
Userland/include/libc.h              [MODIFICA]
Tests/processes/testScheduler.c       [NUEVO]
Tests/AllTests.c                      [MODIFICA]
Tests/Makefile                        [MODIFICA]
```

### `Kernel/include/process.h`

```c
#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

typedef enum { READY, RUNNING, BLOCKED, DEAD } ProcessState;

typedef struct PCB {
    uint64_t      pid;
    char          name[64];
    uint64_t      rsp;        // stack pointer guardado en context switch
    uint64_t      rbp;
    uint64_t      stackBase;
    uint64_t      stackSize;
    ProcessState  state;
    int           priority;   // 0 = máxima, mayor número = menor prioridad
    int           foreground; // 1 si es foreground de la shell
    int           fd[2];      // fd[0]=stdin fd[1]=stdout — para pipes (Paso 6)
    struct PCB  * next;
} PCB;

typedef int (*ProcessMain)(int argc, char ** argv);

PCB * createProcess(const char * name, ProcessMain entry,
                    int argc, char ** argv,
                    int priority, int foreground);
void  destroyProcess(PCB * pcb);

#endif
```

### `Kernel/include/scheduler.h`

```c
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"
#include <stdint.h>

void     initScheduler(PCB * idleProcess);
void     addProcess(PCB * pcb);
uint64_t schedule(uint64_t currentRSP);  // retorna nuevo RSP a cargar
void     blockProcess(uint64_t pid);
void     unblockProcess(uint64_t pid);
void     killProcess(uint64_t pid);
void     setPriority(uint64_t pid, int priority);
PCB    * getCurrentProcess();
uint64_t getCurrentPID();

#endif
```

### Modificar `Kernel/asm/idt.asm`

Agregar el handler IRQ0 (timer) y registrarlo en el IDT. El handler ya existente para IRQ1 puede servir de modelo:

```asm
; Declarar símbolo externo (implementado en scheduler.c)
extern schedule

GLOBAL _irq00Handler

; Handler IRQ0 (timer) — agrega context switch
_irq00Handler:
    pushq 0                  ; código de error ficticio (alineación)
    push rbp
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov al, 0x20
    out 0x20, al             ; EOI al PIC master

    mov rdi, rsp             ; pasar RSP actual como argumento a schedule()
    call schedule            ; retorna el nuevo RSP a cargar
    mov rsp, rax             ; cargar stack del nuevo proceso

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rbp
    add rsp, 8               ; descartar el código de error ficticio
    iretq
```

Y en `syscall_gate_init` agregar:
```asm
SET_GATE 0x20, _irq00Handler    ; IRQ0 = vector 0x20
```

### Modificar `Kernel/kernel.c`

En `init_irqs()`, cambiar la máscara del PIC master para desmascarear también IRQ0:
```c
picMasterMask(0xF8);   // Antes era 0xF9 (IRQ0 maskeado, IRQ1 libre)
                       // 0xF8 = 11111000 → IRQ0 (timer) e IRQ1 (teclado) demascarados
```

Agregar al final de `main()` (antes o en lugar del salto a la shell):
```c
// Crear proceso idle (se ejecuta cuando no hay otros ready)
PCB * idleProc = createProcess("idle", idleMain, 0, NULL, 255, 0);
initScheduler(idleProc);

// Crear proceso shell (primer proceso de usuario)
PCB * shellProc = createProcess("sh", shellMain, 0, NULL, 0, 1);
addProcess(shellProc);

// A partir de acá el scheduler toma el control via IRQ0
// Este código nunca llega más allá del primer tick
while(1) { asm volatile("hlt"); }
```

### Tests en `Tests/processes/testScheduler.c`

```c
#include "../CuTest.h"
#include "process.h"
#include "scheduler.h"

static int dummyMain(int argc, char ** argv) { (void)argc; (void)argv; return 0; }

void testNewProcessIsReady(CuTest * tc) {
    PCB * p = createProcess("test", dummyMain, 0, NULL, 0, 0);
    CuAssertIntEquals(tc, READY, p->state);
    destroyProcess(p);
}

void testBlockedProcessNotScheduled(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * p1 = createProcess("p1", dummyMain, 0, NULL, 0, 0);
    PCB * p2 = createProcess("p2", dummyMain, 0, NULL, 0, 0);
    addProcess(p1);
    addProcess(p2);
    blockProcess(p1->pid);
    for (int i = 0; i < 10; i++) {
        schedule(0);
        CuAssertTrue(tc, getCurrentProcess()->pid != p1->pid);
    }
}

void testRoundRobinFairness(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * p1 = createProcess("p1", dummyMain, 0, NULL, 0, 0);
    PCB * p2 = createProcess("p2", dummyMain, 0, NULL, 0, 0);
    addProcess(p1);
    addProcess(p2);
    int c1 = 0, c2 = 0;
    for (int i = 0; i < 20; i++) {
        schedule(0);
        if (getCurrentProcess()->pid == p1->pid) c1++;
        else if (getCurrentProcess()->pid == p2->pid) c2++;
    }
    CuAssertTrue(tc, c1 > 0 && c2 > 0);
}

void testKilledProcessNotScheduled(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * p = createProcess("p", dummyMain, 0, NULL, 0, 0);
    uint64_t pid = p->pid;
    addProcess(p);
    killProcess(pid);
    for (int i = 0; i < 5; i++) {
        schedule(0);
        PCB * cur = getCurrentProcess();
        CuAssertTrue(tc, cur == NULL || cur->pid != pid);
    }
}

void testHighPriorityRunsMore(CuTest * tc) {
    PCB * idle = createProcess("idle", dummyMain, 0, NULL, 0, 0);
    initScheduler(idle);
    PCB * hi = createProcess("hi", dummyMain, 0, NULL, 0, 0); // prioridad 0
    PCB * lo = createProcess("lo", dummyMain, 0, NULL, 2, 0); // prioridad 2
    addProcess(hi);
    addProcess(lo);
    int hiCount = 0, loCount = 0;
    for (int i = 0; i < 30; i++) {
        schedule(0);
        if (getCurrentProcess()->pid == hi->pid) hiCount++;
        else if (getCurrentProcess()->pid == lo->pid) loCount++;
    }
    CuAssertTrue(tc, hiCount > loCount);
}

CuSuite * getSchedulerTestSuite() {
    CuSuite * suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, testNewProcessIsReady);
    SUITE_ADD_TEST(suite, testBlockedProcessNotScheduled);
    SUITE_ADD_TEST(suite, testRoundRobinFairness);
    SUITE_ADD_TEST(suite, testKilledProcessNotScheduled);
    SUITE_ADD_TEST(suite, testHighPriorityRunsMore);
    return suite;
}
```

Agregar en `Tests/AllTests.c`:
```c
CuSuite * getSchedulerTestSuite();
CuSuiteAddSuite(suite, getSchedulerTestSuite());
```

Agregar en `Tests/Makefile`:
```makefile
SOURCES += processes/testScheduler.c \
           ../Kernel/processes/scheduler.c \
           ../Kernel/processes/process.c
```

### Verificación en el kernel

```bash
make && ./run.sh
# Ejecutar desde shell:
loop &        # background
loop &        # background
ps            # debe mostrar ambos en estado RUNNING/READY
# Los dos loops deben imprimir intercalados en pantalla
```

---

## PASO 4 — Shell real (`sh`) con procesos, foreground/background

### ¿Qué se hace?
Se reemplaza el contenido de `Userland/Shell/shell.c` por la **shell real del TP2**. Los comandos existentes (`help`, `echo`, `time`, `clear`, etc.) se migran como procesos al directorio `Userland/apps/`. El profesor enfatiza que **todo debe ser un proceso** — ninguna función se llama directamente desde la shell.

### Archivos nuevos/modificados

```
Userland/Shell/shell.c              [MODIFICA] reescribir para TP2
Userland/apps/help.c                [NUEVO]
Userland/include/libc.h            [MODIFICA] agregar create_process, waitpid, kill_process, etc.
```

### Tabla de comandos en la shell

La shell mantiene una tabla de tipo:
```c
typedef struct {
    const char * name;
    ProcessMain  entry;
} Command;

static const Command commands[] = {
    { "help",    help_main    },
    { "mem",     mem_main     },
    { "ps",      ps_main      },
    { "loop",    loop_main    },
    { "kill",    kill_main    },
    { "nice",    nice_main    },
    { "block",   block_main   },
    { "cat",     cat_main     },
    { "wc",      wc_main      },
    { "filter",  filter_main  },
    { "mvar",    mvar_main    },
    { "test_mm", test_mm_main },
    { "test_proc", test_proc_main },
    { "test_sync", test_sync_main },
    { NULL, NULL }
};
```

### Lógica de `shell.c` para TP2

```c
while (1) {
    imprimirPrompt();
    leerLinea(linea);

    if (esCtrlD) { eof_process(fg_pid); continue; }
    if (esCtrlC) { kill_process(fg_pid); continue; }

    parsear(linea, &cmd1, &cmd2, &background, &tienePipe);

    ProcessMain entry = buscarComando(cmd1);
    if (!entry) { puts("comando no encontrado\n"); continue; }

    if (!tienePipe) {
        uint64_t pid = create_process(entry, argc, argv, 0, !background);
        if (!background) waitpid(pid);
    } else {
        ProcessMain entry2 = buscarComando(cmd2);
        uint64_t pipeId = pipe_create(NULL);
        // pasar pipeId como stdout del proceso 1 y stdin del proceso 2
        uint64_t pid1 = create_process(entry,  argc1, argv1, /* stdout=pipeId */);
        uint64_t pid2 = create_process(entry2, argc2, argv2, /* stdin=pipeId  */);
        if (!background) { waitpid(pid1); waitpid(pid2); }
    }
}
```

### Verificación

```bash
make && ./run.sh
help              # lista comandos — debe ser un proceso separado
loop              # foreground — Ctrl+C lo mata
loop &            # background — shell vuelve al prompt
ps                # muestra loop corriendo
kill <PID>        # mata el loop
```

---

## PASO 5 — Semáforos

### ¿Qué se hace?
Se implementan semáforos **libres de busy waiting**. `semWait` con valor 0 llama a `blockProcess()`. `semPost` llama a `unblockProcess()`. La sección crítica interna se protege con `xchg` o `lock cmpxchg`.

### Archivos nuevos/modificados

```
Kernel/include/semaphore.h          [NUEVO]
Kernel/sync/semaphore.c             [NUEVO]
Kernel/syscall.c                    [MODIFICA] casos SYS_SEM_*
Userland/include/libc.h            [MODIFICA]
Tests/sync/testSemaphore.c          [NUEVO]
Tests/AllTests.c                    [MODIFICA]
Tests/Makefile                      [MODIFICA]
```

### `Kernel/include/semaphore.h`

```c
#ifndef SEMAPHORE_H
#define SEMAPHORE_H

typedef struct SemaphoreCDT * SemaphoreADT;

SemaphoreADT semOpen(const char * name, int initialValue);
void         semClose(SemaphoreADT sem);
void         semWait(SemaphoreADT sem);   // P — bloquea si valor == 0
void         semPost(SemaphoreADT sem);   // V — desbloquea un waiter

#endif
```

### Wrappers en `Userland/include/libc.h`

```c
// Agregar al enum:
SYS_SEM_OPEN  = 38,
SYS_SEM_CLOSE = 39,
SYS_SEM_WAIT  = 40,
SYS_SEM_POST  = 41,

static inline uint64_t sem_open(const char * name, int initial_value) {
    return sys_3p(SYS_SEM_OPEN, (uint64_t)name, (uint64_t)initial_value, 0);
}
static inline void sem_close(uint64_t sem_id) {
    sys_1p(SYS_SEM_CLOSE, sem_id);
}
static inline void sem_wait(uint64_t sem_id) {
    sys_1p(SYS_SEM_WAIT, sem_id);
}
static inline void sem_post(uint64_t sem_id) {
    sys_1p(SYS_SEM_POST, sem_id);
}
```

### `Tests/sync/testSemaphore.c`

```c
#include "../CuTest.h"
#include "semaphore.h"

void testSemOpenReturnsNonNull(CuTest * tc) {
    SemaphoreADT sem = semOpen("sem1", 1);
    CuAssertPtrNotNull(tc, sem);
    semClose(sem);
}

void testSemWaitPassesWhenPositive(CuTest * tc) {
    SemaphoreADT sem = semOpen("sem2", 1);
    semWait(sem);   // valor 1→0, no bloquea
    semPost(sem);   // valor 0→1
    semClose(sem);
    CuAssertTrue(tc, 1); // llegó hasta acá = no bloqueó
}

void testSameSemNameReturnsSameInstance(CuTest * tc) {
    SemaphoreADT s1 = semOpen("shared", 1);
    SemaphoreADT s2 = semOpen("shared", 1);
    CuAssertPtrEquals(tc, s1, s2);
    semClose(s1);
    semClose(s2);
}

void testDifferentNamesAreIndependent(CuTest * tc) {
    SemaphoreADT s1 = semOpen("semA", 1);
    SemaphoreADT s2 = semOpen("semB", 0);
    CuAssertTrue(tc, s1 != s2);
    semClose(s1);
    semClose(s2);
}

CuSuite * getSemaphoreTestSuite() {
    CuSuite * suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, testSemOpenReturnsNonNull);
    SUITE_ADD_TEST(suite, testSemWaitPassesWhenPositive);
    SUITE_ADD_TEST(suite, testSameSemNameReturnsSameInstance);
    SUITE_ADD_TEST(suite, testDifferentNamesAreIndependent);
    return suite;
}
```

Agregar en `Tests/AllTests.c`:
```c
CuSuite * getSemaphoreTestSuite();
CuSuiteAddSuite(suite, getSemaphoreTestSuite());
```

Agregar en `Tests/Makefile`:
```makefile
SOURCES += sync/testSemaphore.c \
           ../Kernel/sync/semaphore.c
```

### Verificación en el kernel

```bash
test_sync 3 100000 1    # con semáforos → resultado final exactamente 0
test_sync 3 100000 0    # sin semáforos → resultado varía (race condition visible)
```

---

## PASO 6 — Pipes

### ¿Qué se hace?
Se implementan **pipes unidireccionales bloqueantes**. Lo clave: **leer de un pipe o de la terminal es transparente** — el mismo código de `cat` funciona en ambos casos. Esto se logra con la tabla de file descriptors en el PCB (`fd[2]`): FD 0 = stdin, FD 1 = stdout. La shell redirige estos FDs al crear procesos conectados por un pipe.

### Archivos nuevos/modificados

```
Kernel/include/pipe.h               [NUEVO]
Kernel/ipc/pipe.c                   [NUEVO]
Kernel/syscall.c                    [MODIFICA] casos SYS_PIPE_*
Userland/include/libc.h            [MODIFICA]
```

### `Kernel/include/pipe.h`

```c
#ifndef PIPE_H
#define PIPE_H

#include <stddef.h>
#include <stdint.h>

typedef struct PipeCDT * PipeADT;

PipeADT  pipeCreate(const char * name);
PipeADT  pipeOpen(const char * name);
int      pipeRead(PipeADT pipe, char * buf, size_t count);
int      pipeWrite(PipeADT pipe, const char * buf, size_t count);
void     pipeClose(PipeADT pipe);

#endif
```

### Wrappers en `Userland/include/libc.h`

```c
SYS_PIPE_CREATE = 42,
SYS_PIPE_OPEN   = 43,
SYS_PIPE_READ   = 44,
SYS_PIPE_WRITE  = 45,
SYS_PIPE_CLOSE  = 46,

static inline uint64_t pipe_create(const char * name) {
    return sys_1p(SYS_PIPE_CREATE, (uint64_t)name);
}
// etc.
```

### Modificar `sys_read` y `sys_write` en `Kernel/syscall.c`

Los actuales `sys_read` y `sys_write` solo soportan fd=0/1 (terminal). Con pipes, necesitan consultar los FDs del proceso actual:

```c
static uint64_t sys_write(uint64_t fd, const char *buf, uint64_t len) {
    PCB * current = getCurrentProcess();
    if (current && current->fd[1] != 0) {
        // stdout redirigido a un pipe
        PipeADT pipe = getPipeById(current->fd[1]);
        return (uint64_t) pipeWrite(pipe, buf, (size_t)len);
    }
    // comportamiento original: escribir a pantalla
    // ... (código existente)
}

static uint64_t sys_read(uint64_t fd, char *buf, uint64_t len) {
    PCB * current = getCurrentProcess();
    if (current && current->fd[0] != 0) {
        // stdin redirigido a un pipe
        PipeADT pipe = getPipeById(current->fd[0]);
        return (uint64_t) pipeRead(pipe, buf, (size_t)len);
    }
    // comportamiento original: leer del teclado
    // ... (código existente)
}
```

### Verificación en el kernel

```bash
cat | wc             # escribir líneas, Ctrl+D → wc imprime conteo correcto
cat | filter         # escribir texto → filter elimina vocales
cat | cat            # transparencia total
```

---

## PASO 7 — Comandos de User Space restantes

### ¿Qué se hace?
Se implementan todos los comandos obligatorios del TP2 en `Userland/apps/`. Cada uno es una función con firma `int nombre_main(int argc, char ** argv)` que la shell puede instanciar como proceso.

### Archivos nuevos

```
Userland/apps/mem.c
Userland/apps/ps.c
Userland/apps/loop.c
Userland/apps/kill.c
Userland/apps/nice.c
Userland/apps/block.c
Userland/apps/cat.c
Userland/apps/wc.c
Userland/apps/filter.c
Userland/apps/mvar.c
```

### Reglas críticas por comando

**`loop.c` — espera ACTIVA obligatoria:**
```c
int loop_main(int argc, char ** argv) {
    (void)argc; (void)argv;
    uint64_t pid = get_pid();
    while (1) {
        // Imprimir PID con saludo
        printf("Hello from PID %llu\n", pid);
        // Espera activa — NO usar sleep ni yield
        for (volatile long i = 0; i < 10000000L; i++);
    }
    return 0;
}
```

**`mvar.c` — CERO busy waiting en la sincronización:**
```c
// Escritor: sem_wait(vacia) → escribir → sem_post(llena)
// Lector:   sem_wait(llena) → leer    → sem_post(vacia)
// El main termina INMEDIATAMENTE después de crear lectores y escritores
// Nunca un spinloop — todo el sincronismo por semáforos
int mvar_main(int argc, char ** argv) {
    int writers = atoi(argv[1]);
    int readers = atoi(argv[2]);
    // crear escritores y lectores como procesos
    // ...
    return 0;  // termina inmediatamente
}
```

### Verificación en el kernel

```bash
mem                      # total/free/used coherentes y > 0
ps                       # tabla con nombre, PID, prioridad, estado, fg/bg, SP, BP
loop &
loop &
ps                       # dos loops en RUNNING
kill <PID>               # mata uno — desaparece del ps
nice <PID> 2             # baja prioridad — el otro imprime más seguido visualmente
block <PID>              # deja de imprimir
block <PID>              # vuelve a imprimir

mvar 2 2                 # salida: ABABABABAB...
mvar 3 2                 # salida: ABCABCABC...
# Si 5 lectores muestran solo 2 colores → starvation → bug de implementación
```

---

## PASO 8 — Tests de la cátedra como procesos

### ¿Qué se hace?
Los tres tests obligatorios se agregan como procesos ejecutables desde la shell. Deben correr en **foreground Y background**. Si alguno imprime mensajes de error → hay un bug.

### Archivos nuevos

```
Userland/apps/test_mm.c
Userland/apps/test_proc.c
Userland/apps/test_sync.c
```

### Verificación

```bash
# Foreground — sin output = sin errores:
test_mm 1000000
test_proc 10
test_sync 3 10000 1      # resultado: 0

# Background simultáneos:
test_mm 1000000 &
test_proc 10 &
ps                       # ambos visibles y activos

# Race condition visible:
test_sync 3 10000 0      # resultado ≠ 0 (varía cada vez)
```

---

## PASO 9 — Buddy System

### ¿Qué se hace?
Se reemplaza el bump allocator por el **Buddy System**, manteniendo **exactamente la misma interfaz** de `Kernel/include/memoryManager.h`. El switch se hace con `make buddy` — ningún otro archivo del kernel sabe cuál está linkeado.

### Archivos nuevos/modificados

```
Kernel/memory/buddyMM.c             [NUEVO]
Kernel/Makefile                     [MODIFICA] regla buddy con -DUSE_BUDDY
Tests/memory/testBuddyMM.c          [NUEVO]
Tests/AllTests.c                    [MODIFICA]
Tests/Makefile                      [MODIFICA]
```

### Contrato obligatorio — mismas funciones que `dummyMM.c`

```c
// buddyMM.c debe exportar EXACTAMENTE estas funciones bajo #ifdef USE_BUDDY:
MemoryManagerADT createMemoryManager(void * restrict memoryForMM,
                                     void * restrict managedMemory,
                                     uint64_t managedMemorySize);
void *   mallocMM(MemoryManagerADT mm, size_t size);
void     freeMM(MemoryManagerADT mm, void * ptr);   // ahora libera de verdad
uint64_t getTotalMemory(MemoryManagerADT mm);
uint64_t getFreeMemory(MemoryManagerADT mm);
uint64_t getUsedMemory(MemoryManagerADT mm);
```

En `Kernel/memory/dummyMM.c` y `buddyMM.c` usar:
```c
#ifndef USE_BUDDY   // en dummyMM.c
// ... implementación dummy
#else               // en buddyMM.c
// ... implementación buddy
#endif
```

O bien usar dos archivos separados y elegir cuál compilar en el Makefile según el flag `USE_BUDDY`.

### Tests extra para el Buddy en `Tests/memory/testBuddyMM.c`

```c
// Los mismos 7 tests del dummy (contrato idéntico) más:

void testFreeAndRealloc(CuTest * tc) {
    MemoryManagerADT mm = setupBuddyMM();
    void * p1 = mallocMM(mm, 128);
    freeMM(mm, p1);
    void * p2 = mallocMM(mm, 128);
    CuAssertPtrNotNull(tc, p2);
    CuAssertTrue(tc, getUsedMemory(mm) <= 128 + 16);
}

void testCoalescing(CuTest * tc) {
    MemoryManagerADT mm = setupBuddyMM();
    void * p1 = mallocMM(mm, 64);
    void * p2 = mallocMM(mm, 64);
    freeMM(mm, p1);
    freeMM(mm, p2);
    // después del coalescing debe poder alocar un bloque más grande
    CuAssertPtrNotNull(tc, mallocMM(mm, 128));
}

void testFragmentation(CuTest * tc) {
    MemoryManagerADT mm = setupBuddyMM();
    void * ptrs[10];
    for (int i = 0; i < 10; i++) ptrs[i] = mallocMM(mm, 64);
    for (int i = 0; i < 10; i += 2) freeMM(mm, ptrs[i]); // liberar alternados
    CuAssertPtrNotNull(tc, mallocMM(mm, 64));
}
```

### Verificación

```bash
make buddy && ./run.sh

test_mm 1000000       # debe pasar igual que con el dummy
mem                   # la memoria libre debe fluctuar (alloc/free reales visibles)
```

---

## PASO 10 — Pulido final

### ¿Qué se hace?
Revisión final antes de la entrega. Chequeos obligatorios para no ir a recuperatorio.

### Archivos nuevos/modificados

```
README.md           [NUEVO/MODIFICA] instrucciones de compilación, comandos, atajos, ejemplos, limitaciones
.gitignore          [VERIFICA] *.bin, *.img, *.o, *.qcow2, *.vmdk, Tests/runTests
```

### Checklist obligatorio

```bash
# 1. Sin warnings con -Wall — OBLIGATORIO para no ir a recuperatorio
cd x64BareBones && make 2>&1 | grep -i warning
cd x64BareBones && make buddy 2>&1 | grep -i warning
# → ambos deben estar vacíos

# 2. Todos los unit tests pasan
cd x64BareBones/Tests && make run
# OK (N tests)

# 3. Tests de cátedra en foreground Y background
test_mm 1000000
test_mm 1000000 &
test_proc 10
test_proc 10 &
test_sync 3 10000 1
test_sync 3 10000 1 &

# 4. No hay binarios en el repo
find x64BareBones -name "*.bin" -o -name "*.img" -o -name "*.o"
# → todos deben estar ignorados por .gitignore

# 5. README tiene: instrucciones de compilación, lista de comandos con parámetros,
#    caracteres especiales (| y &), atajos (Ctrl+C, Ctrl+D), ejemplos por requerimiento,
#    requerimientos faltantes o parciales, limitaciones, citas de IA usada
```

---

## Resumen de tests por paso

| Paso | Qué se testea | Archivo | Cómo correr |
|------|--------------|---------|-------------|
| 2 | Bump allocator: alloc, solapamiento, NULL, alineación, stats, escritura | `Tests/memory/testDummyMM.c` | `cd Tests && make run` |
| 3 | Scheduler: READY, bloqueado sin quantum, Round Robin, kill, prioridades | `Tests/processes/testScheduler.c` | `cd Tests && make run` |
| 5 | Semáforos: open, wait/post sin bloqueo, nombre compartido, independencia | `Tests/sync/testSemaphore.c` | `cd Tests && make run` |
| 8 | test_mm, test_proc, test_sync en foreground y background | Shell del kernel en QEMU | `./dev.sh → make → ./run.sh` |
| 9 | Buddy: mismo contrato + free real + coalescing + fragmentación | `Tests/memory/testBuddyMM.c` | `cd Tests && make run` |

---

## Referencia rápida: archivos clave del repo actual

| Tarea | Archivo a modificar |
|---|---|
| Agregar nueva syscall (número) | `Kernel/include/syscall.h` |
| Implementar nueva syscall (lógica) | `Kernel/syscall.c` → `syscall_dispatch()` |
| Agregar wrapper userland | `Userland/include/libc.h` |
| Agregar gate a IDT (nueva interrupción) | `Kernel/asm/idt.asm` → `syscall_gate_init` |
| Inicialización del kernel | `Kernel/kernel.c` → `initializeKernelBinary()` y `main()` |
| Agregar nuevo módulo kernel | Crear archivo en subdirectorio + actualizar `SOURCES` en `Kernel/Makefile` |
| Agregar nuevo comando userland | Crear `Userland/apps/nombre.c` + registrar en tabla de comandos de `shell.c` |
| Cambiar máscara IRQs PIC master | `Kernel/kernel.c` → `init_irqs()` → `picMasterMask(...)` |

---

> **Recordatorio del profesor:** El orden de pasos no es arbitrario. El Paso 2 (MM) desbloquea el Paso 3 porque el scheduler necesita `mallocMM` para alocar los PCBs. El Paso 3 desbloquea el Paso 5 porque los semáforos usan `blockProcess` y `unblockProcess`. El Paso 5 desbloquea el Paso 7 porque `mvar` usa semáforos. No saltear pasos.
