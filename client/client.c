#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<fcntl.h>
#include<netdb.h>
#include <string.h>
#include<pthread.h>

#define h_addr h_addr_list[0]

#define BUFFER_SIZE 100
char user[50];

void usage() {
    printf("./client <SERVER-IP/HOSTNAME> <PORT> <CLIENT-USERNAME>");
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void *sendmessage(void *c) {
    char buf[BUFFER_SIZE];
    char com_buf[BUFFER_SIZE + sizeof(user) + 2];

    while(1) {
        int conn = *((int *)c);
        fgets(buf, sizeof(buf), stdin);
        buf[strcspn(buf, "\n")] = '\0';
        snprintf(com_buf, sizeof(com_buf), "%s: %s", user, buf);
        printf("\033[A\033[2K");
        printf("%s\n",com_buf); 
        if (send(conn, com_buf, strlen(com_buf), 0) < 0) {
            printf("Failed to send data to server\n");
            exit(0);
        }
    }
}

void *recvmessage(void *c) {
    int conn = *((int *) c);
    while(1) {
        char buf[100];
        int byte_received;
        while((byte_received = recv(conn, buf, sizeof(buf), 0)) > 0) {
            buf[byte_received] = '\0';
            printf("%s\n",buf);
            break;
        }
        fflush(stdout);
    }
}

int authenticate_client_to_server(int conn) {
    char buf[100];
    int byte_received;
    while((byte_received = recv(conn, buf, sizeof(buf), 0)) > 0) {
        buf[byte_received] = '\0';
        if ((strcmp("Hello", buf)) == 0) {
            printf("Received Hello from server\n");
            printf("Client ready to send msg to server\n");
        }
        break;
    }
    return 0;
    //fflush(stdout);
}


int main(int argc, char *argv[]) {

    int serversock, clientsock;
    struct sockaddr_in server, client;
    char buf[] = "HELLO from client!\n";
    char buf2[100];
    unsigned short port;
    struct hostent *svrhost;
    int auth_client = 0;
    pthread_t send_thread, recv_thread;
    char *endptr;

    if (argc < 4) {
        printf("./client <SERVER-IP/HOSTNAME> <PORT> <CLIENT-USERNAME>\n");
        exit(0);
    }
    svrhost = gethostbyname(argv[1]);
    unsigned long temp = strtoul(argv[2], &endptr, 10);
    port = (unsigned short) temp;
    strcpy(user, argv[3]);

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = *((unsigned long *)svrhost->h_addr);

    if ((serversock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Failed to create socket\n");
        exit(0);
    }
    printf("Waiting for server to authenticate...\n");
    if (connect(serversock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Failed to connect to server\n");
        exit(0);
    }
    auth_client = authenticate_client_to_server(serversock);
    if (auth_client == 0) {
        pthread_create(&send_thread, NULL, sendmessage, &serversock);
        pthread_create(&recv_thread, NULL, recvmessage, &serversock);
        pthread_join(send_thread, NULL);
        pthread_join(recv_thread, NULL);
    }
    
    close(serversock);
    printf("Client closed connection \n");
}