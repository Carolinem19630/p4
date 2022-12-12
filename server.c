#include <stdio.h>
#include <sys/mman.h> 
#include "udp.h"
#include "ufs.h"
#include <sys/stat.h>
#include  <signal.h>
#include "message.h"

#define BUFFER_SIZE (10000)

int sd;
super_t * s;
inode_t * inode_table;
inode_t * root_inode;
dir_ent_t *root_dir;
int image_size;


void intHandler(int dummy) {
    UDP_Close(sd);
    exit(130);
}

unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= 0x1 << offset;
}

void lookup(message_t * m){
    // failure cases: pinum isn't valid or name doesn't exist in parent directory

    // check pinum is valid - gotta check inode bitmap 
    if (!get_bit((unsigned int *) ((void *) s + s->inode_bitmap_addr * UFS_BLOCK_SIZE),  m->pinum)){
        m->rc = -1;
        return;
    }
    inode_t * parentInode = inode_table + m->pinum * sizeof(inode_t);
    int i = 0;
    while(parentInode->direct[i] != -1){
        if (!get_bit((unsigned int *) ((void *) s + s->data_bitmap_addr * UFS_BLOCK_SIZE), parentInode->direct[i] - s->data_region_addr)){
            m->rc = -1;
            return;
        }
        else {
            // start of data block
            dir_ent_t * curr_direct = (void *) s + parentInode->direct[i] * UFS_BLOCK_SIZE;
            int j = 0;
            while(curr_direct[j].inum != -1 && strcmp(curr_direct[j].name, m->name) != 0 && j < 128){
                j++;
            }
            if (strcmp(curr_direct[j].name, m->name) == 0){
                m->inum = curr_direct[j].inum;
                return;
            }
        }
        i++;
    }
    m->rc = -1;
    return;
}

void shut_down(){
    msync(s, image_size, MS_SYNC);
    exit(0);
}

// server code
int main(int argc, char *argv[]) {
    signal(SIGINT, intHandler);
    int fd = open(argv[2], O_RDWR, 0666);
    if (fd > 0){
        sd = UDP_Open(atoi(argv[1])); // pass in port number as arg
        assert(sd > -1);
        struct stat sbuf;
        message_t m;
        int rc = fstat(fd, &sbuf);
        assert(rc > -1);

        image_size = (int) sbuf.st_size;
        void *image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        assert(image != MAP_FAILED);

        s = (super_t *) image;

        printf("inode bitmap address %d [len %d]\n", s->inode_bitmap_addr, s->inode_bitmap_len);
        printf("data bitmap address %d [len %d]\n", s->data_bitmap_addr, s->data_bitmap_len);

        inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
        root_inode = inode_table;
        printf("\nroot type:%d root size:%d\n", root_inode->type, root_inode->size);
        printf("direct pointers[0]:%d [1]:%d\n", root_inode->direct[0], root_inode->direct[1]);

        root_dir = image + (root_inode->direct[0] * UFS_BLOCK_SIZE);
        printf("\nroot dir entries\n%d %s\n", root_dir[0].inum, root_dir[0].name);
        printf("%d %s\n", root_dir[1].inum, root_dir[1].name);

        // change on-disk format(!)
        // s->inode_bitmap_len = 1;// don't do this ... just an example

        // force change!
        //rc = msync(s, sizeof(super_t), MS_SYNC);

    while (1) {
	struct sockaddr_in addr;
	
	printf("server:: waiting...\n");
	int rc = UDP_Read(sd, &addr, (char *) &m, sizeof(message_t));
	printf("server:: read message [size:%d contents:(%d)]\n", m.rc, m.mtype);
    switch(m.mtype){
        case 1: 
            break;
        case 2: 
            lookup(&m);
            break;
        case 8: 
            shut_down();
            break;
    }
	if (rc > 0) {
            message_t r; 
            r.mtype = m.mtype;
            r.inum = m.inum;
            r.rc = m.rc;
            r.pinum = m.pinum;
            r.name = m.name;
            m.rc = UDP_Write(sd, &addr, (char *) &r, sizeof(message_t));
	    printf("server:: reply\n");
	} 
    }}
    return 0; 
}

