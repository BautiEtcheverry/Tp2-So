# TP2 Q1 2026 — ITBA — (72.11) Sistemas Operativos

Construcción del núcleo de un sistema operativo y mecanismos de administración de recursos.

### Grupo
| Nombre | Legajo | Email |
|---|---|---|
| Etcheverry Bautista | 65765 | betcheverry@itba.edu.ar |
| Leite Benicio | 64181 | bleite@itba.edu.ar |
| Villanueva Felipe | 65250 | fvillanueva@itba.edu.ar |

---

## Entorno de compilación

La compilación se realiza dentro de la imagen Docker oficial de la cátedra: `agodio/itba-so-multiarch:3.1`.
QEMU se ejecuta en el host (no dentro del contenedor).

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
make            # compila con el memory manager por defecto (elegido por el grupo)
make buddy      # compila con el buddy system
make clean
```
> Para cambiar de un memory manager al otro conviene `make clean` antes (cambia el `.c` que se compila).

### Ejecutar (en el host, fuera del contenedor)
```bash
./run.sh
```
Levanta QEMU con la imagen generada en `x64BareBones/Image/x64BareBonesImage.qcow2`. Detecta automáticamente coreaudio en macOS y alsa en Linux para el PC speaker.

---

## Comandos y tests

Al iniciar el sistema se carga la **shell** (`sh`). Comandos disponibles:

### Generales
| Comando | Descripción |
|---|---|
| `help` | Muestra los comandos disponibles agrupados por categoría. |
| `ls` | Lista los comandos. |
| `cmd-history [-c]` | Historial de comandos (`-c` lo limpia). |
| `clear` | Limpia la pantalla. |
| `echo <args>` | Imprime los argumentos. |
| `time` | Muestra fecha y hora del RTC. |
| `textColor <color>` | Cambia el color del texto. |
| `textSize <n>` | Cambia el tamaño del texto. |
| `regs` | Imprime los registros capturados (Ctrl+R guarda snapshot). |
| `trigger-div` / `trigger-ud` | Disparan excepciones para testear el handler. |

### Memoria
| Comando | Descripción |
|---|---|
| `mem` | Estado del memory manager (total / usado / libre). |

### Procesos
| Comando | Descripción |
|---|---|
| `ps` | Lista procesos (PID, PPID, prio, estado, stack, RSP, foreground). |
| `kill <pid>` | Termina un proceso. |
| `nice <pid> <prio>` | Cambia la prioridad del proceso. |
| `block <pid>` / `unblock <pid>` | Bloquea / desbloquea un proceso. |
| `loop [&]` | Imprime saludo con su PID periódicamente. |

### IPC
| Comando | Descripción |
|---|---|
| `cat` | Copia stdin a stdout. |
| `wc` | Cuenta líneas leídas de stdin. |
| `filter` | Filtra las vocales de stdin. |
| `mvar <wr> <rd>` | Test de lectores/escritores sobre una MVar (sincronización). |

### Tests de cátedra
| Comando | Descripción |
|---|---|
| `test_mm <max_bytes>` | Stress test del memory manager. |
| `test_proc <max_procs>` | Stress test del scheduler / creación de procesos. |
| `test_sync <n> <use_sem 0\|1>` | Test de semáforos (con/sin sincronización). |
| `test_prio <max_value>` | Test de prioridades del scheduler. |

## Caracteres especiales y atajos

- `|` — pipe entre dos comandos: `cmd1 | cmd2` ejecuta ambos en procesos separados conectando stdout→stdin.
- `&` — ejecuta el comando en background: `loop &`.
- **Ctrl+C** — envía señal al proceso foreground actual (lo termina).
- **Ctrl+D** — EOF en stdin (cierra el extremo de lectura).
- **Ctrl+R** — guarda un snapshot de registros para inspeccionar con `regs`.
- Flechas ↑/↓ — navegan por el historial.

## Limitaciones y requerimientos parcialmente implementados

_(Sección a completar antes de la entrega con lo que quede pendiente o desviado de la consigna.)_

## Citas / uso de IA

Se utilizó asistencia de IA (Claude / ChatGPT) como apoyo para:
- Redacción y formateo de este README.
- Consultas puntuales de sintaxis de NASM, convención System V AMD64 y patrones de scheduling.

Todo el código entregado fue revisado, adaptado e integrado por los integrantes del grupo. Las referencias bibliográficas adicionales (x86BareBones de la cátedra, "Operating Systems: Three Easy Pieces", manual Intel SDM Vol. 3) se citan en el informe.
