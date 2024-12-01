#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<ctype.h>
#include<string.h>
#include"server.h"

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int authenticate(struct sockaddr_in client) {
    int auth_user = 0;
    char admin_response[2];
    printf("Incoming connection from: %s\n",inet_ntoa(client.sin_addr));
    // TODO - implement authentication mechanism
    return 0;

}


void *sendmessage(void *c) {
    char buf[BUFFER_SIZE];
    char user[] = "Server";
    char com_buf[BUFFER_SIZE + sizeof(user) + 2];
    while(1) {
        int conn = *((int *)c);
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = '\0';
        snprintf(com_buf, sizeof(com_buf), "%s: %s", user, buf);
        printf("\033[A\033[2K");
        printf("%s\n",com_buf);
        if (send(conn, com_buf, strlen(com_buf), 0) < 0) {
            printf("Failed to send data to client\n");
            exit(0);
        }
    }
}

void *recvmessage(void *c) {
    int conn = *((int *)c);
    while(1) {
        char buf[100];
        int byte_received;
        fflush(stdout);
        while((byte_received = recv(conn, buf, sizeof(buf), 0)) > 0) {
            buf[byte_received] = '\0';
            printf("%s\n",buf);
            break;
        }
    }
}

void broadcast_msg(char *buf, int conn) {
    pthread_mutex_lock(&client_mutex);
    for(int i = 0; i < client_count; i++) {
        if (client_sockets[i] != conn) {
            if(send(client_sockets[i], buf, strlen(buf), 0) < 0) {
                continue;
            }
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void *do_client_stuff(void *cli_sock) {
    int conn = *((int *)cli_sock);
    int byte_received;
    char buf[BUFFER_SIZE];
    while(1) {
        memset(buf, 0, sizeof(buf));
        byte_received = recv(conn, buf, sizeof(buf), 0);
        if (byte_received <= 0) {
            close(conn);
            pthread_mutex_lock(&client_mutex);
            for(int i = 0; i < client_count; i++) {
                if (client_sockets[client_count] == conn) {
                    client_sockets[i] = client_sockets[client_count-1];
                    client_count = client_count -1;
                    break;
                }
            }
            pthread_mutex_unlock(&client_mutex);
            break;
        }
        buf[byte_received] = '\0';
        broadcast_msg(buf, conn);
    }
    free(cli_sock);
    
}


int main(int argc, char *argv[]) {

    int serversock, clientsock;
    struct sockaddr_in server, client;
    unsigned short port;
    int namelen;
    int *new_sock;
    pthread_t t1, t2;
    pthread_t client_thread;
    int optval = 1;
    int auth_user = 1;
    char ack[] = "Hello";
    printf("Starting server\n");
    if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Failed to create socket");
        exit(0);
    }
    // Set SO_REUSEADDR option to avoid TIME_WAIT state
    if (setsockopt(serversock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    memset(&server, 0, sizeof(server));
    port = 9001;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    if (bind(serversock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Failed in bind");
        exit(0);
    }

    listen(serversock, 10);
    printf("Waiting for client request...\n");
    namelen = sizeof(client);
    
    while(1) {
        if ((clientsock = accept(serversock, (struct sockaddr*)&client, &namelen)) < 0) {
            printf("Failed to accept connection from client\n");
            exit(0);
        }
        new_sock = malloc(sizeof(int));
        *new_sock = clientsock;
        pthread_mutex_lock(&client_mutex);
        client_sockets[client_count++] = clientsock;
        pthread_mutex_unlock(&client_mutex);
        auth_user = authenticate(client);
        if (auth_user == 0) {
            if (send(clientsock, ack, strlen(ack), 0) < 0) {
                printf("Failed to send data to client\n");
                free(new_sock);
                close(clientsock);
                continue;
            }
        }

        if (pthread_create(&client_thread, NULL, do_client_stuff, (void *)new_sock) != 0) {
            printf("Failed to create thread for client\n");
            close(clientsock);
        }

    }
    
    close(serversock);
    close(clientsock);
    printf("Server closed connection\n");
    return 0;
}
