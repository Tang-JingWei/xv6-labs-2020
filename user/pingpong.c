#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    int i;
    int p[2];
    char buf;

    pipe(p);

    if (fork() == 0)
    { // child
        read(p[0], &buf, 1);
        printf("%d: received ping\n", getpid());
        close(p[0]);
        write(p[1], &buf, 1);
        close(p[1]);
    }
    else
    {
        write(p[1], &buf, 1);
        wait(&i);
        read(p[0], &buf, 1);
        printf("%d: received pong\n", getpid());
        close(p[1]);
        close(p[0]);
    }

    exit(0);
}
