#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *file);

void find(char *path, char *file)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if ((fd = open(path, 0)) < 0)
	{
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}

	if (fstat(fd, &st) < 0)
	{
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
	{
		printf("find: path too long\n");
		close(fd);
		return;
	}
	strcpy(buf, path);
	p = buf + strlen(buf); // TODO: strlen buf is not always 512 ?
	*p++ = '/';

	while (read(fd, &de, sizeof(de)) == sizeof(de)) // 遍历目录下所有文件
	{
		if (de.inum == 0) // TODO: what does 0 means?
			continue;

		// get file type
		memmove(p, de.name, DIRSIZ);
		p[DIRSIZ] = 0;
		if (stat(buf, &st) < 0) // stat() : open() -> fstat()
		{
			printf("find: cannot stat %s\n", buf);
			continue;
		}

		if (st.type == 1) // if is a folder
		{
			if (strcmp(".", de.name) == 0 || strcmp("..", de.name) == 0)
			{
				continue;
			}
			
			find(buf, file); // 递归find
		}

		if (strcmp(de.name, file) == 0) // if is a file/device , and has the same name
		{
			fprintf(1, "%s\n", buf);
		}

	}
	close(fd);
}


int main(int argc, char *argv[])
{
	int i;

	if (argc < 2)
	{
		exit(0);
	}

	i = 1;
	find(argv[i], argv[i + 1]);

	exit(0);
}
