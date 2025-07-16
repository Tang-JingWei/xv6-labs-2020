#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        fprintf(2, "Usage: sleep <tick>...\n");
        exit(1);
    }

    for (i = 1; i < argc; i++)
    {
        // printf("arg : %s\n", argv[i]);
        sleep(atoi(argv[i]));
    }

    printf("time over\n");

    exit(0);
}
