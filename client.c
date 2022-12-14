#include <stdio.h>
#include "mfs.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 100000);
    printf("init %d\n", rc);

    rc = MFS_Creat(0, MFS_DIRECTORY, "asdfghjtklasdfghjklasdfghjk");
    rc = MFS_Lookup(0, "asdfghjtklasdfghjklasdfghjk");
    char * buffer = malloc(sizeof(MFS_DirEnt_t));
    rc = MFS_Read(0, buffer, 0, sizeof(MFS_DirEnt_t)); 
    printf("buffer %d\n", ((MFS_DirEnt_t *) buffer)->inum);

    rc = MFS_Shutdown();
    printf("shut %d\n", rc);
    return 0;
}

