#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_BUF_SIZE  512
#define MAX_PIPES      16

/*
 * Todo fd almacenado en PCB->fd[] se interpreta así:
 *   0            = teclado (stdin real)
 *   1            = pantalla (stdout real)
 *   PIPE_FD_BASE + id  = pipe con ese id (0..MAX_PIPES-1)
 *
 * Los procesos no necesitan saber si están hablando con un pipe o
 * con la terminal — sys_read/sys_write consultan esta tabla y rutean.
 */
#define PIPE_FD_BASE   2

#define PIPE_ID_TO_FD(id)   ((id) + PIPE_FD_BASE)
#define PIPE_FD_TO_ID(fd)   ((fd) - PIPE_FD_BASE)
#define IS_PIPE_FD(fd)      ((fd) >= PIPE_FD_BASE)

typedef struct {
    char     buf[PIPE_BUF_SIZE];
    int      read_pos;
    int      write_pos;
    int      count;           /* bytes actualmente en el buffer            */
    int      in_use;          /* 1 si este slot está activo                */
    int      write_closed;    /* 1 cuando el extremo escritor fue cerrado  */
    int      read_closed;     /* 1 cuando el extremo lector fue cerrado    */
    uint64_t blocked_reader;  /* PID del proceso bloqueado leyendo, 0=nadie */
    uint64_t blocked_writer;  /* PID del proceso bloqueado escribiendo, 0=nadie */
} Pipe;

/* Inicializa la tabla global de pipes — llamar desde kernel_init */
void pipe_init(void);

/*
 * Abre (o crea) un pipe.
 *   id == -1  → asigna el primer slot libre (pipe anónimo, para shell |)
 *   id >= 0   → usa exactamente ese slot (pipe nombrado, para procesos no relacionados)
 * Retorna el id asignado, o -1 si no hay slots libres / id inválido.
 */
int pipe_open(int id);

/* Escribe len bytes en el pipe. Bloquea si el buffer está lleno.
 * Retorna bytes escritos, o -1 si el extremo lector ya fue cerrado. */
int pipe_write(int id, const char *buf, int len);

/* Lee hasta len bytes del pipe. Bloquea si el buffer está vacío.
 * Retorna bytes leídos, o 0 si el escritor cerró y no quedan datos (EOF). */
int pipe_read(int id, char *buf, int len);

/* Cierra el extremo escritor del pipe. Desbloquea al lector (EOF). */
void pipe_close_write(int id);

/* Cierra el extremo lector del pipe. Desbloquea al escritor (broken pipe). */
void pipe_close_read(int id);

#endif
