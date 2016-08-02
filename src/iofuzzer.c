/** @file */

#include "array.h"
#include "iofuzzer.h"
#include "random.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <unistd.h>

#define MAXPORT 0xffff

#define usage() \
	fprintf(stderr, "Usage: %s [options]\n", PROGRAM_NAME)

#define version() \
	fprintf(stderr, "%s (%s) %s\n", PROGRAM_NAME, PACKAGE_NAME, PROGRAM_VERSION)

static int debug = 0;
static char *output = NULL;
static char *ports = NULL;
static int quiet = 0;
static random_t *_random = NULL;
static char state[8] = {0};
static int verbose = 0;

static array_t *
iofuzzer_parse_ports(char *string)
{
	array_t *ports;
	char *str;
	char *ptr;
	char *last;

	if (string == NULL) {
		errno = EINVAL;
		return NULL;
	}

	ports = array_new(sizeof(unsigned long));
	if (ports == NULL)
		goto err;

	str = strdup(string);
	ptr = str;
	for (str = strtok_r(str, ",", &last); str != NULL; str = strtok_r(NULL, ",", &last)) {
		char *substr;
		char *ptr;
		char *last;
		unsigned long begin;
		unsigned long end;
		unsigned long port;

		substr = strdup(str);
		ptr = substr;
		substr = strtok_r(substr, "-", &last);
		if (substr != NULL) {
			errno = 0;
			begin = strtoul(substr, NULL, 0);
			if (errno == EINVAL) {
				free(ptr);
				goto err;
			}

			end = begin;
			substr = strtok_r(NULL, "-", &last);
			if (substr != NULL) {
				errno = 0;
				end = strtoul(substr, NULL, 0);
				if (errno == EINVAL) {
					free(ptr);
					goto err;
				}

				if (end > MAXPORT)
					end = MAXPORT;
			}

			for (port = begin; port <= end; port++)
				array_append_val(ports, &port);
		}

		free(ptr);
	}

	free(ptr);

	return ports;

err:
	free(ptr);

	return NULL;
}

static void *
thread_start(void *arg)
{
	unsigned long thread_num = (unsigned long)arg;
	FILE *stream;
	iofuzzer_t *fuzzer;
	uintptr_t *variates;
	size_t length;
	char state[8] = {0};
	char *names[] = { "inb", "inw", "inl", "insb", "insw", "insl", "outb", "outw", "outl", "outsb", "outsw", "outsl" };
	int i;

	stream = stdout;
	if (output != NULL) {
		stream = fopen(output, "a+");
		if (stream == NULL) {
			perror("fopen");
			goto err;
		}
	}

	fuzzer = iofuzzer_new();
	if (fuzzer == NULL) {
		perror("iofuzzer_new");
		goto err;
	}

	iofuzzer_set_ports(fuzzer, iofuzzer_parse_ports(ports));
	array_unref(iofuzzer_get_ports(fuzzer));
	iofuzzer_set_random(fuzzer, _random);
	variates = &array_index(iofuzzer_get_variates(fuzzer), uintptr_t, 0);
	length = array_get_length(iofuzzer_get_variates(fuzzer));
	for (;;) {
		flockfile(stream);
		fprintf(stream, "%d,", (unsigned int)time(NULL));
		fprintf(stream, "%d,", (unsigned int)thread_num);
		iofuzzer_get_state(fuzzer, state, sizeof(state));
		fprintf(stream, "%#llx,", *((unsigned long long *)state));
		fprintf(stream, "%s,", names[(unsigned int)variates[0]]);
		for (i = 1; i < (length - 1); i++)
			fprintf(stream, "%#x,", (unsigned int)variates[i]);

		fprintf(stream, "%#x\n", (unsigned int)variates[i]);
		fflush(stream);
		fsync(fileno(stream));
		iofuzzer_iterate(fuzzer);
		funlockfile(stream);
	}

	iofuzzer_unref(fuzzer);
	fclose(stream);

	pthread_exit((void *)EXIT_SUCCESS);

err:
	iofuzzer_unref(fuzzer);
	fclose(stream);

	pthread_exit((void *)EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	enum {
		OPT_DEBUG = CHAR_MAX + 1,
		OPT_HELP,
		OPT_NUM_THREADS,
		OPT_OUTPUT,
		OPT_PORTS,
		OPT_QUIET,
		OPT_SILENT,
		OPT_STACK_SIZE,
		OPT_STATE,
		OPT_VERBOSE,
		OPT_VERSION,
	};
	static struct option longopts[] = {
		{"debug",       no_argument,       NULL, 'd'             },
		{"help",        no_argument,       NULL, 'h'             },
		{"num-threads", required_argument, NULL, OPT_NUM_THREADS },
		{"output",      required_argument, NULL, 'o'             },
		{"ports",       required_argument, NULL, 'p'             },
		{"quiet",       no_argument,       NULL, 'q'             },
		{"silent",      no_argument,       NULL, 'q'             },
		{"stack-size",  required_argument, NULL, OPT_STACK_SIZE  },
		{"state",       required_argument, NULL, OPT_STATE       },
		{"verbose",     no_argument,       NULL, 'v'             },
		{"version",     no_argument,       NULL, OPT_VERSION     },
		{NULL,          0,                 NULL, 0               }
	};
	static int longindex = 0;
	int c;
	unsigned long num_threads = 1;
	size_t stack_size = 0;
	pthread_attr_t attr;
	unsigned long thread_num;
	pthread_t thread;

	while ((c = getopt_long(argc, argv, "dho:p:qv", longopts, &longindex)) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;

		case 'h':
			usage();
			exit(EXIT_FAILURE);

		case 'o':
			output = optarg;
			break;

		case 'p':
			ports = optarg;
			break;

		case 'q':
			quiet = 1;
			break;

		case 'v':
			verbose = 1;
			break;

		case OPT_NUM_THREADS:
			num_threads = strtoul(optarg, NULL, 0);
			break;

		case OPT_STACK_SIZE:
			stack_size = strtoul(optarg, NULL, 0);
			break;

		case OPT_STATE:
			*((unsigned long long *)state) = strtoull(optarg, NULL, 0);
			break;

		case OPT_VERSION:
			version();
			exit(EXIT_FAILURE);

		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}

	if (iopl(3) == -1) {
		perror("iopl");
		exit(EXIT_FAILURE);
	}

	if (!quiet) {
		int secs = 3;

		fprintf(stderr, "Warning: This program may cause data loss.\n");
		fprintf(stderr, "Press Ctrl+C to interrupt\n");
		do {
			fprintf(stderr, "Starting in %d secs...\r", secs);
			fflush(stderr);
			sleep(1);
		} while (secs--);
	}

	_random = random_new_with_state(state, sizeof(state));
	if (_random == NULL) {
		perror("random_new_with_state");
		exit(EXIT_FAILURE);
	}

	errno = pthread_attr_init(&attr);
	if (errno != 0) {
		perror("pthread_attr_init");
		exit(EXIT_FAILURE);
	}

	errno = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (errno != 0) {
		perror("pthread_attr_setdetachstate");
		exit(EXIT_FAILURE);
	}

	if (stack_size != 0) {
		errno = pthread_attr_setstacksize(&attr, stack_size);
		if (errno != 0) {
			perror("pthread_attr_setstacksize");
			exit(EXIT_FAILURE);
		}
	}

	for (thread_num = 1; thread_num < num_threads; thread_num++) {
		errno = pthread_create(&thread, &attr, &thread_start, (void *)thread_num);
		if (errno != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	errno = pthread_attr_destroy(&attr);
	if (errno != 0) {
		perror("pthread_attr_destroy");
		exit(EXIT_FAILURE);
	}

	thread_start((void *)0);

	pthread_exit((void *)EXIT_SUCCESS);
}
