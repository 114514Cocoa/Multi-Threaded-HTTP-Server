#include "asgn2_helper_funcs.h"
#include "queue.h"
#include "rwlock.h"
#include "debug.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>

#define BUFF_SIZE 4096
#define PATH_SIZE 64

typedef struct Filelock {
    char filename[65];
    rwlock_t *lock;
    int count;
} Filelock;

typedef struct Job {
    int client_fd;
} Job;

typedef struct Server {
    int num_threads;
    pthread_mutex_t mux;
    queue_t *q;
    Filelock *file_locks;
} Server;

//the struct of request
typedef struct Request {
    int id;
    int fd;
    long long contlen;
    int bytes;
    int size;
    char *buf;
    char *path;
    char *message_body;
    char *version;
    char *command;
    Server *server;
    pthread_t thread;
} Request;

//create the server
void createserver(Server *server, int queue_size, int num_threads);

//delete the server
void deleteserver(Server *server);

//initial the request
void requestinit(Request *R);

//free the request
void requestfree(Request *R);

void requestdelete(Request *R);
//the stauts of request
void requestcon(Request *R, int code, int size);

//if the command is get
int requestGet(Request *R);

//if the command is put
int requestPut(Request *R);
