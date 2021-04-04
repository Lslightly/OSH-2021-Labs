#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sysmacros.h>
#include <stdlib.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Fork error\n");
        return 1;
    }
    else if (pid == 0) {
        execl("/1", "/1", NULL);
    }
    else {
        wait(NULL);
        printf("Child complete\n");
    }
    
    pid_t pid_2 = fork();

    if (pid_2 < 0) {
        fprintf(stderr, "Fork error\n");
        return 1;
    }
    else if (pid_2 == 0) {
        if (mknod("/dev/ttyS0", S_IFCHR | S_IRUSR | S_IWUSR, makedev(4, 64)) == -1) {
            perror("mknod() for ttyS0 failed.\n");
        }
        int status = execl("/2", "/2", NULL);
        if (status != 0)
            printf("2 got wrong!\n");
    }
    else {
        wait(NULL);
        printf("Child complete\n");
    }

    pid_t pid_3 = fork();

    if (pid_3 < 0) {
        fprintf(stderr, "Fork error\n");
        return 1;
    }
    else if (pid_3 == 0) {
        if (mknod("/dev/fb0", S_IFCHR |S_IRUSR|S_IWUSR, makedev(29, 0)) == -1)  {
        perror("mknod() for fb0 failed\n");       
    }
        execl("/3", "/3", NULL);
    }
    else {
        wait(NULL);
        printf("Child complete\n");
    }

    while (1) {}
    return 0;
}


