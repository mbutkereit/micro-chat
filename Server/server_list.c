#include "server_list.h"

// Erstellen der Liste mit hilfe der Makros aus dem Queues Header.
LIST_HEAD(server_listhead, server_list)
server_list_head = LIST_HEAD_INITIALIZER(server_list_head);
struct server_listhead *head_list_p; //List Head.

int server_list_usage_size = 0;
pthread_mutex_t lock_server_list;

extern fd_set readfds;
extern int highest_fd;

/**
 *
 */
void init_server_list() {
	LIST_INIT(&server_list_head);
	if (pthread_mutex_init(&lock_server_list, NULL) != 0) {
		perror("pthread_mutex_init");
		fprintf(stderr, "Mutex init fehler in der list. \n");
		exit(EXIT_FAILURE);
	}
}

/*
 *Löscht einen Socket/Server aus der Serverliste
 */
void remove_from_Serverlist_by_Socket(int SocketFD, int notify) {
	server_list* serverlist = NULL;
	server_list* remEntry = NULL;
	pthread_mutex_lock(&lock_server_list);
	LIST_FOREACH(serverlist, &server_list_head, entries_server)
	{
		if (SocketFD == serverlist->connection_item->socketFD) {
			remEntry = serverlist;
		}
	}

	if (highest_fd == remEntry->connection_item->socketFD) {
		highest_fd = find_next_smaller_socket_id();
	}
	FD_CLR(remEntry->connection_item->socketFD, &readfds);
	LIST_REMOVE(remEntry, entries_server);
	server_list_usage_size--;
	fprintf(stderr,"Der Server %d wurde aus der Serverliste gelöscht",SocketFD);

	pthread_mutex_unlock(&lock_server_list);
	if (notify) {
		notify_all_by_update();
	}
}
/**
 *
 */
void add_to_server_list(connection_item* item) {
	server_list *element = (server_list *) malloc(sizeof(server_list));
	element->connection_item = item;
	pthread_mutex_lock(&lock_server_list);
	char status = 1;
	server_list* serverlist = NULL;
	LIST_FOREACH(serverlist, &server_list_head, entries_server)
	{
		if (item->socketFD == serverlist->connection_item->socketFD) {
			status = 0;
		}
	}

	if (status) {
		FD_SET(item->socketFD, &readfds);
		if (highest_fd < item->socketFD) {
			highest_fd = item->socketFD;
		}
		server_list_usage_size++;
		LIST_INSERT_HEAD(&server_list_head, element, entries_server);
	}

	pthread_mutex_unlock(&lock_server_list);
}

/**
 *
 */
void remove_from_server_list(server_list* entry) {
	pthread_mutex_lock(&lock_server_list);
	if (highest_fd == entry->connection_item->socketFD) {
		highest_fd = find_next_smaller_socket_id();
	}
	FD_CLR(entry->connection_item->socketFD, &readfds);
	LIST_REMOVE(entry, entries_server);
	server_list_usage_size--;
	pthread_mutex_unlock(&lock_server_list);
}

/**
 * Informiert alle verbindung die mehr als 0 hops haben sprich server ueber neue benutzer.
 * Todo nach vorgabe im Protokoll verteilen.
 */
void notify_all_by_update() {

	server_list* serverlist = NULL;
	LIST_FOREACH(serverlist, &server_list_head, entries_server)
	{
		sendControllInfo(serverlist->connection_item, NO_FLAGS);
	}
}

void check_event_list(fd_set* test) {
	server_list* serverlist = NULL;

// Server events queuen.
	pthread_mutex_lock(&lock_server_list);
	LIST_FOREACH(serverlist, &server_list_head, entries_server)
	{
		if (FD_ISSET(serverlist->connection_item->socketFD, test)
				&& serverlist->connection_item->status == 0) {

			fprintf(stderr, "IN check_event_list  %d\n",
					serverlist->connection_item->socketFD);
			serverlist->connection_item->status = QUEUED;
			enqueue(serverlist->connection_item);

		}

	}
	pthread_mutex_unlock(&lock_server_list);
}
