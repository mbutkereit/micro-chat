#ifndef socket_work_queue_h
#define socket_work_queue_h

#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include "Client.h"

#define QUEUE_DOWN_SEM    "sem_vm_simulation_OS_X" //!< For OS-X semaphore
#define QUEUE_UP_SEM       "sem_vm_simulation_OS_X_n" //!< For OS-X semaphore

struct queue {
	connection_item* value;
	struct queue* next;
};

typedef struct queue queue;

// Liste der Prototypen.
queue* enqueue(connection_item* item);
void init_queue();
connection_item* dequeue();
void cleanup_queue(void);

#endif /* socket_work_queue_h */
