#ifdef HAVE_CONFIG_H
#endif
#include "client.h"

#define COMMUNICATION_PORT 9012
//#define SERVER_IP "141.22.27.103"
#define SERVER_IP "127.0.0.1"

char username[USERNAME_REAL_SIZE];

//Initialisiert einen LogIN/LogOUT Header
void init_log_in_out(login_out* data, int length, int flags) {
	data->common_header.length = length;
	data->common_header.type = LOG_IN_OUT_HEADER;
	data->common_header.version = SUPPORTED_VERSION;
	data->common_header.flags = flags;
}

/**
 * Behandlung des Login Vorganges
 */
int log_in(int socketFD, char* username) {

	// Anmelden an den Server.
	login_out login_data;
	memset((void *) &login_data, 0, sizeof(login_out));
	init_log_in_out(&login_data, USERNAME_REAL_SIZE, SYN);

	snprintf(&login_data.login_out_header.username[0], USERNAME_REAL_SIZE, "%s",
			username);

	ssize_t bytes_send = send(socketFD, (void*) &login_data, sizeof(login_data),
			0);
	if (bytes_send < 0) {
		perror("Send ERROR");
		fprintf(stderr, "ERROR: Beim Senden.\n");
	}

	// Recieve Antwort
	//These calls return the number of bytes received, or -1 if an error occurred.
	//The return value will be 0 when the peer has performed an orderly shutdown.
	common_header header;
	ssize_t numBytesRcvd = recv(socketFD, (void*) &header, sizeof(header), 0);
	if (numBytesRcvd == 0) {
		perror("No Connection");
		fprintf(stderr, ": Der Server hat die Verbindung geschlossen.\n");
		exit(EXIT_FAILURE);
	}
	if (numBytesRcvd < 0) {
		perror("Recieve ERROR");
		fprintf(stderr, "ERROR: Beim Recieve.\n");
	}

	if (header.version == SUPPORTED_VERSION) {
		if (header.flags == (DUP | SYN | ACK)) {
			printf("Der Username ist bereits vergeben.\n");
			return -1;
		}
		if (header.flags == (SYN | ACK)) {
			return 0;
		}
	}
	return -2;

}

/**
 * Controll Informationen Anfordern.
 */
int infoLoad(int socketFD) {
	common_header header;
	memset((void *) &header, 0, sizeof(common_header));

	header.flags = GET;
	header.length = 0;
	header.type = CONTROL_INFO_HEADER;
	header.version = SUPPORTED_VERSION;

	ssize_t bytes_send = send(socketFD, (void*) &header, sizeof(header), 0);
	if (bytes_send < 0) {
		perror("Send ERROR");
		fprintf(stderr, "ERROR: Beim Senden.\n");
	}

	return 0;

}

/**
 * Ausloggen des Users.
 */
void userCloseConnection(int socket_fd) {
	login_out login_data;
	memset((void *) &login_data, 0, sizeof(login_out));

	init_log_in_out(&login_data, USERNAME_REAL_SIZE, NO_FLAGS);

	snprintf(&login_data.login_out_header.username[0], USERNAME_REAL_SIZE, "%s",
			username);

	ssize_t bytes_send = send(socket_fd, (void*) &login_data,
			sizeof(login_data), 0);

	if (bytes_send < 0) {
		perror("Send ERROR");
		fprintf(stderr, "ERROR: Beim Senden.\n");
	}
}

//void sendToAllMessage(int SocketFD, struct sockaddr_in sa, char* message,
//		int length) {
//}

/**
 * Nachricht zu einem Benutzer schicken.
 */
void sendMessageTo(int SocketFD,char* username_recv,
		int length_username, char* message, int length_message) {

	common_header header;
	memset((void *) &header, 0, sizeof(common_header));
	message_info data;
	memset((void *) &data, 0, sizeof(message_info));

	header.flags = NO_FLAGS;
	header.length = MAX_MESSAGE_SIZE;
	header.type = MESSAGE_HEADER;
	header.version = SUPPORTED_VERSION;

	// Usernames hinzufuegen.
	int user_name_size =
			length_username > USERNAME_PSEUDO_SIZE ?
			USERNAME_PSEUDO_SIZE :
														length_username;
	snprintf(data.destination_username, user_name_size, "%s", username_recv);

	user_name_size = USERNAME_PSEUDO_SIZE;
	snprintf(data.source_username, user_name_size, "%s", username);

	char payload[MAX_MESSAGE_SIZE];
	int max_message_size = length_message > MAX_MESSAGE_SIZE ?
	MAX_MESSAGE_SIZE :
																length_message;
	snprintf(payload, max_message_size, "%s", message);

	FILE *outstream = fdopen(SocketFD, "w");

	if (fwrite(&header, sizeof(common_header), 1, outstream) != 1) {
		perror("fwrite");
		fprintf(stderr, "ERROR: Kann nicht zum Message stream schreiben. \n");
	}

	if (fwrite(&data, sizeof(message_info), 1, outstream) != 1) {
		perror("fwrite");
		fprintf(stderr, "ERROR: Kann nicht zum Message stream schreiben. \n");
	}

	if (fwrite(&payload, sizeof(char) * max_message_size, 1, outstream) != 1) {
		perror("fwrite");
		fprintf(stderr, "ERROR: Kann nicht zum Message stream schreiben. \n");
	}

	fflush(outstream);

}

/**
 * Behandeln der einkommenden Nachrichten vom Server.
 */
void* messageHandlerMain(void * socket_fd_p) {

	char loutoutflag = KEIN_LOGOUT;
	int socket_fd = *((int*) socket_fd_p);
	common_header header;
	while (loutoutflag) {
		memset((void *) &header, 0, sizeof(common_header));

		//Recieven des Headers
		ssize_t numBytesRcvd = recv(socket_fd, (void*) &header,
				sizeof(common_header), 0);
		if (numBytesRcvd == 0) {
			perror("No Connection");
			fprintf(stderr, ": Der Server hat die Verbindung geschlossen.\n");
			loutoutflag = LOGOUT;
		}

		if (header.version == SUPPORTED_VERSION && loutoutflag == KEIN_LOGOUT) {

			// Ausgabe der Message
			if (header.type == MESSAGE_HEADER) {
				message_info data;
				memset((void *) &data, 0, sizeof(message_info));
				char payload[255];
				memset((void *) payload, 0, sizeof(payload));

				//Den Inhalt der Nachricht empfnagen
				numBytesRcvd = recv(socket_fd, (void*) &data,
						sizeof(message_info), 0);
				numBytesRcvd = recv(socket_fd, (void*) &payload,
						sizeof(char) * header.length, 0);
				if (numBytesRcvd == 0) {
					perror("No Connection");
					fprintf(stderr,
							": Der Server hat die Verbindung geschlossen.\n");
					loutoutflag = LOGOUT;
				} else {
					printf("\n %s: %s \n", data.source_username, payload);
				}
			}

			// Ausgabe der Controll Information
			if (header.type == CONTROL_INFO_HEADER) {
				controll_info data;
				int i = 0;
				for (i = 0; i < header.length; i++) {
					memset((void *) &data, 0, sizeof(controll_info));
					ssize_t numBytesRcvd = recv(socket_fd, (void*) &data,
							sizeof(controll_info), 0);
					if (numBytesRcvd == 0) {
						perror("No Connection");
						fprintf(stderr,
								": Der Server hat die Verbindung geschlossen.\n");
						loutoutflag = LOGOUT;
					} else {
						printf("%d: Username: %s \n", i, data.username);
					}
				}
			}

			if (header.type == LOG_IN_OUT_HEADER) {
				if (header.flags == (FIN | ACK)) {
					printf("Logout war erfolgreich \n");
					loutoutflag = LOGOUT;
				} else {
					printf("Logout Header mit falschen Flags erhalten. \n");
				}
			}

			if (!(header.type == LOG_IN_OUT_HEADER
					|| header.type == CONTROL_INFO_HEADER
					|| header.type == MESSAGE_HEADER)) {
				printf("Der Nachrichtentyp %d wird nicht unterstuezt. \n",
						header.type);
			}

		} else {
			printf("Der Header enthaelt die unbekannte Versionnummer %d \n",
					header.version);
		}
	}
	return NULL;
}

/**
 * Main funktion.
 */
int main(int argc, char *argv[]) {
	struct sockaddr_in sa;
	int res;
	int SocketFD;

	SocketFD = socket(AF_INET, SOCK_STREAM, 0); // Erstellen des Sockets.
	if (SocketFD == -1) {
		perror("socket");
		fprintf(stderr, "ERROR: Es kann kein Socket erstellt werden.\n");
		exit(EXIT_FAILURE);
	}

	memset(&sa, 0, sizeof(sa)); // Struktur Initialisieren.

	sa.sin_family = AF_INET;
	sa.sin_port = htons(COMMUNICATION_PORT);

	// Destination = Speicher ergebnis.
	res = inet_pton(AF_INET, SERVER_IP, &sa.sin_addr);
	if (res < 0) {
		perror("wrong ip");
		fprintf(stderr,
				"ERROR: Es kann keine Konvertierung durchgeführt werden.\n");
		exit(EXIT_FAILURE);
	}

	//Connection zum Server aufbauen
	if (connect(SocketFD, (struct sockaddr *) &sa, sizeof sa) == -1) {
		perror("connect failed");
		close(SocketFD);
	}

	/**
	 ** LOG in Phase
	 **
	 */
	int status = 0;
	int i = 0;
	do {
		printf("Bitte geben Sie einen gueltigen Benutzernamen ein: \n");
		fgets(username, USERNAME_REAL_SIZE, stdin);
		username[strcspn(username, "\n")] = 0;
		printf("Einen Momment bitte:%s:\n", username);
		status = log_in(SocketFD, username);
		if (status < 0) {
			printf("Der Login Vorgang ist fehlgeschlagen.\n");
		}
		i++;
		if (i > 10) {
			printf(
					"Der von ihnen ausgewählte Server ist mommentan nicht erreichbar. \n");
			exit(1);
		}
	} while (status < 0);

	/**
	 **Thread starten um Nachrichten vom Server zu empfangen
	 */
	if (status == 0) {
		pthread_t messageHandlerThread;
		int* socket_fd_p = &SocketFD;
		int messageHandler = pthread_create(&messageHandlerThread, NULL,
				messageHandlerMain, (void*) socket_fd_p);
		if (messageHandler) {
			printf("ERROR; return code from pthread_create() is %d\n",
					messageHandler);
			close(SocketFD);
			exit(-1);
		}

		char label[] =
				"    .o oOOOOOOOo                                            OOOo\r\n    Ob.OOOOOOOo  OOOo.      oOOo.                      .adOOOOOOO\r\n    OboO\"\"\"\"\"\"\"\"\"\"\"\".OOo. .oOOOOOo.    OOOo.oOOOOOo..\"\"\"\"\"\"\"\"\"'OO\r\n    OOP.oOOOOOOOOOOO \"POOOOOOOOOOOo.   `\"OOOOOOOOOP,OOOOOOOOOOOB'\r\n    `O'OOOO'     `OOOOo\"OOOOOOOOOOO` .adOOOOOOOOO\"oOOO'    `OOOOo\r\n    .OOOO'            `OOOOOOOOOOOOOOOOOOOOOOOOOO'            `OO\r\n    OOOOO                 '\"OOOOOOOOOOOOOOOO\"`                oOO\r\n   oOOOOOba.                .adOOOOOOOOOOba               .adOOOOo.\r\n  oOOOOOOOOOOOOOba.    .adOOOOOOOOOO@^OOOOOOOba.     .adOOOOOOOOOOOO\r\n OOOOOOOOOOOOOOOOO.OOOOOOOOOOOOOO\"`  '\"OOOOOOOOOOOOO.OOOOOOOOOOOOOO\r\n \"OOOO\"       \"YOoOOOOMOIONODOO\"`  .   '\"OOROAOPOEOOOoOY\"     \"OOO\"\r\n    Y           'OOOOOOOOOOOOOO: .oOOo. :OOOOOOOOOOO?'         :`\r\n    :            .oO%OOOOOOOOOOo.OOOOOO.oOOOOOOOOOOOO?         .\r\n    .            oOOP\"%OOOOOOOOoOOOOOOO?oOOOOO?OOOO\"OOo\r\n                 '%o  OOOO\"%OOOO%\"%OOOOO\"OOOOOO\"OOO':\r\n                      `$\"  `OOOO' `O\"Y ' `OOOO'  o             .\r\n    .                  .     OP\"          : o     .\r\n";
		printf("%s", label);

		char command[KOMMANDO_SIZE];
		char logoutflag = KEIN_LOGOUT;

		//auf Konsoleneingaben warten ....
		while (logoutflag) {
			printf("Geben sie einen Befehl ein: \n");
			memset((void *) &command, 0, 255 * sizeof(char));
			fgets(command, KOMMANDO_SIZE, stdin);
			command[strcspn(command, "\n")] = 0;
			if (strcasecmp(command, "/info") == 0) {
				infoLoad(SocketFD);
			} else if (strcasecmp(command, "/exit") == 0) {
				userCloseConnection(SocketFD);
				logoutflag = LOGOUT;
				return 0;
			} else if (strcasecmp(command, "/msg") == 0) {
				char message[MAX_MESSAGE_SIZE];
				char username_dest[USERNAME_REAL_SIZE];
				printf("An wenn wollen Sie eine Nachricht schicken ? \n");
				fgets(username_dest, USERNAME_REAL_SIZE, stdin);
				printf("\n und nun bitte noch die Nachricht: \n");
				fgets(message, MAX_MESSAGE_SIZE, stdin);
				username_dest[strcspn(username_dest, "\n")] = 0;
				sendMessageTo(SocketFD, username_dest,
				USERNAME_REAL_SIZE, message, MAX_MESSAGE_SIZE);
			} else {
				printf("Unbekannter Befehl \n");
			}

		}
		pthread_join(messageHandlerThread, NULL);
	}

	close(SocketFD);

	return 0;
}
