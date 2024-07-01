#include "request.h"
#include "asgn2_helper_funcs.h"

//defint the regex match
//#define REGEX1 "^([a-zA-Z]{1,8}) /([a-zA-Z0-9.-]{1,63}) (HTTP/[0-9]\\.[0-9])\r\n"
//#define REGEX2 "([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})\r\n"
#define REQUEST_REGEX        "([a-zA-Z0-9.-]{1,128})"
#define REQUESTID_REGEX      "([ -~]{1,128})"
#define REQUEST_METHOD_REGEX REQUEST_REGEX ": " REQUESTID_REGEX EMPTY_LINE_REGEX

//pthread_mutex_t mux;

void readRequest(Request *R) {
    memset(R->buf, '\0', BUFF_SIZE + 1);
    ssize_t bytes_read = read_until(R->fd, R->buf, R->size, "\r\n\r\n"); //get the bytes of read
    if (bytes_read < 0) {
        requestcon(R, 400, 0);
        exit(1);
    }
    memset(R->path, '\0', 64);
    memset(R->version, '\0', 4);
    memset(R->message_body, '\0', BUFF_SIZE + 1);
    memset(R->command, '\0', 5);
    regex_t req; //regex the req
    regmatch_t pmatch[4];
    int status_request;
    int total_read = 0;
    //debug("%s: %d", R->buf,errno);
    //fprintf(stderr,"Hello1");
    status_request = regcomp(&req, REQUEST_LINE_REGEX, REG_EXTENDED);
    //assert(!status_request);
    status_request = regexec(&req, R->buf, 4, pmatch, 0);
    /*if(status_request != 0){
        fprintf(stderr, "Hello1111");
        exit(1);
    }*/
    //fprintf(stderr,"Hello1");
    if (status_request == 0) {
        R->command = R->buf;
        R->path = R->buf + pmatch[2].rm_so; //get the path
        R->version = R->buf + pmatch[3].rm_so; //get the version

        R->buf[pmatch[1].rm_eo] = '\0';
        R->path[pmatch[2].rm_eo - pmatch[2].rm_so] = '\0';
        R->version[pmatch[3].rm_eo - pmatch[3].rm_so] = '\0';

        R->buf += pmatch[3].rm_eo + 2;
        total_read += pmatch[3].rm_eo + 2;
    } else {
        //fprintf(stderr,"Hello11");
        requestcon(R, 400, 0);
        regfree(&req);
        exit(1);
    }

    status_request = regcomp(&req, REQUEST_METHOD_REGEX, REG_EXTENDED);
    status_request = regexec(&req, R->buf, 3, pmatch, 0);
    if (status_request == 0) {
        R->buf[pmatch[1].rm_eo] = '\0';
        R->buf[pmatch[2].rm_eo] = '\0';
        if (strncmp(R->buf, "Request-Id", 14) == 0) {
            int Rid
                = strtol(R->buf + pmatch[2].rm_so, NULL, 10); //get the request id from the request
            //fprintf(stderr,"%d",length);
            if (errno == EINVAL) {
                requestcon(R, 400, 0);
            }
            R->id = Rid;
        }
        R->buf += pmatch[2].rm_eo + 2;
        total_read += pmatch[2].rm_eo + 2;
        status_request = regexec(&req, R->buf, 3, pmatch, 0);
        //debug("%s: %d", R->buf,errno);
    }
    //debug("%s: %d", R->buf,errno);
    //fprintf(stderr,"Hello12");
    status_request = regcomp(&req, HEADER_FIELD_REGEX, REG_EXTENDED);
    status_request = regexec(&req, R->buf, 3, pmatch, 0);
    if (status_request == 0) {
        R->buf[pmatch[1].rm_eo] = '\0';
        R->buf[pmatch[2].rm_eo] = '\0';
        if (strncmp(R->buf, "Content-Length", 14) == 0) {
            int length
                = strtol(R->buf + pmatch[2].rm_so, NULL, 10); //get the length from the request
            //fprintf(stderr,"%d",length);
            if (errno == EINVAL) {
                requestcon(R, 400, 0);
                fprintf(stderr, "GET,/%s,400,%d\n", R->path, R->id);
            }
            //debug("%d: %d", length,errno);
            R->contlen = length;
            //debug("%d: %d", R->contlen,errno);
        }
        //fprintf(stdout,"Hello13");
        R->buf += pmatch[2].rm_eo + 2;
        //debug("%s: %d", R->buf,errno);
        //fprintf(stdout,"Hello131");
        total_read += pmatch[2].rm_eo + 2;
        status_request = regexec(&req, R->buf, 3, pmatch, 0);
    }
    //fprintf(stdout,"Hello131");
    //debug("%s: %d", R->buf,errno);
    //fprintf(stderr,"Hello13");
    if ((status_request != 0) && (R->buf[0] == '\r' && R->buf[1] == '\n')) {
        //fprintf(stdout,"Hello13111");
        R->message_body = R->buf + 2;
        //debug("%zd: %d", bytes_read,errno);
        //debug("%d: %d", total_read,errno);
        total_read += 2;
        R->bytes = bytes_read - total_read;
        //debug("%d: %d", R->bytes,errno);
    } else if (status_request != 0) {
        //fprintf(stdout,"Hello90");
        requestcon(R, 400, 0);
        fprintf(stderr, "GET,/%s,400,%d\n", R->path, R->id);
        regfree(&req);
        exit(1);
    }
    regfree(&req);
    //exit(1);
    //fprintf(stdout,"Hello131");
    int method;
    //debug("%d: %d", R->id,errno);
    //debug("%s: %d", R->message_body,errno);
    //debug("%d: %d", R->contlen,errno);
    //debug("%s: %d", R->version,errno);
    if (strncmp(R->version, "HTTP/1.1", 8) != 0) {
        requestcon(R, 505, 0);
        fprintf(stderr, "GET,/%s,505,%d\n", R->path, R->id);
        //exit(1);
    } else if (strncmp(R->command, "GET", 3) == 0) {
        method = 1;
        //exit(1);
        //requestGet(R);
    } else if (strncmp(R->command, "PUT", 3) == 0) {
        method = 2;
        //exit(1);
        //requestPut(R);
    } else {
        requestcon(R, 501, 0);
        fprintf(stderr, "GET,/%s,501,%d\n", R->path, R->id);
        exit(1);
    }
    //fprintf(stdout,"hello3");
    pthread_mutex_lock(&R->server->mux);
    //exit(1);
    const char *filename = R->path;
    Filelock *file_lock = NULL;
    for (int i = 0; i < R->server->num_threads; i++) { //set the lock for each file
        if (R->server->file_locks[i].count == 0
            || strcmp(R->server->file_locks[i].filename, filename) == 0) {
            file_lock = &R->server->file_locks[i];
            break;
            //exit(1);
        }
    }
    //fprintf(stdout,"hello4");
    if (file_lock == NULL) {
        pthread_mutex_unlock(&R->server->mux);
        exit(1);
    }
    //assert(file_lock != NULL);
    //exit(1);
    if (file_lock->count == 0) {
        strcpy(file_lock->filename, filename); //get the filename to the lock
    }

    file_lock->count++; //increase the number of lock
    pthread_mutex_unlock(&R->server->mux);
    if (method == 1) {
        //fprintf(stdout,"hello6");
        reader_lock(file_lock->lock);
        requestGet(R);
        reader_unlock(file_lock->lock);
    } else if (method == 2) {
        //fprintf(stdout,"hello7");
        writer_lock(file_lock->lock);
        requestPut(R);
        writer_unlock(file_lock->lock);
        //exit(1);
    }
    pthread_mutex_lock(&R->server->mux);
    file_lock->count--;
    if (file_lock->count == 0) {
        file_lock->filename[0] = '\0';
    }
    //exit(1);
    //fprintf(stdout,"hello8");
    pthread_mutex_unlock(&R->server->mux);
    //exit(1);
    //return (EXIT_SUCCESS);
}
