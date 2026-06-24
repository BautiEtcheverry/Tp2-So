#include "../../../include/libc.h"

#define MAX_WRITERS 8
#define MAX_READERS 8
#define MAX_CHILDREN (MAX_WRITERS + MAX_READERS)
#define BUF_SIZE 32 /* buffer circular pequeño */
#define MAX_PRIO 3	/* 0 = mayor prioridad (más quantums), 3 = menor */

/*
 * Base del sleep del escritor. Un escritor con prio P duerme
 * PRIO_UNIT*(P+1) + jitter spins entre letras:
 *   prio 0 (mayor) → sleep corto  → más letras/seg
 *   prio 3 (menor) → sleep largo  → menos letras/seg
 * El jitter (LCG, semilla distinta por escritor, explicado abajo) rompe la sincronización
 * entre escritores de igual prioridad → salida mezclada y no ABABAB.
 */
#define PRIO_UNIT 10000000u

/* Semaforores que usamos:
 *   SEM_DATA  (init 0)        — items listos para leer
 *   SEM_SPACE (init BUF_SIZE) — slots libres para escribir
 *   SEM_WMUX  (init 1)        — mutex sobre write_pos (entre escritores)
 *   SEM_RMUX  (init 1)        — turno exclusivo de lectura (entre lectores)
 *
 * Escritor: consulta su prioridad, escribe 1 letra, espera activa proporcional
 *   a la prioridad. prio 0 → 1×PRIO_UNIT spins → escribe más letras/seg.
 *
 * Lector: toma rmux (turno) antes de sem_wait(data). Suelta rmux y el
 *   otro lector bloqueado en rmux recibe el turno por FIFO → alternación
 *   estricta sin depender del scheduling.
 */

/* Paleta de colores POR LECTOR (colo identifica al lector; letra al escritor). */
static const uint32_t reader_colors[MAX_READERS] = {
	0xFF5555, /* rojo     */
	0x55FF55, /* verde    */
	0x5599FF, /* azul     */
	0xFFFF55, /* amarillo */
	0xFF55FF, /* magenta  */
	0x55FFFF, /* cian     */
	0xFFFFFF, /* blanco   */
	0xFF9933, /* naranja  */
};

static char shared_buf[BUF_SIZE];
static int write_pos;
static int read_pos;

#define SEM_DATA "mvar_data"
#define SEM_SPACE "mvar_space"
#define SEM_WMUX "mvar_wmux"
#define SEM_RMUX "mvar_rmux"

static char child_arg[MAX_CHILDREN][2];
static char *child_argv[MAX_CHILDREN][3];
static int64_t child_pids[MAX_CHILDREN];
static int n_children;

/* Devuelve la prioridad del proceso, -1 si no se encontró en la tabla. */
static int get_own_priority(void) {
	ProcessInfo procs[32];
	int n = get_processes(procs, 32);
	uint64_t my_pid = (uint64_t) getpid();
	for (int i = 0; i < n; i++)
		if (procs[i].pid == my_pid)
			return procs[i].priority;
	return -1;
}

static void active_wait(uint32_t spins) {
	for (volatile uint32_t i = 0; i < spins; i++)
		;
}

/*
 * Escritores: 1 letra por ciclo, hacen un sleep = PRIO_UNIT*(prio+1) + jitter.
 * El LCG con semilla distinta por escritor (letter-'A'+1) desincroniza
 * a escritores de igual prioridad → salida mezclada en vez de ABABAB.
 * prio 0 → sleep ≈ PRIO_UNIT; prio 3 → sleep ≈ 4×PRIO_UNIT.
 */
/* LCG(Linear Congruential Generator, https://rosettacode.org/wiki/Linear_congruential_generator)
	es un algortimo que nos permite hacer aleatoria la espera de cada escritor.
	Funciona poniendo una seed, que es un numero incial, y dos constantes. En nuestro caso como a cada proceso le
	asignamos una letra usamos de seed el numero (letra-A+1) osea el indice de escritor empezando en 1(porque al primer
	proceso se le da la A).
*/
static int mvar_writer(int argc, char *argv[]) {
	(void) argc;
	char letter = argv[1][0];
	int data = sem_open(SEM_DATA, 0);
	int space = sem_open(SEM_SPACE, BUF_SIZE);
	int wmux = sem_open(SEM_WMUX, 1);
	int my_prio = 0;
	uint32_t rng = (uint32_t) (letter - 'A' + 1); // Esta es la seed, para despues aplicar las operaciones de abajo y
												  // obtener un numero pseudo aleatorio.
	while (1) {
		int p = get_own_priority();
		if (p >= 0 && p <= MAX_PRIO)
			my_prio = p;
		sem_wait(space);
		sem_wait(wmux);
		shared_buf[write_pos] = letter;
		write_pos = (write_pos + 1) % BUF_SIZE;
		sem_post(wmux);
		sem_post(data);
		rng = rng * 1664525u + 1013904223u; /* LCG estándar 1664525 y 1013904223 son ctes que se suelen usar para este
											   calculo, podrian ser otras */
		active_wait(PRIO_UNIT * (uint32_t) (my_prio + 1) + rng % (PRIO_UNIT / 2));
	}
	return 0;
}

/*
 * Lector: sem_wait(rmux) PRIMERO para turno exclusivo, luego sem_wait(data).
 * El active_wait va DESPUÉS de liberar rmux: el otro lector puede leer mientras
 * este duerme, reflejando la prioridad (prio baja → duerme más → lee menos).
 * Un `nice` sobre el lector surte efecto en el siguiente ciclo.
 */
static int mvar_reader(int argc, char *argv[]) {
	(void) argc;
	uint32_t color = reader_colors[argv[1][0] - '0'];
	int data = sem_open(SEM_DATA, 0);
	int space = sem_open(SEM_SPACE, BUF_SIZE);
	int rmux = sem_open(SEM_RMUX, 1);
	int my_prio = 0;
	while (1) {
		int p = get_own_priority();
		if (p >= 0 && p <= MAX_PRIO)
			my_prio = p;
		sem_wait(rmux);
		sem_wait(data);
		char c = shared_buf[read_pos];
		read_pos = (read_pos + 1) % BUF_SIZE;
		sem_post(rmux); /* liberar turno antes de dormir */
		sem_post(space);
		if (c >= 'A' && c < 'A' + MAX_WRITERS) {
			set_text_color(color);
			char s[2] = {c, 0};
			puts(s);
		}
		/* prio 0 → 1×PRIO_UNIT (lee más); prio 3 → 4×PRIO_UNIT (lee menos) */
		active_wait(PRIO_UNIT * (uint32_t) (my_prio + 1));
	}
	return 0;
}

/* Mata los hijos de la tanda anterior */
static void stop_children(void) {
	for (int i = 0; i < n_children; i++)
		if (child_pids[i] > 0)
			kill((uint64_t) child_pids[i]);
	n_children = 0;
}

static int64_t spawn_child(int slot, int (*fn)(int, char **), const char *name, char arg) {
	child_arg[slot][0] = arg;
	child_arg[slot][1] = 0;
	child_argv[slot][0] = (char *) name;
	child_argv[slot][1] = child_arg[slot];
	child_argv[slot][2] = (void *) 0;
	return create_process(fn, 2, child_argv[slot]);
}

int mvar_main(int argc, char *argv[]) {
	stop_children();

	if (argc == 2 && streq_nocase(argv[1], "stop"))
		return 0;

	if (argc != 3) {
		printf("usage: mvar <writers> <readers>   |   mvar stop\n");
		return -1;
	}

	int nw = (int) str_to_uint(argv[1]);
	int nr = (int) str_to_uint(argv[2]);
	if (nw <= 0 || nr <= 0 || nw > MAX_WRITERS || nr > MAX_READERS) {
		printf("mvar: invalid counts (1..%d writers, 1..%d readers)\n", MAX_WRITERS, MAX_READERS);
		return -1;
	}

	for (int i = 0; i < BUF_SIZE; i++)
		shared_buf[i] = ' ';
	write_pos = 0;
	read_pos = 0;
	n_children = 0;

	for (int i = 0; i < nw; i++) {
		int slot = n_children;
		child_pids[slot] = spawn_child(slot, mvar_writer, "writer", (char) ('A' + i));
		n_children++;
	}

	for (int i = 0; i < nr; i++) {
		int slot = n_children;
		child_pids[slot] = spawn_child(slot, mvar_reader, "reader", (char) ('0' + i));
		n_children++;
	}
	return 0;
}
