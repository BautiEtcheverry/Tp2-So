#include "pipe.h"
#include "scheduler.h"
#include "libasm.h"
#include <stddef.h>
#include <stdint.h>

static Pipe pipes[MAX_PIPES];

void pipe_init(void) {
    for (int i = 0; i < MAX_PIPES; i++) {
        pipes[i].in_use       = 0;
        pipes[i].read_pos     = 0;
        pipes[i].write_pos    = 0;
        pipes[i].count        = 0;
        pipes[i].write_closed = 0;
        pipes[i].read_closed  = 0;
        pipes[i].blocked_reader = 0;
        pipes[i].blocked_writer = 0;
    }
}

int pipe_open(int id) {
    if (id == -1) {
        /* Pipe anónimo: buscar el primer slot libre */
        for (int i = 0; i < MAX_PIPES; i++) {
            if (!pipes[i].in_use) {
                id = i;
                break;
            }
        }
        if (id == -1)
            return -1;  /* no hay slots libres */
    } else {
        if (id < 0 || id >= MAX_PIPES)
            return -1;
        /* Pipe nombrado: si ya existe, simplemente retornar el id */
        if (pipes[id].in_use)
            return id;
    }

    Pipe *p = &pipes[id];
    p->read_pos      = 0;
    p->write_pos     = 0;
    p->count         = 0;
    p->write_closed  = 0;
    p->read_closed   = 0;
    p->blocked_reader = 0;
    p->blocked_writer = 0;
    p->in_use        = 1;
    return id;
}

int pipe_write(int id, const char *buf, int len) {
    if (id < 0 || id >= MAX_PIPES)
        return -1;

    Pipe *p = &pipes[id];
    uint64_t flags = irq_save();
    if (!p->in_use || p->read_closed) {
        irq_restore(flags);
        return -1;
    }

    int written = 0;
    while (written < len) {
        /* Esperar mientras el buffer está lleno */
        while (p->count == PIPE_BUF_SIZE) {
            if (p->read_closed) {
                irq_restore(flags);
                return written;  /* el lector desapareció */
            }

            p->blocked_writer = getCurrentPID();
            blockProcess(getCurrentPID());
            /* sti+hlt atómico: si el wakeup llega entre el sti y el hlt
             * en x86 se mantiene pendiente y dispara el hlt igual.
             * Si los hicieramos por separado se podría perder el wakeup
             * y este proceso quedaría dormido para siempre. */
            sti_hlt();
            cpu_cli();
            p->blocked_writer = 0;
            /* re-chequea count y read_closed en la siguiente iter del while */
        }

        p->buf[p->write_pos] = buf[written++];
        p->write_pos = (p->write_pos + 1) % PIPE_BUF_SIZE;
        p->count++;

        /* Si había un lector bloqueado esperando datos, despertarlo */
        if (p->blocked_reader != 0) {
            unblockProcess(p->blocked_reader);
            p->blocked_reader = 0;
        }
    }
    irq_restore(flags);
    return written;
}

int pipe_read(int id, char *buf, int len) {
    if (id < 0 || id >= MAX_PIPES)
        return -1;

    Pipe *p = &pipes[id];
    uint64_t flags = irq_save();
    if (!p->in_use) {
        irq_restore(flags);
        return -1;
    }

    /* Esperar mientras el buffer está vacío */
    while (p->count == 0) {
        if (p->write_closed) {
            irq_restore(flags);
            return 0;  /* EOF: el escritor cerró y no quedan datos */
        }

        p->blocked_reader = getCurrentPID();
        blockProcess(getCurrentPID());
        sti_hlt();
        cpu_cli();
        p->blocked_reader = 0;
    }

    /* Leer hasta len bytes de lo que haya disponible */
    int n = 0;
    while (n < len && p->count > 0) {
        buf[n++] = p->buf[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUF_SIZE;
        p->count--;
    }

    /* Si había un escritor bloqueado esperando espacio, despertarlo */
    if (p->blocked_writer != 0) {
        unblockProcess(p->blocked_writer);
        p->blocked_writer = 0;
    }

    irq_restore(flags);
    return n;
}

void pipe_close_write(int id) {
    if (id < 0 || id >= MAX_PIPES)
        return;

    Pipe *p = &pipes[id];
    uint64_t flags = irq_save();
    if (!p->in_use) {
        irq_restore(flags);
        return;
    }

    p->write_closed = 1;

    /* Despertar al lector para que detecte el EOF */
    if (p->blocked_reader != 0) {
        unblockProcess(p->blocked_reader);
        p->blocked_reader = 0;
    }

    /* Si el lector también cerró, liberar el slot */
    if (p->read_closed)
        p->in_use = 0;
    irq_restore(flags);
}

void pipe_close_read(int id) {
    if (id < 0 || id >= MAX_PIPES)
        return;

    Pipe *p = &pipes[id];
    uint64_t flags = irq_save();
    if (!p->in_use) {
        irq_restore(flags);
        return;
    }

    p->read_closed = 1;

    /* Despertar al escritor para que detecte el broken pipe */
    if (p->blocked_writer != 0) {
        unblockProcess(p->blocked_writer);
        p->blocked_writer = 0;
    }

    /* Si el escritor también cerró, liberar el slot */
    if (p->write_closed)
        p->in_use = 0;
    irq_restore(flags);
}
