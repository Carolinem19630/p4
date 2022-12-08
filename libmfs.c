#include <stdio.h>
#include "mfs.h"
#include "message.h"
#include "udp.h"

#define BUFFER_SIZE (1000)

int MFS_Init(char *hostname, int port) {
    printf("MFS Init2 %s %d\n", hostname, port);
    struct sockaddr_in addrSnd, addrRcv;
    int sd = UDP_Open(20000);
    message_t m;
    int rc = UDP_FillSockAddr(&addrSnd, hostname, port);

    m.mtype = MFS_INIT;
    rc = UDP_Write(sd, &addrSnd, (char *) &m, BUFFER_SIZE);
    if (rc < 0) {
	    printf("client:: failed to send\n");
	    exit(1);
    }

    printf("client:: wait for reply...\n");
    rc = UDP_Read(sd, &addrRcv, (char *) &m, BUFFER_SIZE);
    printf("client:: got reply [size:%d contents:(%d)\n", rc, m.mtype);
    return rc;
}

int MFS_Lookup(int pinum, char *name) {
    // network communication to do the lookup to server
    return 0;
}

int MFS_Stat(int inum, MFS_Stat_t *m) {
    return 0;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes) {
    return 0;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes) {
    return 0;
}

int MFS_Creat(int pinum, int type, char *name) {
    return 0;
}

int MFS_Unlink(int pinum, char *name) {
    return 0;
}

int MFS_Shutdown() {
    printf("MFS Shutdown\n");
    return 0;
}
