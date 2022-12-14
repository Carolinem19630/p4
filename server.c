#include <stdio.h>
#include <sys/mman.h> 
#include "udp.h"
#include "ufs.h"
#include <sys/stat.h>
#include  <signal.h>
#include "message.h"

#define BUFFER_SIZE (10000)

int sd;
void * image;
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
    if (!get_bit((unsigned int *) (image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE)),  m->pinum)){
        m->rc = -1;
        return;
    }
    inode_t * parentInode = inode_table + (m->pinum * sizeof(inode_t));
    int i = 0;
    while(parentInode->direct[i] != -1){
        if (!get_bit((unsigned int *) (image + (s->data_bitmap_addr * UFS_BLOCK_SIZE)), parentInode->direct[i] - s->data_region_addr)){
            m->rc = -1;
            return;
        }
        else {
            // start of data block
            dir_ent_t * curr_direct = image + (parentInode->direct[i] * UFS_BLOCK_SIZE);
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

/*
MFS_Read() reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the buffer 
from file specified by inum. The routine should work for either a file or directory; directories should 
return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1. 
Failure modes: invalid inum, invalid offset, invalid nbytes.
*/
void _read(message_t * m){
    // check the bitmap to see if inum is valid - else failure
    if (!get_bit((unsigned int *) (image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE)),  m->inum)){
        m->rc = -1;
        return;
    }
    // find inode 
    inode_t * currInode = (void *) inode_table + (m->inum * sizeof(inode_t));
    // check that the offset is less than the size of the file / directory & check that the offset + nbytes is less than the size
    if (m->offset + m->nbytes > currInode->size){
        m->rc = -1;
        return;
    }
    
    // calculate the start of the reading in -> which block does it live in? - if offset + nbytes > block size -> then which block(s) does the data live in
    if (m->offset + m->nbytes > UFS_BLOCK_SIZE){
        // find first block 
        dir_ent_t * startBlock = image + (currInode->direct[m->offset % UFS_BLOCK_SIZE] * UFS_BLOCK_SIZE);
        char * tempBuffer = malloc(m->nbytes);
        startBlock = (dir_ent_t* ) ((char *) startBlock + m->offset);
        memcpy(tempBuffer, startBlock, UFS_BLOCK_SIZE - m->offset);
        tempBuffer = (dir_ent_t *)((char*) tempBuffer +(UFS_BLOCK_SIZE - m->offset));
        
        // find second block
        dir_ent_t * endBlock = image + (currInode->direct[(m->offset + m->nbytes) % UFS_BLOCK_SIZE] * UFS_BLOCK_SIZE);
        memcpy(tempBuffer, (char*) endBlock, m->nbytes - (UFS_BLOCK_SIZE - m->offset));
    }
    else {
        dir_ent_t * startBlock = image + (currInode->direct[m->offset % UFS_BLOCK_SIZE] * UFS_BLOCK_SIZE);
        char * tempBuffer = malloc(m->nbytes);
        memcpy(tempBuffer, (char*)startBlock, m->nbytes);
        m->buffer = malloc(m->nbytes);
        memcpy(m->buffer, tempBuffer, m->nbytes);
    } 
}

/*
* MFS_Creat() makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) 
* in the parent directory specified by pinum of name name. Returns 0 on success, -1 on failure. 
* Failure modes: pinum does not exist, or name is too long. If name already exists, return success.
*
* To create a file, the file
system must not only allocate an inode, but also allocate space within
the directory containing the new file. The total amount of I/O traffic to
do so is quite high: one read to the inode bitmap (to find a free inode),
one write to the inode bitmap (to mark it allocated), one write to the new
inode itself (to initialize it), one to the data of the directory (to link the
high-level name of the file to its inode number), and one read and write
to the directory inode to update it. If the directory needs to grow to accommodate the new entry, 
additional I/Os (i.e., to the data bitmap, and
the new directory block) will be needed too. All that just to create a file!
*/
void create(message_t * m){
    // check pinum is valid - gotta check inode bitmap 
    printf("0");
    if (!get_bit((unsigned int *) (image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE)),  m->pinum)){
        m->rc = -1;
        return;
    }
    printf("1");
    printf("2");
    // go through inode bitmap and find free inum + set bit
    for (int i = 0; i < (s->inode_bitmap_len * UFS_BLOCK_SIZE * 8); i++){
        if (get_bit((unsigned int *) (image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE)), i) == 0){
            m->inum = i;
            printf("\ninode bitmap\n%d\n", i);
            set_bit((unsigned int *)(image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE)), i);
            msync(image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE), s->inode_bitmap_len * UFS_BLOCK_SIZE, MS_SYNC);
            break;
        }
    }
    // create inode in table at the inum index
    inode_t * nInode = malloc(sizeof(inode_t)); 
    // set file type in inode
    nInode->type = m->type;
    // set size to 0
    if (m->type == UFS_DIRECTORY){
        nInode->size = UFS_BLOCK_SIZE;
    }
    else {
        nInode->size = 0;
    }
    // go through data bitmap and find free inum + set bit
    for (int i = 0; i < (s->data_bitmap_len * UFS_BLOCK_SIZE * 8); i++){
        if (get_bit(image +  (s->data_bitmap_addr * UFS_BLOCK_SIZE), i) == 0){
            nInode->direct[0] = s->data_region_addr + i; // set direct pointer to be the block
            // set first pointer to be -1
            nInode->direct[1] = -1;
            printf("\ninode bitmap 2\n%d\n", i);
            set_bit(image + (s->data_bitmap_addr), i);
            msync(image + (s->data_bitmap_addr * UFS_BLOCK_SIZE), s->data_bitmap_len * UFS_BLOCK_SIZE, MS_SYNC);
            break;
        }
    }
    memcpy(image + (s->inode_region_addr * UFS_BLOCK_SIZE) + (sizeof(inode_t) * m->inum), nInode, sizeof(inode_t));
    msync(image + (s->inode_region_addr * UFS_BLOCK_SIZE) + (sizeof(inode_t) * m->inum), sizeof(inode_t), MS_SYNC);
    // find parent directory data block 
    inode_t * parentInode = image + (s->inode_region_addr * UFS_BLOCK_SIZE) + (m->pinum * sizeof(inode_t));
    printf("\nparent Inode 2\n%p %d\n", parentInode, m->pinum);
    int i = 0; 
    int found = 0;
    while (parentInode->direct[i] != -1 && found == 0){
        dir_ent_t * directory = image + (parentInode->direct[i] *UFS_BLOCK_SIZE); 
        int j = 0; 
        while (j < UFS_BLOCK_SIZE/sizeof(dir_ent_t) && found == 0){
            if (directory[j].inum == -1){
                directory[j].inum = m->inum;
                strcpy(directory[j].name, m->name);
                found = 1;
                msync(image + (parentInode->direct[i] * UFS_BLOCK_SIZE), UFS_BLOCK_SIZE, MS_SYNC);
            }
            j++;
        }
        i++;
    }
    // add entry in parent directory data block - check that still inside data block - otherwise have to create a new 
    if (found == 0){
        for (int k = 0; k < (s->data_bitmap_len * UFS_BLOCK_SIZE * 8); k++){
            if (get_bit((void *)s +  s->data_bitmap_addr, k) == 0){
                parentInode->direct[i] = s->data_region_addr + k; // set direct pointer to be the block
                // set first pointer to be -1
                if (i != 29){
                    nInode->direct[i +1] = -1;
                }
                set_bit((void *)s + s->data_bitmap_addr, k);
                msync(image + s->data_bitmap_addr * UFS_BLOCK_SIZE, s->data_bitmap_len * UFS_BLOCK_SIZE, MS_SYNC);
                break;
            }
        }
        dir_ent_t * directory = image + (parentInode->direct[i] *UFS_BLOCK_SIZE); 
        directory[0].inum = m->inum;
        strcpy(directory[0].name, m->name);
        for (int k = 1; k < UFS_BLOCK_SIZE/sizeof(dir_ent_t); k++){
            directory[k].inum = -1;
        }
        msync(directory, UFS_BLOCK_SIZE, MS_SYNC);
        msync(image + s->inode_region_addr * UFS_BLOCK_SIZE, s->inode_region_len * UFS_BLOCK_SIZE, MS_SYNC);
    }
    // if the file is of type directory: 
    // have to add two entries: .. & .
    // don't forget to msync everything
    if (nInode->type == UFS_DIRECTORY){
        dir_ent_t * directory = image + (nInode->direct[0] *UFS_BLOCK_SIZE);
        strcpy(directory[0].name, ".");
        directory[0].inum = m->inum;
        strcpy(directory[1].name, "..");
        directory[1].inum = m->pinum;
        for (int k = 2; k < UFS_BLOCK_SIZE/sizeof(dir_ent_t); k++){
            directory[k].inum = -1;
        }
    }
    msync(image + (nInode->direct[0] *UFS_BLOCK_SIZE), UFS_BLOCK_SIZE, MS_SYNC); 
}

void shut_down(){
    msync(s, image_size, MS_SYNC);
    exit(0);
}

// server code
int main(int argc, char *argv[]) {
    signal(SIGINT, intHandler);
    int fd = open(argv[2], O_RDWR, 0666);
    if (fd <= 0){
        char error_message[30] = "image does not exist\n";
        if(write(STDERR_FILENO, error_message, strlen(error_message))){
            exit(1);
        } 
    }
    if (fd > 0){
        sd = UDP_Open(atoi(argv[1])); // pass in port number as arg
        assert(sd > -1);
        struct stat sbuf;
        message_t m;
        int rc = fstat(fd, &sbuf);
        assert(rc > -1);

        image_size = (int) sbuf.st_size;
        image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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
        printf("%p root_dir -CHECK\n", root_dir);
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
        case 5:
            _read(&m);
            break;
        case 6: 
            create(&m);
            break;
        case 8: 
            shut_down();
            break;
    }
	if (rc > 0) {
        m.rc = UDP_Write(sd, &addr, (char *) &m, sizeof(message_t));
	    printf("server:: reply\n");
	} 
    }}
    return 0; 
}

