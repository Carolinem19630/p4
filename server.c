#include <stdio.h>
#include <sys/mman.h> 
#include "udp.h"
#include "ufs.h"
#include <sys/stat.h>
#include  <signal.h>
#include "message.h"

#define BUFFER_SIZE (1000)

int sd;

void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

// server code
int main(int argc, char *argv[]) {
    signal(SIGINT, intHandler);
    int fd = open(argv[2], O_RDWR, 0666);
    if (fd > 0){
        sd = UDP_Open(atoi(argv[1])); // pass in port number as arg
        assert(sd > -1);
        struct stat sbuf;
        int rc = fstat(fd, &sbuf);
        assert(rc > -1);

        int image_size = (int) sbuf.st_size;
        void *image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(image != MAP_FAILED);

        super_t * s = (super_t *) image;
        printf("inode bitmap address %d [len %d]\n", s->inode_bitmap_addr, s->inode_bitmap_len);
        printf("data bitmap address %d [len %d]\n", s->data_bitmap_addr, s->data_bitmap_len);

        inode_t * inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
        inode_t *root_inode = inode_table;
        printf("\nroot type:%d root size:%d\n", root_inode->type, root_inode->size);
        printf("direct pointers[0]:%d [1]:%d\n", root_inode->direct[0], root_inode->direct[1]);

        dir_ent_t *root_dir = image + (root_inode->direct[0] * UFS_BLOCK_SIZE);
        printf("\nroot dir entries\n%d %s\n", root_dir[0].inum, root_dir[0].name);
        printf("%d %s\n", root_dir[1].inum, root_dir[1].name);

        // change on-disk format(!)
        // s->inode_bitmap_len = 1;// don't do this ... just an example

        // force change!
        //rc = msync(s, sizeof(super_t), MS_SYNC);

    while (1) {
	struct sockaddr_in addr;
	message_t m;
	
	printf("server:: waiting...\n");
	m.rc = UDP_Read(sd, &addr, (char *) &m, sizeof(message_t));
	printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
	if (rc > 0) {
            message_t r; 
            r.mtype = 1;
            r.rc = UDP_Write(sd, &addr, (char *) &r, BUFFER_SIZE);
	    printf("server:: reply\n");
	} 
    }}
    return 0; 
}

