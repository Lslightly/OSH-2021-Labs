#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        fprintf(stderr, "Fork error\n");
        return 1;
    }
    else if (pid == 0)
    {
        execl("/1", "/1", NULL);
        exit(1);
    }
    else
    {
        wait(NULL);
    }

    pid_t pid_2 = fork();

    if (pid_2 < 0)
    {
        fprintf(stderr, "Fork error\n");
        return 1;
    }
    else if (pid_2 == 0)
    {
        if (/*mknod("/dev/ttyS0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(4, 64)) == -1 ||*/ mknod("/dev/ttyAMA0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(204, 64)) == -1)
        {
            perror("mknod() for ttyS0 failed.\n");
        }
        // else {
        //     printf("success\n");
        // }
        int status = execl("/2", "/2", NULL);
        if (status != 0)
            printf("2 got wrong!\n");
        exit(1);
    }
    else
    {
        wait(NULL);
    }

    pid_t pid_3 = fork();

    if (pid_3 < 0)
    {
        fprintf(stderr, "Fork error\n");
        return 1;
    }
    else if (pid_3 == 0)
    {
        if (mknod("/dev/fb0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(29, 0)) == -1)
        {
            perror("mknod() for fb0 failed\n");
        }
        execl("/3", "/3", NULL);
        exit(1);
    }
    else
    {
        wait(NULL);
    }

    while (1)
    {
    }
    return 0;
}
