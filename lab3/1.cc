#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#define MESSAGE_OFFSET 8    //  "Message:" length

using namespace std;

typedef struct Pipe {
    int fd_send;
    int fd_recv;
}Pipe;

typedef struct Msg {
    char * buffer;
    int len;
}Msg;

bool split_enter(char * &data, Msg &msg, char * &former_string);

void *handle_chat(void *data) {
    struct Pipe *pipe = (struct Pipe *)data;
    char buffer[10] = "";
    char prompt[1048600] = "Message:";
    char *former_string;
    former_string = (char *)malloc(sizeof(char) * 1048600);
    strcpy(former_string, "");
    ssize_t len;
    ssize_t len_before = 0;
    Msg msg;
    char * readin = nullptr;
    while ((len = recv(pipe->fd_send, buffer, 5, 0)) > 0) {
        readin = buffer;
        if (!split_enter(readin, msg, former_string)) continue;
        strcpy(prompt+MESSAGE_OFFSET, msg.buffer);
        for (int i = 0; i < (msg.len + MESSAGE_OFFSET) / 10; i++) {
            send(pipe->fd_recv, prompt+i*10, 10, 0);
        }
        send(pipe->fd_recv, prompt+(msg.len + MESSAGE_OFFSET)/10*10, msg.len + MESSAGE_OFFSET - (msg.len+MESSAGE_OFFSET)/10*10, 0);
        free(msg.buffer);
        msg.len = 0;
    }
    return NULL;
}

bool split_enter(char * &data, Msg &msg, char * &former_string) {
    char * split_pos = nullptr;
    if (split_pos = strstr(data, "\n")) {   //  find \n
        msg.buffer = (char *)malloc(sizeof(char) * (strlen(former_string) + split_pos-data+1));
        msg.len = 0;
        int data_len = strlen(data);
        *split_pos = '\0';
        strcat(former_string, data);
        strcpy(msg.buffer, former_string);
        strcat(msg.buffer, "\n");
        memset(former_string, 0, sizeof former_string);
        memset(data, 0, sizeof data);
        msg.len = strlen(msg.buffer);
        return true;
    } else {
        strcat(former_string, data);
        memset(data, 0, sizeof data);
        return false;
    }
}

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2)) {
        perror("listen");
        return 1;
    }
    int fd1 = accept(fd, NULL, NULL);
    int fd2 = accept(fd, NULL, NULL);
    if (fd1 == -1 || fd2 == -1) {
        perror("accept");
        return 1;
    }
    pthread_t thread1, thread2;
    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;
    pthread_create(&thread1, NULL, handle_chat, (void *)&pipe1);
    pthread_create(&thread2, NULL, handle_chat, (void *)&pipe2);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}