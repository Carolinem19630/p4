#include <stdio.h>
#include "mfs.h"
#include "message.h"
#include "udp.h"

#define BUFFER_SIZE (10000)
int sd; 
int MFS_Init(char *hostname, int port) {
    printf("MFS Init2 %s %d\n", hostname, port);
    struct sockaddr_in addrSnd, addrRcv;
    sd = UDP_Open(20000);
    message_t m;
    int rc = UDP_FillSockAddr(&addrSnd, hostname, port);

    m.mtype = MFS_INIT;
    rc = UDP_Write(sd, &addrSnd, (char *) &m, sizeof(message_t));
    if (rc < 0) {
	    printf("client:: failed to send\n");
	    exit(1);
    }

    printf(" NEW client:: wait for reply...\n");
    rc = UDP_Read(sd, &addrRcv, (char *) &m, sizeof(message_t));
    printf("client:: got reply [size:%d contents:(%d)\n", rc, m.mtype);
    return rc;
}

/*
* Takes the parent inode number (which should be the inode number of a directory) 
* and looks up the entry name in it. The inode number of name is returned. 
* Success: return inode number of name; failure: return -1. 
* Failure modes: invalid pinum, name does not exist in pinum.
*/
int MFS_Lookup(int pinum, char *name) {
    // network communication to do the lookup to server
    message_t m;
    struct sockaddr_in addrSnd, addrRcv;
    m.mtype = MFS_LOOKUP;
    m.pinum = pinum; 
    m.name = name;
    m.rc = UDP_Write(sd, &addrSnd, (char *) &m, sizeof(message_t));
    if (m.rc < 0) {
	    printf("client:: failed to send\n");
	    exit(1);
    }

    printf(" NEW client:: wait for reply...\n");
    m.rc = UDP_Read(sd, &addrRcv, (char *) &m, sizeof(message_t));
    printf("client:: got reply [size:%d contents:(%d)\n", m.rc, m.mtype);
    if (m.rc == -1){
        return -1;
    }
    return m.inum;
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
