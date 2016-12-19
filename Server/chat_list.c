#include "chat_list.h"

// Erstellen der Liste mit hilfe der Makros aus dem Queues Header.
LIST_HEAD(chat_listhead, controll_info_list)
chat_list_head = LIST_HEAD_INITIALIZER(chat_list_head);
struct chat_listhead *headp; //List Head.

int controll_info_usage_size = 0;
pthread_mutex_t lock_list;

extern fd_set readfds; //todo fix that
extern int highest_fd; //todo fix that

/**
 *
 */
void init_chat_list() {
	LIST_INIT(&chat_list_head);
	if (pthread_mutex_init(&lock_list, NULL) != 0) {
		perror("pthread_mutex_init");
		fprintf(stderr, "Mutex init fehler in der list. \n");
		exit(EXIT_FAILURE);
	}
}

/**
 *
 */
void add_new_controll_info(char username[USERNAME_REAL_SIZE], uint16_t hops,
		connection_item * item) {

	controll_info_list *controll_info_table = (controll_info_list *) malloc(
			sizeof(controll_info_list));

	snprintf(controll_info_table->controll_info.username, USERNAME_PSEUDO_SIZE,
			"%s", username);
	controll_info_table->controll_info.hops = hops;
	controll_info_table->connection_item = item;

	//Arbeiten mit geteilten resourcen.
	pthread_mutex_lock(&lock_list);

	FD_SET(item->socketFD, &readfds);
	highest_fd = item->socketFD;
	controll_info_usage_size++;
	LIST_INSERT_HEAD(&chat_list_head, controll_info_table, entries);

	pthread_mutex_unlock(&lock_list);
	notify_all_by_update();
}

/**
 * Ueberprueft ob ein benutzer schon in der Liste existiert.
 */
int checkUserAlreadyExist(char username[USERNAME_REAL_SIZE]) {
	controll_info_list *entry = NULL;
	int status = 0;

	pthread_mutex_lock(&lock_list);

	LIST_FOREACH(entry, &chat_list_head, entries)
	{
		if (strcmp(username, entry->controll_info.username) == 0) {
			status = -1;
			break;
		}
	}

	pthread_mutex_unlock(&lock_list);

	return status;
}

/**
 * Findet einen User bei seinem Username.
 */
controll_info_list* findUserByName(char *username) {
	controll_info_list *response = NULL;

	pthread_mutex_lock(&lock_list);

	controll_info_list *entry = NULL;
	if (!LIST_EMPTY(&chat_list_head)) {
		LIST_FOREACH(entry, &chat_list_head, entries)
		{
			if (strcmp(username, entry->controll_info.username) == 0) {
				response = entry;
				break;
			}
		}
	}

	pthread_mutex_unlock(&lock_list);

	return response;
}

/**
 * Ueberprueft ob ein Nutzer mit diesem Server verbunden ist.
 */
int checkUserConnectedWithMe(char *username) {
	controll_info_list *user = findUserByName(username);

	if (user != NULL) {
		return user->controll_info.hops;
	}

	return -1;
}

/**
 * Findet den naechst kleineren Socket in der Liste.
 * @TODO
 */
int find_next_smaller_socket_id() {
	int temp = 0;
	controll_info_list *entry = NULL;

	pthread_mutex_lock(&lock_list);
	if (!LIST_EMPTY(&chat_list_head)) {
		LIST_FOREACH(entry, &chat_list_head, entries)
		{
			if (entry->connection_item->socketFD > temp) {
				temp = entry->connection_item->socketFD;
			}
		}
	}

	pthread_mutex_unlock(&lock_list);

	return temp;
}

/**
 * Schreibt die Chat Liste in den Buffer.
 */
void writeChatListToBuffer(FILE * socketStream) {
	controll_info response_value;
	controll_info_list *controll_info_table = NULL;

	pthread_mutex_lock(&lock_list);

	LIST_FOREACH(controll_info_table, &chat_list_head, entries)
	{

		//Schreibe Information zum senden in die Structure.
		response_value.hops = controll_info_table->controll_info.hops;
		snprintf(response_value.username, USERNAME_PSEUDO_SIZE, "%s",
				controll_info_table->controll_info.username);

		//Schreibe Structure in den Sender Buffer.
		if (fwrite(&response_value, sizeof(controll_info), 1, socketStream)
				!= 1) {
			fprintf(stderr,
					"ERROR: Kann die Chat Liste nicht in den Buffer schreiben. \n");
			exit(EXIT_FAILURE);
		}
	}
	pthread_mutex_unlock(&lock_list);
}

/**
 * Todo noch einen Namens check hinzufügen oder andere metric um externe nutzer zu erkennen.
 * (Sockets sind nicht eindeutig da es fuer einen user mehrfach den selben geben kann.)
 */
controll_info_list* _find_user_by_socket(int socketFD) {
	controll_info_list* entry = NULL;
	controll_info_list* response_value = NULL;
	pthread_mutex_lock(&lock_list);
	if (!LIST_EMPTY(&chat_list_head)) {
		LIST_FOREACH(entry, &chat_list_head, entries)
		{
			if (socketFD == entry->connection_item->socketFD) {
				response_value = entry;
				break;
			}
		}
	}

	pthread_mutex_unlock(&lock_list);

	return response_value;
}

/**
 * Entfernt einen Nutzer anhand seines SocketFD.
 * Todo noch einen Namens check hinzufügen oder andere metric um externe nutzer zu erkennen.
 * (Sockets sind nicht eindeutig da es fuer einen user mehrfach den selben geben kann.)
 * (oder zumindest an den punkten validieren wo es noetig ist den richtigen user zu löschen.)
 */
int remove_user_by_socket(int socketFD) {
	int status = -1;
	controll_info_list *entry = _find_user_by_socket(socketFD);

	while (entry != NULL) {
		if (highest_fd == entry->connection_item->socketFD) {
			highest_fd = find_next_smaller_socket_id();
		}

		pthread_mutex_lock(&lock_list);

		FD_CLR(entry->connection_item->socketFD, &readfds);
		LIST_REMOVE(entry, entries);
		controll_info_usage_size--;
		//Todo der Speicher fuer das connection item muss noch irgendwo freigegeben werden
		// nur aktuell noch nicht ganz sicher wo.
		status = 0;

		pthread_mutex_unlock(&lock_list);
		entry = _find_user_by_socket(socketFD);
	}

	notify_all_by_update();

	return status;
}

/**
 * Merged einen User aus einer von einem Server erhaltenen Routing Table
 * in die eigene Table.
 */
int merge_user_list(connection_item* item, controll_info* user) {
	int status = -1;
	controll_info_list *entry = findUserByName(user->username);
	if (entry != NULL) {
		if (entry->controll_info.hops > (++user->hops)) {
			pthread_mutex_lock(&lock_list);
			entry->connection_item = item;
			entry->controll_info.hops = (++user->hops);
			pthread_mutex_unlock(&lock_list);
		}
	} else {
		add_new_controll_info(user->username, ++user->hops, item);
		status = 0;
	}
	return status;
}



/**
 * Funktion um das naechst eintreffende event zu Queuen.
 */
void checkEvent(fd_set* test) {
	controll_info_list *controll_info_table = NULL;

	pthread_mutex_lock(&lock_list);

	LIST_FOREACH(controll_info_table, &chat_list_head, entries)
	{
		if (FD_ISSET(controll_info_table->connection_item->socketFD, test)
				&& controll_info_table->connection_item->status == 0) {

			controll_info_table->connection_item->status = QUEUED;
			enqueue(controll_info_table->connection_item);

		}
	}
	pthread_mutex_unlock(&lock_list);
	/*
	 controll_info_list*  list = _find_user_by_socket(item->socketFD);
	 if(list == NULL){
	 FD_SET(item->socketFD, &readfds);
	 highest_fd = item->socketFD;
	 }*/

}
