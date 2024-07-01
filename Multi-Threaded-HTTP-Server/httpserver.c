#include "request.h"

#include <pthread.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <bits/getopt_core.h>
#include <unistd.h>
#include <signal.h>

#define OPTIONS "-t"

//define the working thread
void *wthread(void *arg) {
    Request *R = (Request *) arg;
    Server *server = R->server;
    while (1) {
        void *job = NULL;
        //fprintf(stdout,"hello");
        bool rtn = queue_pop(server->q, &job); //pop the work from the queue
        if (rtn != 1) { //check the pop queue
            fprintf(stdout, "queue_pop failed \n");
            exit(1);
        }
        requestinit(R); //init the request
        Job *req = (Job *) job;
        R->fd = req->client_fd;
        free(req);
        //fprintf(stdout,"hello1");
        readRequest(R); //read the request
        //fprintf(stdout,"hello2");
        //exit(1);
        requestfree(R); //free the request
        //exit(1);
        R->fd = -1; //reset the fd
        //exit(1);
    }
    //exit(1);
    return NULL;
}

int main(int argc, char *argv[]) {
    int num_threads = 4;
    int port = 0;
    /*debug("%d: %d", argc,errno);
    debug("%s: %d", argv[0],errno);
    debug("%s: %d", argv[1],errno);
    debug("%s: %d", argv[2],errno);
    debug("%s: %d", argv[3],errno);*/
    if (argc == 4) {
        if (strcmp(argv[1], OPTIONS) != 0) {
            warnx("Wrong Command\n");
            exit(1);
        }
        num_threads = atoi(argv[2]); //get the number of threads
        if (num_threads <= 0) {
            warnx("The number of threads: %d\n", num_threads);
            exit(1);
        }
        port = atoi(argv[3]);
    } else if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        //fprintf(stdout,"can not\n");
        warnx("Please enter the correct command,\n");
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);

    Listener_Socket listener;
    //debug("%d: %d", port,errno);
    if (listener_init(&listener, port) != 0) {
        return 1;
        exit(1);
    }

    Server server;
    createserver(&server, 4097, num_threads);

    Request *R = malloc(num_threads * sizeof(Request));
    //fprintf(stdout,"Initial");
    for (int i = 0; i < num_threads; i++) {
        R[i].server = &server;
        requestinit(&R[i]); //init the request
        memset(&R[i].thread, 0, sizeof(R[i].thread));
        //exit(1);
        int checks = pthread_create(&R[i].thread, NULL, wthread, &R[i]);
        //exit(1);
        if (checks != 0) {
            fprintf(stdout, "Error creating thread %d\n", i);
            exit(1);
        }
        //debug("%d: %d", i,errno);
    }
    //exit(1);

    while (1) {
        int client_fd = listener_accept(&listener);
        //exit(1);
        if (client_fd < 0) {
            //exit(1);
            continue;
        }
        //exit(1);
        Job *req = malloc(sizeof(Job));
        req->client_fd = client_fd;
        //exit(1);
        bool rtn = queue_push(server.q, req); //push the work from the queue
        //exit(1);
        if (!rtn) { //check the push queue
            fprintf(stdout, "queue push failed\n");
            exit(1);
        }
        //int j = 0;
        //j++;
        //debug("%d: %d", j,errno);
        //exit(1);
    }
    //exit(1);
    for (int i = 0; i < num_threads; i++) {
        pthread_join(R[i].thread, NULL);
        //debug("%d: %d", i,errno);
    }

    close(listener.fd); //close the server
    deleteserver(&server);
    //exit(1);
    return 0;
}
