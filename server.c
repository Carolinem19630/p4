#include <stdio.h>
#include <sys/mman.h> 
#include "udp.h"
#include "ufs.h"

#define BUFFER_SIZE (1000)

super_t * super; // super block
unsigned int * iBitmap; // inode bitmap 
unsigned int * dBitmap; // data bitmap
inode_t * inodeTable; // inode table

// server code
int main(int argc, char *argv[]) {
    int fd = open(argv[2], O_RDWR, 0666);
    if (fd < 0){
        int sd = UDP_Open(argv[1]); // pass in port number as arg
        assert(sd > -1);
        fseek(fd, 0, SEEK_END); // seek to end of file
        int size = ftell(fd); // get current file pointer
        fseek(fd, 0, SEEK_SET); 
        // read in superblock
        super = malloc(sizeof(super_t));
        read(fd, super, UFS_BLOCK_SIZE);
        // read in inode bitmap
        iBitmap = malloc(super->inode_bitmap_len * UFS_BLOCK_SIZE);
        fseek(fd, super->inode_bitmap_addr, SEEK_SET);
        read(fd, &iBitmap, super->inode_bitmap_len);
        // read in data bitmap 
        dBitmap = malloc(super->data_bitmap_len * UFS_BLOCK_SIZE);
        fseek(fd, super->data_bitmap_addr, SEEK_SET);
        read(fd, &dBitmap, super->data_bitmap_len);
        // read in inode table
        inodeTable = malloc(UFS_BLOCK_SIZE * super->inode_region_len);
        fseek(fd, super->inode_region_addr, SEEK_SET);
        read(fd, &inodeTable, super->inode_region_len);

    while (1) {
	struct sockaddr_in addr;
	char message[BUFFER_SIZE];
	//printf("server:: waiting...\n");
	int rc = UDP_Read(sd, &addr, message, BUFFER_SIZE);
	printf("server:: read message [size:%d contents:(%s)]\n", rc, message);
	if (rc > 0) {
            char reply[BUFFER_SIZE];
            sprintf(reply, "goodbye world");
            rc = UDP_Write(sd, &addr, reply, BUFFER_SIZE);
	    printf("server:: reply\n");
	} 
    }}
    return 0; 
}