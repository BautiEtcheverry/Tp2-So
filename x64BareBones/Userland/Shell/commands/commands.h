#ifndef COMMANDS_H
#define COMMANDS_H

// Firma que debe cumplir todo comando de la shell
typedef int (*cmdFuncP)(int argc, char *argv[]);

// Categorías para agrupar la salida de "help"
typedef enum { CAT_GENERAL, CAT_MEM, CAT_PROC, CAT_IPC, CAT_TEST } CmdCategory;

typedef enum { BUILTIN, PROGRAM } CmdKind;

// Una entrada de la tabla de comandos
typedef struct {
	const char *name;	  // lo que tipea el usuario
	cmdFuncP fn;		  // función a ejecutar
	const char *help;	  // descripción / uso (lo muestra "help")
	CmdCategory category; // Categoria para agrupar en comando "help"
	CmdKind kind;		  // cómo lo corre la shell
} Command;

// Tabla de comandos, terminada en   {NULL, NULL, NULL, 0}. Se define en shell.c.
extern const Command commands[];

#endif