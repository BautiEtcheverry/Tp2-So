#include "../../../include/libc.h"

/*
 * mvar <escritores> <lectores>
 *
 * Problema de múltiples lectores/escritores sobre una variable global,
 * estilo MVar de Haskell: una "caja" que está vacía o contiene UN valor.
 *
 * Se modela como un buffer acotado de 1 slot con dos semáforos:
 *   sem_empty (init 1): hay lugar para escribir (la caja está vacía).
 *   sem_full  (init 0): hay un valor para leer  (la caja está llena).
 *
 * putMVar (escritor): wait(empty) -> escribe -> post(full)
 * takeMVar (lector):  wait(full)  -> consume -> post(empty)
 *
 * Esto garantiza exclusión mutua (solo un proceso toca la caja a la vez) y,
 * gracias a la cola FIFO de los semáforos, los escritores se alternan
 * (A, B, A, B, ...). El proceso principal termina apenas crea a los hijos.
 */

#define MAX_WRITERS 8
#define MAX_READERS 8

/* Paleta de colores para distinguir a cada lector. */
static const uint32_t reader_colors[MAX_READERS] = {
	0xFF5555, /* rojo   */
	0x55FF55, /* verde  */
	0x5599FF, /* azul   */
	0xFFFF55, /* amarillo */
	0xFF55FF, /* magenta */
	0x55FFFF, /* cian   */
	0xFFFFFF, /* blanco */
	0xFF9933, /* naranja */
};

/* ---- Estado compartido (todos los procesos comparten el address space) ---- */
static volatile char box; /* el valor en la MVar */

/*
 * Nombres de los dos semáforos de la MVar. Cada hijo los abre por su cuenta
 * (sem_open por nombre devuelve el mismo id a todos), así son los hijos los que
 * tienen el refcount: al matarlos a todos, el kernel libera los semáforos y la
 * próxima corrida los crea frescos. Por eso main NO los abre.
 */
#define SEM_EMPTY "mvar_empty" /* "hay lugar para escribir" (init 1) */
#define SEM_FULL "mvar_full"   /* "hay un valor para leer"  (init 0) */

/*
 * argv de los hijos: debe persistir después de que main retorne, así que vive
 * en almacenamiento estático (el kernel no copia argv, solo guarda el puntero).
 * Cada hijo recibe {"writer"/"reader", "<dato>", NULL}; el dato es un único
 * carácter: la letra del escritor, o el índice de color del lector.
 */
static char child_arg[MAX_WRITERS + MAX_READERS][2];
static char *child_argv[MAX_WRITERS + MAX_READERS][3];

/*
 * Espera activa de duración pseudo-aleatoria
 * No se puede usar una seed estática. En su lugar se deriva de dos fuentes que varían por llamada y por proceso:
 *   - el TSC del CPU (sys_0p(SYS_READ_TSC)) que avanza constantemente, le da aleatoridad a la espera activa
 *   - el pid: distingue a cada proceso.
 */
static void active_wait_random(void) {
	uint32_t tsc = (uint32_t) sys_0p(SYS_READ_TSC);
	uint32_t r = (tsc ^ ((uint32_t) getpid() << 16)) * 2654435761u;
	uint32_t spins = (r >> 8) % 8000000u + 1000000u;
	for (volatile uint32_t i = 0; i < spins; i++)
		;
}

/* Escritor: escribe su letra única una y otra vez. */
static int mvar_writer(int argc, char *argv[]) {
	(void) argc;
	char letter = argv[1][0];
	int empty = sem_open(SEM_EMPTY, 1);
	int full = sem_open(SEM_FULL, 0);
	while (1) {
		active_wait_random();
		sem_wait(empty); /* esperar a que la caja esté vacía */
		box = letter;
		sem_post(full); /* avisar que hay un valor */
	}
	return 0;
}

/* Lector: consume el valor e imprime con su color único. */
static int mvar_reader(int argc, char *argv[]) {
	(void) argc;
	uint32_t color = reader_colors[argv[1][0] - '0'];
	int empty = sem_open(SEM_EMPTY, 1);
	int full = sem_open(SEM_FULL, 0);
	while (1) {
		active_wait_random();
		sem_wait(full); /* esperar a que haya un valor */
		char c = box;
		set_text_color(color);
		char s[2] = {c, 0};
		puts(s);
		sem_post(empty); /* avisar que la caja quedó vacía */
	}
	return 0;
}

int mvar_main(int argc, char *argv[]) {
	if (argc != 3) {
		printf("uso: mvar <escritores> <lectores>\n");
		return -1;
	}

	int nw = (int) str_to_uint(argv[1]);
	int nr = (int) str_to_uint(argv[2]);
	if (nw <= 0 || nr <= 0 || nw > MAX_WRITERS || nr > MAX_READERS) {
		printf("mvar: cantidades invalidas (1..%d escritores, 1..%d lectores)\n", MAX_WRITERS, MAX_READERS);
		return -1;
	}

	/* La caja arranca vacía. Los semáforos los abren los propios hijos
	 * (ver SEM_EMPTY/SEM_FULL): así el refcount lo tienen ellos y al matarlos
	 * el kernel los libera, evitando reusar estado viejo entre corridas. */
	box = 0;

	/* Crear escritores: cada uno con una letra distinta ('A', 'B', ...). */
	for (int i = 0; i < nw; i++) {
		child_arg[i][0] = (char) ('A' + i);
		child_arg[i][1] = 0;
		child_argv[i][0] = "writer";
		child_argv[i][1] = child_arg[i];
		child_argv[i][2] = (void *) 0;
		create_process(mvar_writer, 2, child_argv[i]);
	}

	/* Crear lectores: cada uno con un índice de color distinto. */
	for (int i = 0; i < nr; i++) {
		int k = nw + i;
		child_arg[k][0] = (char) ('0' + i);
		child_arg[k][1] = 0;
		child_argv[k][0] = "reader";
		child_argv[k][1] = child_arg[k];
		child_argv[k][2] = (void *) 0;
		create_process(mvar_reader, 2, child_argv[k]);
	}

	/* El proceso principal termina inmediatamente. */
	return 0;
}
