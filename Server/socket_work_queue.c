#include "socket_work_queue.h"

static sem_t *queue_up_sem;
static sem_t *queue_down_sem;

queue* _connectionQueue = NULL;
queue* _connectionQueueFront = NULL;
pthread_mutex_t lock_queue;

void init_queue() {

	// Mac's don't ask.
    sem_unlink(QUEUE_UP_SEM);
	 // Notwendig falls das Programm mal abstuertzt.
    sem_unlink(QUEUE_DOWN_SEM);

	queue_up_sem = sem_open(QUEUE_UP_SEM, O_CREAT | O_EXCL, 0777, 0);
	if (queue_up_sem == SEM_FAILED) {
        fprintf(stderr, "\n mutex init failed\n");
        exit(EXIT_FAILURE);
        
	}

	queue_down_sem = sem_open(QUEUE_DOWN_SEM, O_CREAT | O_EXCL, 0777, 6);
	if (queue_down_sem == SEM_FAILED) {
        fprintf(stderr, "\n mutex init failed\n");
        exit(EXIT_FAILURE);
	}

	if (pthread_mutex_init(&lock_queue, NULL) != 0) {
		fprintf(stderr, "\n mutex init failed\n");
		exit(EXIT_FAILURE);
	}
}

queue* enqueue(connection_item* item) {
	sem_wait(queue_down_sem);
	pthread_mutex_lock(&lock_queue);
	queue* element = _connectionQueue;
	while (element != NULL && element->next != NULL) {
		element = element->next;
	}



	queue* new_item = (queue*) malloc(sizeof(queue));
	new_item->value = item;
	new_item->next = NULL;

	if (_connectionQueueFront == NULL && _connectionQueue == NULL) {
		_connectionQueueFront = _connectionQueue = new_item;
		pthread_mutex_unlock(&lock_queue);
		sem_post(queue_up_sem);
		return new_item;
	}
	_connectionQueue->next = new_item;
	_connectionQueue = new_item;
	pthread_mutex_unlock(&lock_queue);
	sem_post(queue_up_sem);
	return new_item;
}

connection_item* dequeue() {
	sem_wait(queue_up_sem);
	pthread_mutex_lock(&lock_queue);
	queue* temp = _connectionQueueFront;

	if (_connectionQueueFront == NULL) {
		printf("Queue is Empty\n");
		pthread_mutex_unlock(&lock_queue);
		sem_post(queue_down_sem);
		return NULL;
	}
	if (_connectionQueueFront == _connectionQueue) {
		_connectionQueueFront = NULL;
		_connectionQueue = NULL;
	} else {
		_connectionQueueFront = _connectionQueueFront->next;
	}
	connection_item* temp_value = temp->value;
	free(temp);
	pthread_mutex_unlock(&lock_queue);
	sem_post(queue_down_sem);
	return temp_value;
}

void cleanup_queue(void) {
    sem_unlink(QUEUE_UP_SEM);
    sem_unlink(QUEUE_DOWN_SEM);
    sem_close(queue_up_sem);
    sem_close(queue_down_sem);
	pthread_mutex_destroy(&lock_queue);
}

