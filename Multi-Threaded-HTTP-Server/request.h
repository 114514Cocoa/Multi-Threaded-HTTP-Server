#include "asgn2_helper_funcs.h"
#include "response.h"
#include "protocol.h"

#include <stdio.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <string.h>

//read the request
void readRequest(Request *R);
