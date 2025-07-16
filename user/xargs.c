#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

#define LINES 3
#define ARG_LEN 10

void _fmt_args(char *buf, char *args[LINES][MAXARG], char *argv[], int argc)
{
    int j = 0;
    int index = 0;
    int argvlen = argc - 1; // xargs 不算

    for (int i = 0; i < LINES; i++)
    {
        // printf("LINE %d\n", i);
        if (buf[j] == '\0')
        {
            // printf("str eof\n");
            break;
        }
 
        for (int k = 0; k < argvlen; k++) // 将原参数填充至 args 前面
        {
            args[i][k] = argv[k + 1];
        }
        
        for (int k = argvlen; k < MAXARG; k++)
        {
            index = 0;
            char *memtmp = malloc(ARG_LEN);
            memset(memtmp, atoi('\0'), sizeof(memtmp));

            while (buf[j] != '\n' && buf[j] != ' ')
            {
                // printf("now point ->%c<-\n", buf[j]);
                memtmp[index++] = buf[j++];
            }

            memtmp[index++] = '\0';
            args[i][k] = memtmp;
            // printf("%s\n", args[i][k]);

            if (buf[j] == '\n')
            {
                j++;
                break;
            }
            else if (buf[j] == ' ')
            {
                j++;
                continue;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int i;
    char buf[LINES * MAXARG * ARG_LEN];
    char *args[LINES][MAXARG];
    int p[2];
    int pid;

    pipe(p);

    // 获取标准输入的参数
    if (fork() == 0) // child
    {
        read(0, buf, LINES * MAXARG * ARG_LEN);

        // 写参数
        write(p[1], buf, strlen(buf));

        close(p[1]);
        close(p[0]);

        exit(1);
    }
    else
    {
        wait(&i);

        read(p[0], buf, LINES * MAXARG * ARG_LEN);

        // 格式化参数
        _fmt_args(buf, args, argv, argc);
        
        // 打印所有行的参数
        // for (int i = 0; i < LINES; i++)
        // {
        //     if (args[i][0] == (void *)0)
        //     {
        //         continue;
        //     }

        //     printf("LINE %d\n", i);

        //     for (int j = 0; j < MAXARG; j++)
        //     {
        //         if (args[0][j] == (void *)0) // 指针为空
        //         {
        //             continue;
        //         }
        //         printf("args-%d %s\n", j, args[i][j]);
        //     }
        // }

        for (int i = 0; i < LINES; i++)
        {
            if (args[i][0] == (void *)0)
            {
                break;
            }

            pid = fork();

            if (pid == 0)
            {
                exec(argv[1], args[i]);
            }
            else
            {
                wait((void *)0); // 不要 wait(&i); i 已经用来被迭代了 :(
            }
        }
    }

    exit(0);
}
