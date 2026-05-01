# TP2 Q1 2026 — ITBA — (72.11) Sistemas Operativos

Construcción del núcleo de un sistema operativo y mecanismos de administración de recursos.

### Grupo
| Nombre | Legajo | Email |
|---|---|---|
| Etcheverry Bautista | 65765 | betcheverry@itba.edu.ar |
| Humphreys Juan Edwin | 65477 | jhumphreys@itba.edu.ar |
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
make            # compila con el memory manager por defecto
# make buddy    # (a implementar) compila con buddy system
make clean
```

### Ejecutar (en el host, fuera del contenedor)
```bash
./run.sh
```
Levanta QEMU con la imagen generada en `x64BareBones/Image/x64BareBonesImage.qcow2`. Detecta automáticamente coreaudio en macOS y alsa en Linux para el PC speaker.

---

## Comandos y tests

_Sección a completar a medida que se implementen las apps de userland (sh, ps, kill, nice, mem, mvar, cat, wc, filter, loop, help, block) y los tests provistos (test_mm, test_proc, test_sync, test_prio)._

## Caracteres especiales y atajos

_A completar (pipe `|`, background `&`, Ctrl+C, Ctrl+D)._

## Limitaciones y requerimientos parcialmente implementados

_A completar._

## Citas / uso de IA

_A completar._
