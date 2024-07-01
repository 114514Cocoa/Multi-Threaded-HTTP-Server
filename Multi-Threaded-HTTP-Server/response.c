#include "response.h"
#include "asgn2_helper_funcs.h"
rwlock_t *lock1;
void createserver(Server *server, int queue_size, int num_threads) {
    pthread_mutex_init(&server->mux, NULL);
    server->num_threads = num_threads;
    server->file_locks = malloc(num_threads * sizeof(*server->file_locks));
    for (int i = 0; i < num_threads; ++i) {
        Filelock *file_lock = &server->file_locks[i];
        file_lock->lock = rwlock_new(READERS, 0);
        file_lock->count = 0;
        memset(file_lock->filename, 0, sizeof(file_lock->filename));
    }
    server->q = queue_new(queue_size);
}

void deleteserver(Server *server) {
    queue_delete(&server->q);
    pthread_mutex_destroy(&server->mux);
    for (int i = 0; i < server->num_threads; ++i) {
        rwlock_delete(&server->file_locks[i].lock);
    }
    free(server->file_locks);
}

void requestinit(Request *R) {
    //R->fd = socket_fd;
    R->contlen = -1;
    R->bytes = 0;
    R->buf = malloc(BUFF_SIZE + 1);
    R->size = BUFF_SIZE + 1;
    R->id = 0;
    R->path = malloc(65);
    R->message_body = malloc(BUFF_SIZE);
    R->version = malloc(4);
    R->command = malloc(5);
    R->path[0] = '\0';
    R->message_body[0] = '\0';
    R->version[0] = '\0';
    R->command[0] = '\0';
}

void requestfree(Request *R) {
    while (read_n_bytes(R->fd, R->buf, 2048) > 0) { //check read of the bytes
        continue;
    }
    close(R->fd); //close the file
    //free(R->buf); //free the memory
}

void requestcon(Request *R, int code, int size) {
    if (code == 200) { //status phrase OK
        if (size == 2) {
            dprintf(R->fd, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nOK\n", size + 1);
        } else {
            dprintf(R->fd, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", size);
        }
    } else if (code == 201) { //status phrase Created
        dprintf(R->fd, "HTTP/1.1 201 Created\r\nContent-Length: %d\r\n\r\nCreated\n", 8);
    } else if (code == 400) { //status phrase Bad Request
        dprintf(R->fd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
    } else if (code == 403) { //status phrase Forbidden
        dprintf(R->fd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
    } else if (code == 404) { //status phrase Not Found
        dprintf(R->fd, "HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\n\r\nNot Found\n", 10);
    } else if (code == 500) { //status phrase Internal Server Error
        dprintf(R->fd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            23);
    } else if (code == 501) { //status phrase Not Implemented
        dprintf(R->fd,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\nNot Implemented\n", 16);
    } else if (code == 505) { //status phrase Version Not Supported
        dprintf(R->fd,
            "HTTP/1.1 505 Version Not Supported\r\nContent-Length: %d\r\n\r\nVersion Not "
            "Supported\n",
            22);
    }
}

int requestGet(Request *R) {
    //int status = 0;
    if (R->contlen != -1) {
        requestcon(R, 400, 0);
        //status = 400;
        fprintf(stderr, "GET,/%s,400,%d\n", R->path, R->id);
        exit(1);
    }
    int fd;
    const char *filename = R->path; //get the filename
    //fprintf(stderr,"Hello123");

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        if (errno == EACCES) {
            requestcon(R, 403, 0);
            fprintf(stderr, "GET,/%s,403,%d\n", R->path, R->id);
        } else if (errno == ENOENT) {
            fprintf(stderr, "%s", filename);
            requestcon(R, 404, 0);
            fprintf(stderr, "GET,/%s,404,%d\n", R->path, R->id);
        } else {
            requestcon(R, 500, 0);
            fprintf(stderr, "GET,/%s,500,%d\n", R->path, R->id);
        }
        exit(1);
    }
    //fprintf(stderr,"Hello1");
    //get the size of file
    struct stat stat_size;
    if (fstat(fd, &stat_size) < 0) {
        requestcon(R, 500, 0);
        fprintf(stderr, "GET,/%s,500,%d\n", R->path, R->id);
        exit(1);
    }
    if (!S_ISREG(stat_size.st_mode)) {
        requestcon(R, 403, 0);
        fprintf(stderr, "GET,/%s,403,%d\n", R->path, R->id);
        exit(1);
    }
    off_t file_size = stat_size.st_size;
    //fprintf(stderr,"Hello1");
    requestcon(R, 200, file_size);
    fprintf(stderr, "GET,/%s,200,%d\n", R->path, R->id);
    int bytes_written = pass_n_bytes(fd, R->fd, file_size);
    if (bytes_written == -1) {
        requestcon(R, 500, 0);
        fprintf(stderr, "GET,/%s,500,%d\n", R->path, R->id);
        exit(1);
    }
    //fprintf(stderr,"Hello1");
    close(fd);
    return 0;
}

int requestPut(Request *R) {
    //fprintf(stdout,"hello9");
    //writer_lock(lock1);
    int a = 2;
    if (R->contlen == -1) {
        //fprintf(stderr,"Hello1");
        requestcon(R, 400, 0);
        //fprintf(stdout,"hello12");
        fprintf(stderr, "PUT,/%s,400,%d\n", R->path, R->id);
        exit(1);
    }
    //fprintf(stdout,"hello11");
    int fd;
    const char *filename = R->path; //get the file of name
    bool check = true;
    fd = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0 && errno == EEXIST) {
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        check = false;
    }
    if (fd < 0) {
        if (errno == EACCES) {
            requestcon(R, 403, 0);
            fprintf(stderr, "PUT,/%s,403,%d\n", R->path, R->id);
            exit(1);
        } else {
            requestcon(R, 500, 0);
            fprintf(stderr, "PUT,/%s,500,%d\n", R->path, R->id);
            exit(1);
        }
    }
    //fprintf(stdout,"hello101");
    //debug("%d: %d", R->bytes,errno);
    int bytes_written = write_n_bytes(fd, R->message_body, R->bytes);
    if (bytes_written == -1) {
        requestcon(R, 500, 0);
        fprintf(stderr, "PUT,/%s,500,%d\n", R->path, R->id);
        exit(1);
    }
    //int length = R->contlen;
    //fprintf(stderr,"%d",length);
    int total_written = R->contlen - R->bytes;
    bytes_written = pass_n_bytes(R->fd, fd, total_written);
    if (bytes_written == -1) {
        //fprintf(stderr,"Hello1");
        requestcon(R, 500, 0);
        fprintf(stderr, "PUT,/%s,500,%d\n", R->path, R->id);
        exit(1);
    }
    //fprintf(stdout,"hello111");
    if (check == true) {
        requestcon(R, 201, 0);
        //fprintf(stdout,"hello1112");
        fprintf(stderr, "PUT,/%s,201,%d\n", R->path, R->id);
    } else {
        requestcon(R, 200, a);
        //fprintf(stdout,"hello1113");
        fprintf(stderr, "PUT,/%s,200,%d\n", R->path, R->id);
    }
    close(fd);
    //writer_unlock(lock1);
    return 0;
}
