#include <stdio.h>
#include "mfs.h"

int main(int argc, char *argv[]) {
    int rc = MFS_Init("localhost", 100000);
    printf("init %d\n", rc);

    rc = MFS_Creat(0, MFS_DIRECTORY, "asdfghjtklasdfghjklasdfghjk");
    rc = MFS_Lookup(0, "asdfghjtklasdfghjklasdfghjk");

    rc = MFS_Shutdown();
    printf("shut %d\n", rc);
    return 0;
}

