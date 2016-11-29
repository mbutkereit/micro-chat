#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Client.h"

#define COMMUNICATION_PORT 9012
#define SERVER_IP "127.0.0.1"

#define MAX_MESSAGE_SIZE 255

char username[USERNAME_REAL_SIZE];

//API functionen
void init_log_in_out(login_out* data,int length,int flags){
    data->common_header.length = length;
    data->common_header.type = LOG_IN_OUT_HEADER;
    data->common_header.version= SUPPORTED_VERSION;
    data->common_header.flags = flags;
}

/**
 * Log_in vorgang behandlung.
 */
int log_in(int socketFD,char* username){
    
    // Anmelden an den Server.
    login_out login_data;
    memset((void *)&login_data,0,sizeof(login_out));
    init_log_in_out(&login_data,USERNAME_REAL_SIZE,SYN);
    
    snprintf(&login_data.login_out_header.username[0], USERNAME_REAL_SIZE,"%s" ,username);
    
    ssize_t bytes_send = send(socketFD,(void*)&login_data, sizeof(login_data), 0);
    if(bytes_send < 0){
        
    }
    
    // Recieve Antwort
    common_header header;
    ssize_t numBytesRcvd = recv(socketFD, (void*) &header, sizeof(header), 0);
    
    if(numBytesRcvd < 0){
    }
    
    if(header.version == SUPPORTED_VERSION){
        if (header.flags == (DUP|SYN|ACK)){
            return -1;
        }
        if (header.flags == (SYN|ACK)) {
            return 0;
        }
    }
  return -2;
    
}

/**
 * Controll Informationen Anfordern.
 */
int infoLoad(int socketFD){
    common_header header;
    
    header.flags = GET;
    header.length = 0;
    header.type = CONTROL_INFO_HEADER;
    header.version = SUPPORTED_VERSION;
 
    
    ssize_t bytes_send = send(socketFD,(void*)&header, sizeof(header), 0);
    if(bytes_send < 0 ){
    // behandeln
    }
    
    return 0;
    
}

/**
 * Ausloggen des Users.
 */
void userCloseConnection(int socket_fd, struct sockaddr_in sa){
    login_out login_data;
    memset((void *)&login_data,0,sizeof(login_out));
    
    init_log_in_out(&login_data,USERNAME_REAL_SIZE,NO_FLAGS);
    
    snprintf(&login_data.login_out_header.username[0], USERNAME_REAL_SIZE,"%s" ,username);
    
   ssize_t bytes_send = send(socket_fd,(void*)&login_data, sizeof(login_data), 0);
    
   if(bytes_send < 0 ){
        // behandeln
    }
}

void  sendToAllMessage(int SocketFD, struct sockaddr_in sa, char* message ,int length) {}


/**
 * Nachricht zu einem Benutzer schicken.
 */
void sendMessageTo(int SocketFD, struct sockaddr_in sa, char* username_recv, int length_username ,char* message, int length_message){
    
    common_header header;
    memset((void *)&header,0,sizeof(common_header));
    message_info data;
    memset((void *)&data,0,sizeof(message_info));
    
    header.flags = NO_FLAGS;
    header.length = 255;
    header.type = MESSAGE_HEADER;
    header.version = SUPPORTED_VERSION;
    
    // Usernames hinzufuegen.
    int user_name_size = length_username > USERNAME_PSEUDO_SIZE ? USERNAME_PSEUDO_SIZE:length_username;
    snprintf(data.destination_username, user_name_size,"%s" ,username_recv);
    
    user_name_size = USERNAME_PSEUDO_SIZE;
    snprintf(data.source_username, user_name_size ,"%s" ,username);
    
    char payload[255];
    int max_message_size = length_message > MAX_MESSAGE_SIZE ? MAX_MESSAGE_SIZE :length_message;
    snprintf(payload, max_message_size,"%s" ,message);

    FILE *outstream = fdopen(SocketFD, "w");
    
    if (fwrite(&header, sizeof(common_header), 1, outstream) != 1) {
        perror("fwrite");
        fprintf(stderr,"ERROR: Kann nicht zum Message stream schreiben. \n");
    }
    
    if (fwrite(&data, sizeof(message_info), 1, outstream) != 1) {
        perror("fwrite");
        fprintf(stderr,"ERROR: Kann nicht zum Message stream schreiben. \n");
    }

    
    if (fwrite(&payload, sizeof(char)*max_message_size, 1, outstream) != 1) {
        perror("fwrite");
        fprintf(stderr,"ERROR: Kann nicht zum Message stream schreiben. \n");
    }
    
    // Todo anschauen fehlerbehandlung
    fflush(outstream);

}

/**
 * Behandeln der einkommenden Nachrichten.
 */
void* messageHandlerMain(void * socket_fd_p){
    for (;;) {
        int socket_fd = *((int*) socket_fd_p);
            common_header header;
            memset((void *)&header,0,sizeof(common_header));

            ssize_t numBytesRcvd = recv(socket_fd, (void*) &header, sizeof(common_header), 0);
        
        if(numBytesRcvd == 0){
         // Programm sicher beeden
        }
            if(header.version == SUPPORTED_VERSION){
                
                // Ausgabe der Message
                if (header.type == MESSAGE_HEADER){
                    message_info data;
                    memset((void *)&data,0,sizeof(message_info));
                    char payload[255];
                    
                    numBytesRcvd = recv(socket_fd, (void*) &data, sizeof(message_info), 0);
                    numBytesRcvd = recv(socket_fd, (void*) &payload, sizeof(char)*header.length, 0);
                    printf("\n %s: %s \n",data.source_username,payload);
            
                }
                
                // Ausgabe der Controll Information
                if (header.type == CONTROL_INFO_HEADER){
                            controll_info data;
                            for(int i = 0 ; i < header.length ; i++){
                                memset((void *)&data,0,sizeof(controll_info));
                                ssize_t numBytesRcvd = recv(socket_fd, (void*) &data, sizeof(controll_info), 0);
                                if(numBytesRcvd < 0){
                                // abfangen.
                                }
                                printf("%d: Username: %s \n",i,data.username);
                    }
                }
                
                if (header.type == LOG_IN_OUT_HEADER){
                    if(header.flags == (FIN | ACK)){
                        printf("Logout war erfolgreich \n");
                    }
                    
                }
            }
        }
}

/**
 * Main funktion.
 */
int main(int argc, char *argv[])
{
    struct sockaddr_in sa;
    int res;
    int SocketFD;
    
    SocketFD = socket(AF_INET, SOCK_STREAM, 0); // Erstellen des Sockets.
    if (SocketFD == -1) {
    //error
    }
    
    memset(&sa, 0, sizeof(sa)); // Struktur Initialisieren.
    
    sa.sin_family = AF_INET;
    sa.sin_port = htons(COMMUNICATION_PORT);
    
    // Destination = Speicher ergebnis.
    res = inet_pton(AF_INET, SERVER_IP, &sa.sin_addr);
    if (connect(SocketFD, (struct sockaddr *)&sa, sizeof sa) == -1) {
        perror("connect failed");
        close(SocketFD);
    }
    
    /**
     ** LOG in Phase
     **
     */
    char string [USERNAME_REAL_SIZE];
    int status = 0;
    int i = 0;
    do{
        printf ("Bitte geben Sie einen gueltigen Benutzernamen ein: \n");
        //todo replace with right method
        fgets (string,USERNAME_REAL_SIZE,stdin);
        string[strcspn(string, "\n")] = 0;
        printf ("Einen Momment bitte: %s\n",string);
        status = log_in(SocketFD , string);
        i++;
        if(i > 10 ){
            printf("Der von ihnen ausgewählte Server ist mommentan nicht erreichbar. \n");
            exit(1);
        }
    }while(status<0 );
    
    /**
     **
     ** Handle Input.
     **
     */
    
    if(status == 0 ){
        snprintf(username, USERNAME_REAL_SIZE,"%s" ,string);
        pthread_t messageHandlerThread;
        int* socket_fd_p = &SocketFD;
        int messageHandler = pthread_create(&messageHandlerThread, NULL, messageHandlerMain, (void*)socket_fd_p);
        if (messageHandler){
            printf("ERROR; return code from pthread_create() is %d\n", messageHandler);
            close(SocketFD);
            exit(-1);
        }
        
        
        
            char label[] = "    .o oOOOOOOOo                                            OOOo\r\n    Ob.OOOOOOOo  OOOo.      oOOo.                      .adOOOOOOO\r\n    OboO\"\"\"\"\"\"\"\"\"\"\"\".OOo. .oOOOOOo.    OOOo.oOOOOOo..\"\"\"\"\"\"\"\"\"'OO\r\n    OOP.oOOOOOOOOOOO \"POOOOOOOOOOOo.   `\"OOOOOOOOOP,OOOOOOOOOOOB'\r\n    `O'OOOO'     `OOOOo\"OOOOOOOOOOO` .adOOOOOOOOO\"oOOO'    `OOOOo\r\n    .OOOO'            `OOOOOOOOOOOOOOOOOOOOOOOOOO'            `OO\r\n    OOOOO                 '\"OOOOOOOOOOOOOOOO\"`                oOO\r\n   oOOOOOba.                .adOOOOOOOOOOba               .adOOOOo.\r\n  oOOOOOOOOOOOOOba.    .adOOOOOOOOOO@^OOOOOOOba.     .adOOOOOOOOOOOO\r\n OOOOOOOOOOOOOOOOO.OOOOOOOOOOOOOO\"`  '\"OOOOOOOOOOOOO.OOOOOOOOOOOOOO\r\n \"OOOO\"       \"YOoOOOOMOIONODOO\"`  .   '\"OOROAOPOEOOOoOY\"     \"OOO\"\r\n    Y           'OOOOOOOOOOOOOO: .oOOo. :OOOOOOOOOOO?'         :`\r\n    :            .oO%OOOOOOOOOOo.OOOOOO.oOOOOOOOOOOOO?         .\r\n    .            oOOP\"%OOOOOOOOoOOOOOOO?oOOOOO?OOOO\"OOo\r\n                 '%o  OOOO\"%OOOO%\"%OOOOO\"OOOOOO\"OOO':\r\n                      `$\"  `OOOO' `O\"Y ' `OOOO'  o             .\r\n    .                  .     OP\"          : o     .\r\n";
            printf("%s", label);
        
            char command[255];

            for(;;){
                fgets(command, 255, stdin);
                command[strcspn(command, "\n")] = 0;
                if (strcmp(command, "/info") == 0) 
                {
                    infoLoad(SocketFD);
                }
                else if (strcmp(command, "/exit") == 0)
                {
                    userCloseConnection(SocketFD, sa);
                    return 0;
                }
                else if (strcmp(command,"/msg") == 0)
                {
                    char message[255];
                    char username_dest[USERNAME_REAL_SIZE];
                    printf("An wenn wollen Sie eine Nachricht schicken ? \n");
                    fgets(username_dest,USERNAME_REAL_SIZE,stdin);
                    printf("\n und nun bitte noch die Nachricht: \n");
                    fgets(message,255, stdin);
                    username_dest[strcspn(username_dest, "\n")] = 0;
                    sendMessageTo(SocketFD, sa, username_dest, USERNAME_REAL_SIZE,message, 255);
                }else {
                   // sendToAllMessage(SocketFD, sa, command, 255);
                }
                
            }
            
        }
    
    close(SocketFD);
    
    return 0;
}
