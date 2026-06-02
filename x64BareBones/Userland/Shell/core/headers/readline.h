#ifndef READLINE_H
#define READLINE_H

#include <stddef.h>   // size_t

#define CMD_MAX_LEN 64   // largo máximo de una línea de comando

// Lee una línea de stdin con edición:
//   - Backspace borra el último carácter (pantalla y buffer).
//   - Tab autocompleta con el historial por prefijo, o lo navega si no hay texto.
// Escribe hasta max-1 caracteres + '\0' en buf.
// Devuelve la longitud leída, incluyendo el '\n' final si se presionó Enter.
size_t readline_hist(char *buf, size_t max);

#endif