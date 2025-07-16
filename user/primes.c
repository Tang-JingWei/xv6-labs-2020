#include "kernel/types.h"
#include "user/user.h"

#define BUFLEN 100
#define PRIMES_LEN 34

int main(int argc, char *argv[])
{
	// int i;
	int p[2];
	uint8 numbers[PRIMES_LEN];
	uint8 rec[PRIMES_LEN];
	uint8 tmp[PRIMES_LEN];

	for (int i = 0; i < PRIMES_LEN; i++)
	{
		numbers[i] = i + 2;
	}

	// for (int i = 0; i < PRIMES_LEN; i++)
	// {
	// 	printf("%d ", numbers[i]);
	// }

	pipe(p);

	for (int i = 0; i < 11; i++)
	{
		if (fork() == 0) // child
		{
			memset(rec, 0, PRIMES_LEN);
			read(p[0], rec, PRIMES_LEN);

			// 素数打印
			printf("prime %d\n", rec[0]);

			int index = 0;

			// 筛子
			for (int j = 1; j < PRIMES_LEN; j++)
			{
				if (!rec[j])
				{
					break;
				}

				if (rec[j] % rec[0] != 0)
				{
					tmp[index++] = rec[j];
				}
			}

			if (i != 10)
			{
				// write
				write(p[1], tmp, index);
			}

			exit(0);
		}
		else
		{
			if (i == 0)
			{
				// write 2-35
				write(p[1], numbers, PRIMES_LEN);
			}

			wait((void *)0);

			// printf("all child finish\n");
		}
	}

	exit(0);
}
