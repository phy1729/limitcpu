#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BILLION 1000000000
#define PID_MAX 99999

static void cont_and_exit(int);

pid_t *pids;
size_t npids;

int
main(int argc, char *argv[]) {
	struct timespec timeout[2], remainder;
	const char *errstr;
	unsigned long long cycle_time = BILLION, timeout_len;
	size_t j;
	int ch;
	unsigned char duty_cycle = 50, i = 0;

	pledge("stdio proc", NULL);

	while ((ch = getopt(argc, argv, "c:d:n:")) != -1) {
		switch (ch) {
		case 'c':
			cycle_time = strtonum(optarg, 1, ULLONG_MAX / BILLION, &errstr);
			if (errstr != NULL)
				errx(1, "cycle time is %s: %s", errstr, optarg);
			cycle_time *= BILLION;
			break;
		case 'd':
			duty_cycle = strtonum(optarg, 1, 99, &errstr);
			if (errstr != NULL)
				errx(1, "duty cycle is %s: %s", errstr, optarg);
			break;
		case 'n':
			cycle_time = strtonum(optarg, 1, LLONG_MAX, &errstr);
			if (errstr != NULL)
				errx(1, "cycle time is %s: %s", errstr, optarg);
			break;
		default:
			errx(1, "invalid flag %c", ch);
		}
	}
	argc -= optind;
	argv += optind;

	for (i = 0; i < 2; i++) {
		timeout_len = cycle_time * (i ? 100 - duty_cycle : duty_cycle) / 100;
		timeout[i].tv_sec = timeout_len / BILLION;
		timeout[i].tv_nsec = timeout_len % BILLION;
	}

	npids = argc;
	if ((pids = reallocarray(NULL, npids, sizeof(pid_t))) == NULL)
		err(1, "malloc");

	for (j = 0; j < npids; j++) {
		pids[j] = strtonum(argv[j], -PID_MAX, PID_MAX, &errstr);
		if (errstr != NULL)
			errx(1, "pid is %s: %s", errstr, argv[j]);
	}

	signal(SIGHUP, cont_and_exit);
	signal(SIGINT, cont_and_exit);

	for (i = 0; 1; i = (i + 1) % 2) {
		nanosleep(&timeout[i], &remainder);
		for (j = 0; j < npids; j++) {
			if (kill(pids[j], i ? SIGCONT : SIGSTOP) != 0)
				err(1, "kill");
		}
	}
}

static void
cont_and_exit(int sigraised) {
	size_t i;

	for (i = 0; i < npids; i++)
		kill(pids[i], SIGCONT);
	signal(sigraised, SIG_DFL);
	kill(getpid(), sigraised);
}
