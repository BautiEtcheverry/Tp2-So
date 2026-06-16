# TP2 Q1 2026 — ITBA — (72.11) Sistemas Operativos

Construcción del núcleo de un sistema operativo y mecanismos de administración de recursos.

### Grupo
| Nombre | Legajo | Email |
|---|---|---|
| Etcheverry Bautista | 65765 | betcheverry@itba.edu.ar |
| Leite Benicio | 64181 | bleite@itba.edu.ar |
| Villanueva Felipe | 65250 | fvillanueva@itba.edu.ar |

---

## 1. Instrucciones de compilación y ejecución

La compilación se realiza dentro de la imagen Docker oficial de la cátedra (`agodio/itba-so-multiarch:3.1`). QEMU se ejecuta en el host.

### Requisitos previos
- Docker (Docker Desktop en macOS/Windows, daemon en Linux).
- QEMU instalado en el host (`qemu-system-x86_64`).

### Levantar el entorno
Desde la raíz del repo:
```bash
./dev.sh
```
Esto:
1. Verifica que Docker esté corriendo.
2. Descarga la imagen `agodio/itba-so-multiarch:3.1` si no está local.
3. Crea el contenedor `tp2-so` con el repo montado en `/root/tp2-so` (si no existe).
4. Lo arranca si está parado.
5. Te deja en una shell interactiva dentro de `/root/tp2-so/x64BareBones`.

Salir con `exit` no detiene el contenedor; la próxima vez `./dev.sh` entra de inmediato. Para apagarlo manualmente: `docker stop tp2-so`.

### Compilar (dentro del contenedor)
```bash
make            # compila con el memory manager por defecto (free-list)
make buddy      # compila con el buddy system
make clean
```
> Para cambiar de un memory manager al otro conviene `make clean` antes (cambia el `.c` que se compila).

### Ejecutar (en el host, fuera del contenedor)
```bash
./run.sh
```
Levanta QEMU con la imagen generada en `x64BareBones/Image/x64BareBonesImage.qcow2`. Detecta automáticamente `coreaudio` en macOS y `alsa` en Linux para el PC speaker.

---

## 2. Instrucciones de replicación

### 2.1 Comandos y tests

Al iniciar el sistema se carga la shell. Todos los comandos aceptan `<args>` separados por espacios. Los marcados como **PROGRAM** corren en un proceso aparte (pueden combinarse con `|` y `&`); los **BUILTIN** se ejecutan dentro de la shell.

#### Generales
| Comando | Parámetros | Descripción |
|---|---|---|
| `help` | — | Lista comandos por categoría (paginado). |
| `ls` | — | Lista todos los comandos disponibles. |
| `cmd-history` | `[-c]` | Muestra historial; `-c` lo limpia. |
| `clear` | — | Limpia la pantalla. |
| `echo` | `<args...>` | Imprime los argumentos. |
| `time` | — | Fecha y hora del RTC. |
| `textColor` | `<color>` | Cambia color de texto. |
| `textSize` | `<n>` | Cambia tamaño de texto. |
| `regs` | — | Imprime los registros capturados con Ctrl+R. |
| `trigger-div` | — | Dispara `#DE` (divide-by-zero). |
| `trigger-ud` | — | Dispara `#UD` (opcode inválido). |

#### Memoria
| Comando | Parámetros | Descripción |
|---|---|---|
| `mem` | — | Estado del memory manager: total / usado / libre. |

#### Procesos
| Comando | Parámetros | Descripción |
|---|---|---|
| `ps` | — | Lista procesos (PID, nombre, estado, prioridad, foreground, stack base, RSP). |
| `kill` | `<pid>` | Termina el proceso `<pid>`. |
| `nice` | `<pid> <prio>` | Cambia la prioridad de `<pid>` (0 = más alta). |
| `block` | `<pid>` | Bloquea el proceso `<pid>`. |
| `unblock` | `<pid>` | Desbloquea el proceso `<pid>`. |
| `loop` | — | Imprime "Hola desde PID <n>" cada ~1s. |
| `endless_loop` | — | Spin silencioso (para tests de scheduler). |

#### IPC
| Comando | Parámetros | Descripción |
|---|---|---|
| `cat` | — | Copia stdin a stdout. |
| `wc` | — | Cuenta líneas leídas de stdin. |
| `filter` | — | Filtra las vocales de stdin. |
| `mvar` | `<writers> <readers>` | Crea N escritores y M lectores sobre una MVar. |

#### Tests de cátedra
| Comando | Parámetros | Descripción |
|---|---|---|
| `test_mm` | `<max_bytes>` | Stress test del memory manager. |
| `test_proc` | `<max_procs>` | Stress test de creación/scheduling de procesos. |
| `test_sync` | `<n> <use_sem 0\|1>` | 4 procesos (2 suman, 2 restan) hacen `n` operaciones sobre una variable global. Con sem=1 → 0; sin sem=0 → varía entre corridas. |
| `test_prio` | `<count>` | 3 procesos cuentan hasta `<count>`; con distinta prioridad se ve cuál termina antes. |

### 2.2 Caracteres especiales (pipes y background)

- **`|`** — pipe entre dos comandos. `cmd1 | cmd2` los lanza en procesos separados, conectando `stdout` de `cmd1` con `stdin` de `cmd2`. La shell hace `waitpid` de ambos antes de devolver el prompt.
- **`&`** — al final del comando lo lanza en background; la shell devuelve el prompt enseguida sin esperarlo. Ejemplo: `loop &`. Compatible con pipe: `cat | wc &`.

### 2.3 Atajos de teclado

- **Ctrl+C** — termina el proceso foreground actual (la shell vuelve a recibir el prompt).
- **Ctrl+D** — envía EOF en `stdin` (cierra el extremo de lectura del proceso foreground).
- **Ctrl+R** — guarda un snapshot del estado de registros para inspeccionar luego con `regs`.
- **↑ / ↓** — navegan el historial de comandos.
- **← / →**, **Backspace** — edición de la línea actual.

### 2.4 Ejemplos por requerimiento (fuera de los tests)

**Memory manager (`mem`)**
```
> mem
> loop &                # PID 4
> loop &                # PID 5
> loop &                # PID 6
> mem                   # se observa que "usado" creció
> ps
> kill 4
> kill 5
> kill 6
> mem                   # vuelve a bajar
```

**Scheduler / prioridades (`nice`, `block`)**
```
> loop &                # PID 4
> loop &                # PID 5
> nice 4 0              # 4 con máxima prioridad
> nice 5 3              # 5 con baja prioridad (rango 0-3) → corre menos seguido
> block 4               # 4 deja de imprimir
> unblock 4
> kill 4
> kill 5
```

**Pipes y background (IPC sin semáforo)**
```
> cat | wc              # se tipea texto, Ctrl+D, imprime cantidad de líneas
> cat | filter          # los caracteres se reenvían sin vocales en tiempo real
> loop &                # corre en background mientras seguimos usando la shell
```

**Semáforos / sincronización**
```
> mvar 2 3              # 2 escritores y 3 lectores sobre una MVar
> test_sync 1000 0      # sin semáforo → race condition: resultado != 0 (varía cada corrida)
> test_sync 1000 1      # con semáforo → resultado == 0 siempre
```

**Excepciones**
```
> trigger-div           # muestra pantalla de error con registros + reinicia shell
> regs                  # luego de Ctrl+R, vuelca el snapshot
```

### 2.5 Requerimientos faltantes o parcialmente implementados

- **Memory manager**: implementados free-list y buddy system. Se elige en tiempo de compilación (`make` / `make buddy`).
- **Procesos**: `fork/exec` no implementados (la cátedra no lo pide). Se usa `sys_create_process` con entry point + argv.
- **Scheduler**: round-robin con prioridades (0–3, 0 = más alta; a mayor prioridad, más quantums por turno). Aging no implementado (no requerido).
- **IPC**: pipes con buffer circular (read/write bloqueantes) y semáforos contadores con cola de espera. Soportan pipes con nombre (procesos no relacionados los comparten acordando un id vía `pipe_open(id)`); la shell usa pipes anónimos (`id = -1`) para `|`.
- **Sincronización**: `test_sync` sin semáforo agrega una espera activa en la sección crítica para ensanchar la ventana y hacer visible la race condition (con semáforo se serializa → resultado 0).
- **Shell**: soporta un único `|` por línea (no cadenas de 3+ comandos). No hay redirección a archivo (`>`, `<`) porque no hay filesystem.
- Resto de la consigna obligatoria: implementado.

---

## 3. Limitaciones

- Un solo CPU lógico (kernel no SMP); la concurrencia se basa en preempción por timer.
- No hay paginación on-demand ni protección por anillos para userland (todo corre en ring 0).
- Sin filesystem ni persistencia: la imagen es read-only y el estado se pierde al reiniciar.
- El historial de comandos es en memoria (se pierde al reboot).
- `textSize` ofrece tres tamaños predefinidos (default / large / xlarge = 1× / 2× / 3×); no hay escalado arbitrario.
- Los procesos comparten el address space; un bug de userland puede corromper memoria del kernel.
- Cadenas de más de un pipe no están soportadas por el parser de la shell.
- El audio del PC speaker requiere backend de QEMU compatible (coreaudio/alsa); en otros entornos se silencia.

---

## 4. Citas de fragmentos de código y uso de IA

### Código de terceros
- Base **x64BareBones** provista por la cátedra (Bootloader, Loader, esqueleto de Makefile, `naiveConsole`, ABI de syscalls): https://bitbucket.org/RowDaBoat/x64barebones/
- Toolchain (NASM, ld, gcc) y entrypoint de loader: parte de la imagen `agodio/itba-so-multiarch:3.1`.

### Bibliografía
- Tanenbaum, A. — *Modern Operating Systems* (4ª ed.) — capítulos de procesos, scheduling, IPC y memoria.
- Arpaci-Dusseau — *Operating Systems: Three Easy Pieces* — referencia para semáforos, productor/consumidor y buddy allocator.
- Intel® 64 and IA-32 Architectures Software Developer's Manual, Vol. 3 — para excepciones, IDT, modo largo y comportamiento de `sti; hlt`.

### Uso de IA
Se utilizaron asistentes de IA (**Claude** y **ChatGPT**) como herramienta de apoyo, no como autor de código entregable. Casos concretos:
- Redacción y formateo de este README y comentarios.
- Consultas puntuales de sintaxis de NASM y de la convención de llamada System V AMD64.
- Revisión de race conditions y patrones de busy-wait (auditoría que motivó los `irq_save`/`irq_restore` en semáforos y la escritura serializada a video).
- Discusión de alternativas de diseño (deferred reap, `sti; hlt` atómico en sleeps de pipes).

Todo el código entregado fue revisado, adaptado e integrado manualmente por los integrantes del grupo, y validado contra los tests provistos por la cátedra (`test_mm`, `test_proc`, `test_sync`, `test_prio`).
