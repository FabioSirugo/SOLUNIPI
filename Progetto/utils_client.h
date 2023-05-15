#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#define RESPONSE_SIZE 256
#define UNIX_PATH_MAX 108
#define SOCKNAME "objstore.sock"
#define MAX_SIZE_FILE 255
/*																			Firma funzioni							 							*/

int os_connect(char * name);
int os_store(char * name, void * block, size_t len);
void * os_retrieve(char * name);
int os_delete(char * name);
int os_disconnect();

/*								Funzioni che mi garantiscono la scrittura e lettura di esattamente size byte.										*/

static inline int writen(long fd, void *buf, size_t size) {
	size_t left = size;
	int r;
	char *bufptr = (char*)buf;
	while(left>0) {
		if ((r=write((int)fd ,bufptr,left)) == -1) {
			if (errno == EINTR) continue;
			return -1;
		}
		if (r == 0) return 0;
		left    -= r;
		bufptr  += r;
	}
	return 1;
}

static inline int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // fine messaggio
    left    -= r;
	bufptr  += r;
    }
    return size;
}