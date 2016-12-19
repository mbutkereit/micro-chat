#include "server.h"
#include <errno.h>

extern int controll_info_usage_size;

fd_set readfds; // Select file descriptor set.
struct timeval tv; // Timer Struktur fuer select().
int highest_fd = 0; // höchster file descriptor

/**
 * Initialisiert die log_in_out structure.
 */
void init_log_in_out(login_out* data, int length, int flags) {
	data->common_header.length = length;
	data->common_header.type = LOG_IN_OUT_HEADER;
	data->common_header.version = SUPPORTED_VERSION;
	data->common_header.flags = flags;
}

/**
 * Initialisierungs Funktion fuer den common Header.
 */
void init_common_header(common_header* data, uint8_t length, uint8_t flags,
		uint8_t type) {
	data->length = length;
	data->type = type;
	data->version = SUPPORTED_VERSION;
	data->flags = flags;
}

/**
 * Bearbeitet den Versand von Nachrichten.
 */
int handleLogin(connection_item* item, common_header* header) {
	int status = 0;
	if (header->flags & FIN) {
		login_out_header data;
		// Recieve Username.
		ssize_t numBytesRcvd = recv(item->socketFD, (void*) &data,
				header->length * sizeof(char), 0);
		if (numBytesRcvd < 0) {
			perror("recv");
			fprintf(stderr,
					"ERROR: Kann den Username von dem LOG_IN_OUT_HEADER nicht recv\n");
			return -1;
		}

		controll_info_list* user = findUserByName(data.username);
		if (user != NULL) {
			//Hier ist es kein Problem da die Nutzer an diesem Server eigene Sockets bekommen.
			remove_user_by_socket(item->socketFD);
			common_header header;

			header.flags = FIN | ACK;
			header.length = 0;
			header.type = LOG_IN_OUT_HEADER;
			header.version = SUPPORTED_VERSION;

			ssize_t send_info = send(item->socketFD, &header,
					sizeof(common_header), 0);

			if (send_info < 0) {
				perror("send");
				fprintf(stderr, "ERROR: Bei Send Logout bestaetigung. \n");
				exit(EXIT_FAILURE);
			}

			status = 0;
		}
	} else if (header->flags & SYN) {

		int i = 0;
		int socketFD = item->socketFD;
		do {

			//Im Fehlerfall
			if (i != 0) {
				common_header cheader;
				//recieve header
				ssize_t numBytesRcvd = recv(socketFD, (void*) &cheader,
						sizeof(common_header), 0);
				if (numBytesRcvd < 0) {
					perror("recv");
					fprintf(stderr,
							"ERROR: Kann den Username von dem LOG_IN_OUT_HEADER nicht recv\n");
					status = LOG_IN_NOT_POSSIBLE;
				}
			}

			login_out response_value;
			memset((void *) &response_value, 0, sizeof(login_out));

			login_out_header* data = (login_out_header*) malloc(
					sizeof(login_out_header));
			memset((void *) data, 0, sizeof(login_out_header));

			// Recieve Username.
			ssize_t numBytesRcvd = recv(socketFD, (void*) data,
					header->length * sizeof(char), 0);
			if (numBytesRcvd < 0) {
				perror("recv");
				fprintf(stderr,
						"ERROR: Kann den Username von dem LOG_IN_OUT_HEADER nicht recv\n");
				status = LOG_IN_NOT_POSSIBLE;
			}

			// Checken ob es den User schon gibt.
			fprintf(stderr, "Der erhaltene Username ist:%s\n", data->username);
			int user_already_exist = checkUserAlreadyExist(data->username);

			if (user_already_exist < 0) {
				init_log_in_out(&response_value, 0, SYN | ACK | DUP);
				status = LOG_IN_NOT_POSSIBLE;
			} else {
				init_log_in_out(&response_value, 0, SYN | ACK);
				add_new_controll_info(data->username, DEFAULT_HOPS, item);
				status = LOG_IN_SUCCESSFUL;
			}

			// Antwort senden
			ssize_t send_info = send(socketFD, &response_value,
					sizeof(common_header), 0);

			if (send_info < 0) {
				perror("send");
				fprintf(stderr,
						"ERROR: Das senden des LogIn Response war nicht erfolgreich \n");
			}

			i++;
		} while (status != LOG_IN_SUCCESSFUL || i == MAX_LOG_IN_TRYS);

		// clean up
		if (status != LOG_IN_SUCCESSFUL) {
			item->status = REMOVE;
		}
	}

	return status;
}

/**
 * Sendet Controll Information an client.
 */
void sendControllInfo(connection_item* item, uint8_t flags) {
	common_header header;

	init_common_header(&header, controll_info_usage_size, flags,
			CONTROL_INFO_HEADER);

	// Kann nach test entfernt werden.
	header.flags = flags;
	header.version = SUPPORTED_VERSION;
	header.type = CONTROL_INFO_HEADER;
	header.length = controll_info_usage_size;

	FILE *outstream = fdopen(item->socketFD, "w");

	if (fwrite(&header, sizeof(common_header), 1, outstream) != 1) {
		perror("fwrite");
		fprintf(stderr, "ERROR: Kann nicht zum Message stream schreiben. \n");
		exit(EXIT_FAILURE);
	}

	writeChatListToBuffer(outstream);

	// Todo anschauen fehlerbehandlung
	fflush(outstream);
}

/**
 * Versand von Controll Informationen (Routing Table).
 *
 * Zwei moegliche:
 *   - Zustande GET flag einfach die controllInfos senden.
 *   - Zustand NO_Flags es wurden information von einem Server an uns gesendet.
 */
void handleControllInfo(connection_item* item, common_header* old_header) {

	if (old_header->flags == GET) {
		sendControllInfo(item, NO_FLAGS);
		// Fuegt den Socket dem Select hinzu falls kein User existiert.
		controll_info_list*  list = _find_user_by_socket(item->socketFD);
		if(list == NULL){
			add_to_server_list(item);
		}

	}

	if (old_header->flags == NO_FLAGS) {
		add_to_server_list(item);
		controll_info data;
		int i = 0;
		for (i = 0; i < old_header->length; i++) {
			// Zuruecksetzten der Information nach jeder behandlung.
			memset((void *) &data, 0, sizeof(controll_info));

			ssize_t numBytesRcvd = recv(item->socketFD, (void*) &data,
					sizeof(controll_info), 0);
			if (numBytesRcvd < 0) {
				perror("fwrite");
				fprintf(stderr,
						"ERROR: Kann nicht zum Message stream schreiben. \n");
			}

			merge_user_list(item, &data);
		}
	}
}

/**
 * Bearbeitet den Versand von Nachrichten.
 */
void handleMessage(int socketFD, common_header* header) {

	message_info data;

	memset((void *) &data, 0, sizeof(message_info));

	char payLoadBuffer[header->length];

	// Recieve Message Info (Usernames usw ).
	ssize_t numBytesRcvd = recv(socketFD, (void*) &data, sizeof(message_info),
			0);

	if (numBytesRcvd < 0) {
		perror("recv");
		fprintf(stderr,
				"ERROR: Es konnte die Usernames des Message nicht empfangen werden \n");
	}

	// Recieve die Message selbst.
	numBytesRcvd = recv(socketFD, (void*) &payLoadBuffer,
			sizeof(char) * header->length, 0);
	if (numBytesRcvd < 0) {
		perror("recv");
		fprintf(stderr, "ERROR: Kein Recv der Message moeglich\n");
	}

	// Finde den User oder verwerfe die nachricht.
	controll_info_list* next_server = findUserByName(data.destination_username);

	if (next_server != NULL) {

		FILE *outstream = fdopen(next_server->connection_item->socketFD, "w");

		if (fwrite(header, sizeof(common_header), 1, outstream) != 1) {
			perror("fwrite");
			fprintf(stderr,
					"ERROR: Common Header kann nicht in den Buffer geschrieben werden. \n");
			exit(EXIT_FAILURE);
		}

		if (fwrite(&data, sizeof(message_info), 1, outstream) != 1) {
			perror("fwrite");
			fprintf(stderr,
					"ERROR: Usernames konnen nicht in den Buffer geschrieben werden. \n");
			exit(EXIT_FAILURE);
		}

		if (fwrite(&payLoadBuffer, sizeof(char) * header->length, 1, outstream)
				!= 1) {
			perror("fwrite");
			fprintf(stderr,
					"ERROR: Die Message kann nicht in den Buffer geschrieben werden. \n");
			exit(EXIT_FAILURE);
		}
		//todo schauen ob error codes kommen
		fflush(outstream);
	}

}

/**
 * Empfangen eines Headers.
 */
int RecieveHeaderTCP(common_header* header, int clntSocket) {
	ssize_t numBytesRcvd = 0;

	numBytesRcvd = recv(clntSocket, (void*) header, sizeof(common_header), 0);

	if (numBytesRcvd < 0) {
		perror("recv");
		fprintf(stderr,
				"ERROR: Die Message kann nicht in den Buffer geschrieben werden. \n");
	}

	if (numBytesRcvd == 0) {
		return 0;
	}

	return 1;
}

/**
 * Thread um die neu eintrefenden Events zu dispatchen.
 */
void* eventDispatcherThreadMain() {
	tv.tv_sec = SELECT_WARTEZEIT;
	fd_set read_fd_set;

	for (;;) {
		read_fd_set = readfds; // readfds wuede sonst resetet werden.
		int rv = select((highest_fd + 1), &read_fd_set, NULL, NULL, &tv); // Highest fd wird erhoeht um 1 laut man.
		tv.tv_sec = SELECT_WARTEZEIT;
		if (rv > 0) {
			checkEvent(&read_fd_set);
		}
	}
}

/**
 * Die Workerthreads empfangen die Nachrichten von Sockets aus der Queue
 */
void* workerThreadMain() {
	for (;;) {
		connection_item* item = dequeue();
		if (item != NULL) {
			common_header header;
			memset((void *) &header, 0, sizeof(common_header));

			int header_recieved = RecieveHeaderTCP(&header, item->socketFD);
			if (header_recieved == 0) {
				perror("recv");
				fprintf(stderr,
						"Die Verbindung von einem Clienten oder Server wurde geschlossen.\n");
				//@TODO Verbindung schließen remove_user_by_socket(item->socketFD); //
			}

			if (header_recieved < 0) {
				fprintf(stderr, "ERROR: Out of memory\n");
				exit(EXIT_FAILURE);
			}

			if (header.version == SUPPORTED_VERSION) {
				switch (header.type) {
				case LOG_IN_OUT_HEADER:
					fprintf(stderr, "Log Header erhalten\n");
					handleLogin(item, &header);
					break;
				case CONTROL_INFO_HEADER:
					fprintf(stderr, "Control Info Header erhalten\n");
					handleControllInfo(item, &header);
					break;
				case MESSAGE_HEADER:
					fprintf(stderr, "Message Header erhalten\n");
					handleMessage(item->socketFD, &header);
					break;
				default:
					fprintf(stderr, "Unbekannter MessageTyp: %d\n",
							(int) header.type);
				}
			} else {
				fprintf(stderr, "Unbekannte Headerversion: %d\n",
						(int) header.version);
				// Ueberpruefe MAXIMUM_TIME_TO_LIVE und entferne anderenfalls den eintrag.
				item->ttl++;
				if (item->ttl > MAXIMUM_TIME_TO_LIVE) {
					//todo umbennen es werden alle user gelöscht.
					remove_user_by_socket(item->socketFD); //
				}
			}

			if (item->status == QUEUED) {
				item->status = PENDING;
			} else if (item->status == REMOVE) {
				close(item->socketFD);
				free(item);
			}

		}
	}
}

/**
 * Nimmt alle neuen Verbindungen entgegen und schreibt sie auf die Queue.
 */
void connectionHandling() {
	struct sockaddr_in sa;

	int socketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketFD < 0) {
		perror("socket");
		fprintf(stderr, "ERROR: Es kann kein Socket erstellt werden.\n");
		exit(EXIT_FAILURE);
	}

	memset((void *) &sa, 0, sizeof(sa));

	sa.sin_port = htons(DEFAULT_PORT);

	int yes = 1;
	//char yes='1'; // Solaris people use this

	// lose the pesky "Address already in use" error message
	if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("setsockopt");
		exit(1);
	}

	int bind_code = bind(socketFD, (struct sockaddr *) &sa, sizeof(sa));
	if (bind_code < 0) {
		perror("bind");
		fprintf(stderr, "ERROR: Bind funktioniert nicht.\n");
		exit(EXIT_FAILURE);
	}

	int listen_code = listen(socketFD, ALLOWED_PENDING_CONNECTIONS);

	if (listen_code < 0) {
		fprintf(stderr, "ERROR: Es konnte kein Listing aktiviert werden \n");
		exit(EXIT_FAILURE);
	}

	for (;;) {
		connection_item* request = (connection_item*) malloc(
				sizeof(connection_item));

		if (request == 0) {
			perror("malloc");
			fprintf(stderr, "ERROR: Out of memory\n");
			exit(EXIT_FAILURE);
		}

		memset((void *) request, 0, sizeof(connection_item));

		unsigned int structure_size = sizeof(request->client_addr);
		int connectionFD = accept(socketFD,
				(struct sockaddr *) &request->client_addr, &structure_size);

		if (connectionFD < 0) {
			free(request);
			fprintf(stderr,
					"ERROR: Es konnte keine Verbindung aufgebaut werden.\n");
			continue;
		}

		request->socketFD = connectionFD;
		request->ttl = 0;

		enqueue(request);
	}

}

/**
 * Funktion um den Initialen Server austausch anzustossen.
 * baut Verbindung zum Server auf und fragt nach der Tabelle
 */
void initializeRequest(char* ip) {
	struct sockaddr_in sa;
	int res;
	int socketFD;

	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Erstellen des Sockets.
	if (socketFD == -1) {
		fprintf(stderr, "ERROR: Socket erstellung\n");
		exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof(struct sockaddr_in)); // Struktur Initialisieren.

	sa.sin_family = AF_INET;
	sa.sin_port = htons(DEFAULT_PORT);

	// Destination = Speicher ergebnis.
	res = inet_pton(AF_INET, ip, &sa.sin_addr);
	if (res < 0) {
		fprintf(stderr, "ERROR: Konvertierung \n");
		exit(EXIT_FAILURE);
	}

	if (connect(socketFD, (struct sockaddr *) &sa, sizeof sa) == -1) {
		perror("connect");
		close(socketFD);
	}

	common_header header;
	controll_info data;

	header.flags = GET;
	header.length = 0;
	header.type = CONTROL_INFO_HEADER;
	header.version = SUPPORTED_VERSION;

	ssize_t bytes_send = send(socketFD, (void*) &header, sizeof(common_header),
			0);
	if (bytes_send < 0) {
		fprintf(stderr, "ERROR; Send \n");

	}

	// memset((void *)&header,0,sizeof(common_header));
	ssize_t numBytesRcvd = recv(socketFD, (void*) &header,
			sizeof(common_header), 0);
	if (numBytesRcvd < 0) {
		fprintf(stderr, "ERROR; Recieve\n");

	}

	connection_item* request = (connection_item*) malloc(
			sizeof(connection_item));
	if (request == 0) {
		perror("malloc");
		fprintf(stderr, "ERROR: Out of memory\n");
		exit(EXIT_FAILURE);
	}

	memset((void *) request, 0, sizeof(connection_item));

	request->socketFD = socketFD;
	request->ttl = 0;
	int i = 0;
	if (header.version == SUPPORTED_VERSION) {
		for (i = 0; i < header.length; i++) {
			memset((void *) &data, 0, sizeof(controll_info));

			ssize_t numBytesRcvd = recv(socketFD, (void*) &data,
					sizeof(controll_info), 0);
			if (numBytesRcvd < 0) {
				fprintf(stderr, "ERROR; Recieve is \n");
			}
			merge_user_list(request, &data);
		}

	}
}

/**
 * Einstiegspunkt des Programmes.
 */
int main(int argc, const char * argv[]) {

	/**
	 * Initiaisierung der Komponeneten.
	 */
	init_queue();
	init_chat_list();
	init_server_list();
	FD_ZERO(&readfds);

	initializeRequest("141.22.27.107");

	/**
	 * Starten der Threads.
	 */
	pthread_t eventDispatcherThread;
	int eventDispatcher = pthread_create(&eventDispatcherThread, NULL,
			eventDispatcherThreadMain, NULL);
	if (eventDispatcher) {
		fprintf(stderr, "ERROR; return code from pthread_create() is %d\n",
				eventDispatcher);
		return EXIT_FAILURE;
	}

	int i = 0;
	pthread_t workerThread[WORKER_THREADS];
	for (i = 0; i < WORKER_THREADS; i++) {
		int worker = pthread_create(&workerThread[i], NULL, workerThreadMain,
		NULL);
		if (worker) {
			fprintf(stderr, "ERROR; return code from pthread_create() is %d\n",
					worker);
			return EXIT_FAILURE;
		}
	}

	// Main Thread Funktion.
	connectionHandling();

	return 0;
}

