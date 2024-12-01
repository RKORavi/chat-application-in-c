#include <netinet/in.h>
#include <pthread.h>
#define BUFFER_SIZE 100
#define MAX_CONNECTIONS 100
int client_sockets[MAX_CONNECTIONS];
int client_count = 0;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;