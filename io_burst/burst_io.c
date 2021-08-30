#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include<signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define READ 0
#define WRITE 1
#define RW 2

struct option  options[] = {
        { "disks", required_argument, NULL, 'd' },
        { "threads", required_argument, NULL, 't' },
        { "run_time", required_argument, NULL, 'r' },
        { "sleep_time", required_argument, NULL, 's' },
        { "active_time", required_argument, NULL, 'a' },
        { "opt_type", required_argument, NULL, 'o' },
        { "io_size", required_argument, NULL, 'i' },
	{ "usage", no_argument, NULL, 'h'},
	{0, 0, 0, 0}
};

void
usage(char *cmd)
{
	printf("Usage %s:\n", cmd);
	printf("  --disks <disk1>,<disk2>  Comma separated list of disks to use\n");
	printf("  --threads <# threads>,<# threads>  Comma separated list of threads to use\n");
	printf("  --run_time <seconds>  How long the test is to run for.\n");
	printf("  --sleep_time <seconds>  How long to sleep between bursts.\n");
	printf("  --active_time <seconds>  How long to to be active for before sleeping\n");
	printf("  --opt_type read/write/rw  Type of io to do.\n");
	printf("  --io_size <size>  Size of IO\n");
	printf("  --usage This usage message\n");
	exit(0);
}

struct 	test_args {
	char *disks;
	int index;
	int run_time;
	int sleep_time;
	int active_time;
	int threads;
	int oper_type;
	int io_size;
	pthread_t tids;
	pthread_attr_t tattr;
	long long *ios_done;
	char *disk_using;
	pthread_barrier_t *barrier;
};

pthread_barrier_t wait_barrier;

volatile int io_done = 0;

void *
io_interval(void *passed_args)
{
	struct test_args *test_args;

	test_args = (struct test_args *) passed_args;

	for (;;) {
		pthread_barrier_wait(&wait_barrier);
		sleep(test_args->active_time);
		io_done = 1;
	}
}

void *
perform_io(void *passed_args)
{

	int fd;
	int ios_done;
	int interval;
	char *buffer;
	time_t start_time, end_time, current_time = 0, run_endtime;
	struct test_args *test_args;

	test_args = (struct test_args *) passed_args;
	
	buffer = (char *) calloc(1, test_args->io_size);
	fd = open(test_args->disks, O_RDWR);
	if (fd == 0) {
		perror(test_args->disks);
		exit (1);
	}
	interval = 0;
	pthread_barrier_wait(test_args->barrier);
	start_time = time(NULL);
	end_time = start_time + test_args->run_time;
	while (end_time > current_time) {
		if ( test_args->index == 0 ) {
			pthread_barrier_wait(&wait_barrier);
		}
		io_done = 0;
		while (io_done == 0) {
			if ( test_args->oper_type == READ) {
				if (read(fd, buffer, test_args->io_size) != test_args->io_size) {
					lseek(fd, SEEK_SET, 0);
					continue;
				}
				ios_done++;
				continue;
			}
			if ( test_args->oper_type == WRITE) {
				if (write(fd, buffer, test_args->io_size) != test_args->io_size) {
					lseek(fd, SEEK_SET, 0);
					continue;
				}
				ios_done++;
				continue;
			}
			if ( test_args->oper_type == RW) {
				if (read(fd, buffer, test_args->io_size) != test_args->io_size) {
					lseek(fd, SEEK_SET, 0);
					continue;
				}
				ios_done++;
				if (write(fd, buffer, test_args->io_size) != test_args->io_size) {
					lseek(fd, SEEK_SET, 0);
					continue;
				}
				ios_done++;
			}
		}
		test_args->ios_done[interval++] = ios_done;
		ios_done = 0;
		if ( test_args->index == 0 ) {
			sleep(test_args->sleep_time);
		}
		pthread_barrier_wait(test_args->barrier);
		current_time = time(NULL);
	}
	close(fd);
	free(buffer);
	pthread_exit(NULL);
}

long
convert_size(char *size_string)
{
	long size=atol(size_string);
	if (strchr(size_string, 'k')  || strchr(size_string, 'K')) {
		return (size * 1024);
	}
	if (strchr(size_string, 'm')  || strchr(size_string, 'M')) {
		return (size * 1024 * 1024);
	}
	return (size);
}


int
main(int argc, char **argv)
{
	int option_index = 0;
	char opt;
	char *opt_type = "read";
	char *opt_size = "4k";
	pthread_t tid;
	pthread_attr_t tattr;
	struct test_args test_args;
	struct test_args *run_info;
	int total_threads=1;
	int count;
	int count1;
	char *disk_to_use[128];
	char *ptr;
	char *ptr1;
	char *threads = "5";
	char *tptr;
	char *tptr1;
	int disk_count=0;
	int interval;
	pthread_barrier_t barrier;
	int pass=0;

	test_args.disks = NULL;
	test_args.threads = 5;
	test_args.run_time = 1200;
	test_args.sleep_time = 60;
	test_args.active_time = 60;
	test_args.sleep_time = 60;

	while ((opt = getopt_long (argc, argv, "d:", options, &option_index)) != -1) {
		switch (opt) {
			case 'd':
				test_args.disks=strdup(optarg);
			break;
			case 'i':
				opt_size=strdup(optarg);
			break;
			case 'o':
				opt_type=strdup(optarg);
			break;
			case 'a':
				test_args.active_time=atoi(optarg);
			break;
			case 'r':
				test_args.run_time=atoi(optarg);
			break;
			case 's':
				test_args.sleep_time=atoi(optarg);
			break;
			case 't':
				threads = optarg;
			break;
			case 'h':
				usage(argv[0]);
			break;
			case '?':
				printf("%c Unkown option\n", opt);
				exit(1);
			break;
			default:
				printf("%c Unkown option\n", opt);
				exit(1);
			break;
		}
		/*
		 * Encountered a bug, workaround.
		 * How long is this required?
		 */
		if (optind == argc)
			break;
	}
	if ( test_args.disks == NULL) {
		printf("Need to designate at least one disk\n");
		exit(1);
	}
	if ( strcmp(opt_type, "read") == 0 )
		test_args.oper_type = READ;
	if ( strcmp(opt_type, "write") == 0 )
		test_args.oper_type = WRITE;
	if ( strcmp(opt_type, "rw") == 0 )
		test_args.oper_type = RW;
	tptr = threads;
	printf("IO requested: %s\n", opt_type);
	printf("IO size: %s\n", opt_size);
	printf("Run time: %d\n", test_args.run_time);
	printf("Sleep time: %d\n", test_args.sleep_time);
	printf("Active time: %d\n", test_args.active_time);

	for (;;) {
		tptr1 = strchr(tptr, ',');
		test_args.threads = atoi(tptr);
		total_threads = test_args.threads;
		ptr=test_args.disks;
		if (disk_count == 0) {
			for (;;) {
				ptr1 = strchr(ptr, ',');
				if (ptr1)
					ptr1[0] = '\0';
				disk_to_use[disk_count++] = strdup(ptr);
				if (!ptr1)
					break;
				ptr = ptr1 + 1;
			}
		}
		total_threads = total_threads * disk_count;

		printf("total threads: %d\n", total_threads);
		test_args.io_size = convert_size(opt_size);
 		pthread_barrier_init (&barrier, NULL, total_threads);
	
		run_info = (struct test_args *) calloc(total_threads, sizeof(struct test_args));
		interval = (test_args.run_time + (test_args.sleep_time + test_args.active_time -1))/(test_args.sleep_time + test_args.active_time);
		if (pass == 0) {
			pthread_attr_init(&tattr);
			test_args.index = 
 			pthread_barrier_init (&wait_barrier, NULL, 2);
			pthread_create(&tid, &run_info[0].tattr, io_interval, &test_args);
		}
		pass++;
		for (count = 0; count < total_threads;  count++) {
			memcpy(&run_info[count], &test_args, sizeof(struct test_args));
			pthread_attr_init(&run_info[count].tattr);
			run_info[count].disks = disk_to_use[count % disk_count];
			run_info[count].barrier = &barrier;
			run_info[count].ios_done = (long long *) calloc(interval, sizeof (long long));
		}
		for (count = 0; count < total_threads;  count++) {
			run_info[count].index = count;
			pthread_create(&run_info[count].tids, &run_info[count].tattr, perform_io, &run_info[count]);
		}

		for (count = 0; count < total_threads;  count++) {
			pthread_join(run_info[count].tids, NULL);
		}
		printf("time period");
		for (count1 = 0; count1 < total_threads;  count1++) {
			printf(":%s", run_info[count1].disks);
		}
		printf("\n");
		for (count = 0; count < interval; count++) {
			printf("%d", count);
			for (count1 = 0; count1 < total_threads;  count1++) {
				printf(":%ld", run_info[count1].ios_done[count]);
			}
			printf("\n");
		}
		free (run_info);
		if (tptr1 == NULL)
			break;
		tptr = tptr + 2;
	}
	return (0);
}


