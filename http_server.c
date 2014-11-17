#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "thread_pool.h"
#include "seats.h"
#include "util.h"

#define BUFSIZE 1024
#define FILENAMESIZE 100

void shutdown_server(int);

int listenfd;
pool_t* threadpool;
int jobno = 0;

int main(int argc,char *argv[])
{
    int flag, num_seats = 20;
    int connfd = 0;
    struct sockaddr_in serv_addr;

    char send_buffer[BUFSIZE];

    listenfd = 0;

    int server_port = 8080;

    if (argc > 1)
    {
        num_seats = atoi(argv[1]);
    }

    if (server_port < 1500)
    {
        fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
        exit(-1);
    }

    if (signal(SIGINT, shutdown_server) == SIG_ERR)
        printf("Issue registering SIGINT handler");

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( listenfd < 0 ){
        perror("Socket");
        exit(errno);
    }
    printf("Established Socket: %d\n", listenfd);
    flag = 1;
    setsockopt( listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag) );

    // initialize the threadpool
    // Set the number of threads and size of the queue
    printf("About to create theadpool\n");
    threadpool = pool_create(20, MAX_THREADS);

    // Load the seats;
    printf("Loading seats\n");
    pthread_mutex_init(&slock, NULL);
    load_seats(num_seats); //TODO read from argv

    // set server address
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(send_buffer, '0', sizeof(send_buffer));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(server_port);

    // bind to socket
    if (bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) != 0)
    {
        perror("socket--bind");
        exit(errno);
    }

    // listen for incoming requests
    listen(listenfd, 30);

    // handle connections loop (forever)
    while(1)
    {
        if (JDEBUG) printf("Listening for connections...\n");
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        if (JDEBUG) printf("Connection received: Added job %d\n", ++jobno);

        errno = pool_add_task(threadpool,
                (void*)&handle_connection,
                (void*)connfd,
                jobno);

        if(errno) printf("Error adding task\n");
    }
}

void shutdown_server(int signo) {
    pool_destroy(threadpool);
    unload_seats();
    close(listenfd);
    exit(0);
}
