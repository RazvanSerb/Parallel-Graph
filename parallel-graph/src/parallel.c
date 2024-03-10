// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t sumMutexLock;

/* TODO: Define graph task argument. */
typedef struct os_graph_arg_t {
	unsigned int idx;
	void (*func)(unsigned int idx);
} os_graph_arg_t;

static void action(void *arg)
{
	((os_graph_arg_t *)arg)->func(((os_graph_arg_t *)arg)->idx);
}

static void destroy_arg(void *arg)
{
	free(arg);
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */
	os_node_t *node;

	node = graph->nodes[idx];
	pthread_mutex_lock(&sumMutexLock);
	sum += node->info;
	pthread_mutex_unlock(&sumMutexLock);
	graph->visited[idx] = DONE;
	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		pthread_mutex_lock(&tp->taskQueueMutex);
		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			graph->visited[node->neighbours[i]] = PROCESSING;
			void *argument;

			argument = malloc(sizeof(os_graph_arg_t));
			DIE(argument == NULL, "malloc");
			((os_graph_arg_t *)argument)->idx = node->neighbours[i];
			((os_graph_arg_t *)argument)->func = &process_node;
			enqueue_task(tp, create_task(&action, argument, &destroy_arg));
		}
		pthread_mutex_unlock(&tp->taskQueueMutex);
	}
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	DIE(pthread_mutex_init(&sumMutexLock, NULL) != 0, "mutex");
	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);
	pthread_mutex_destroy(&sumMutexLock);
	printf("%d", sum);

	return 0;
}
