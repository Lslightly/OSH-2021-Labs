#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

using namespace std;

#define MESSAGE_OFFSET 8    //  Message:字串的长度
#define MAX_GUEST 32        //  最大顾客数量
#define SEND_LEN_EACH 10    //  每次发送(send)的长度
#define MAX_BYTES   1048600 //  最大发送字节数

//  现有顾客数量
static int number_guests = 0;

//  客服端结构体
typedef struct Guest{
    int fd;             //  描述符
    pthread_t thread;   //  线程
}Guest;

Guest guests[MAX_GUEST];

ssize_t len = 0;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;


/// Description:
///     服务器处理聊天
/// Parameter: 
///     fd_send:    发送者
/// Return:
///     void
void *handle_chat(void * fd_send);

/// Description:
///     将recv得到的消息拼接到原有消息之上
/// Parameter:
///     data:   传入的消息
///     msgbuffer:    message buffer
///     msg_len:    message length
///     former_string:  在还没有发现\\n之前先将之前的字符串存起来
/// Return:
///     true:   得到一条消息，可以发送
///     false:  没有得到一条完整的消息，还不可以发送，还需要继续recv得到新的字符串直到发现\\n
bool split_enter(char * &data, char * &msgbuffer, ssize_t &msg_len, char * &former_string);

/// Description:
///     refresh guests, will decrease guests if guest don't reply
/// Parameters:
///     void
/// Return:
///     void
void refresh(int fd_send);

void *handle_chat(void *data) {
    int fd_send = *((int *)(data));

    char *buffer = (char *)malloc(sizeof(char)*10);
    strcpy(buffer, "");

    char *prompt = (char *)malloc(sizeof(char) * 1048600);
    strcpy(prompt, "Message:");

    char *former_string = (char *)malloc(sizeof(char) * 1048600);
    strcpy(former_string, "");

    ssize_t msg_len = 0;
    char * msgbuffer = nullptr;
    char * readin = nullptr;
    bool in_critical = false;
    while (1) {
        len = recv(fd_send, buffer, 5, 0);
        if (len <= ssize_t(0)) {
            printf("client %d quits.\n", fd_send);
            break;
        }
        if (!in_critical) {
            pthread_mutex_lock(&mutex);
            printf("client %d get mutex, will send.\n", fd_send);
        }
        in_critical = true;
        readin = buffer;
        if (!split_enter(readin, msgbuffer, msg_len, former_string)) continue;
        printf("client %d will send message\n", fd_send);
        strcpy(prompt+MESSAGE_OFFSET, msgbuffer);
        for (int j = 0; j < number_guests; j++) {
            if (guests[j].fd != fd_send) {
                for (int i = 0; i < (msg_len + MESSAGE_OFFSET) / SEND_LEN_EACH; i++) {
                    send(guests[j].fd, prompt+i*10, 10, 0);
                }
            }
        }
        for (int j = 0; j < number_guests; j++) {
            if (guests[j].fd != fd_send) {
                printf("client %d send to client %d\n", fd_send, guests[j].fd);
                send(guests[j].fd, prompt+(msg_len + MESSAGE_OFFSET)/SEND_LEN_EACH*SEND_LEN_EACH, msg_len + MESSAGE_OFFSET - (msg_len+MESSAGE_OFFSET)/10*10, 0);
            }
        }
        free(msgbuffer);
        msg_len = 0;
        in_critical = false;
        pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_lock(&mutex);
    refresh(fd_send);
    free(buffer);
    free(former_string);
    free(prompt);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

bool split_enter(char * &data, char * &msgbuffer, ssize_t &msg_len, char * &former_string) {
    char * split_pos = nullptr;
    if (split_pos = strstr(data, "\n")) {   //  find \n
        msgbuffer = (char *)malloc(sizeof(char) * (strlen(former_string) + split_pos-data+1));
        strcpy(msgbuffer, "");
        msg_len = 0;
        *split_pos = '\0';
        strcat(former_string, data);
        strcpy(msgbuffer, former_string);
        strcat(msgbuffer, "\n");
        strcpy(former_string, "");
        strcpy(data, "");
        msg_len = strlen(msgbuffer);
        return true;
    } else {
        strcat(former_string, data);
        strcpy(data, "");
        return false;
    }
}

void refresh(int fd_send) {
    printf("client %d close.\n", fd_send);
    close(fd_send);
    for (int i = 0; i < number_guests; i++) {
        if (guests[i].fd == fd_send) {
            pthread_join(guests[i].thread, NULL);
            for (int j = i; j < number_guests-1; j++) {
                guests[j] = guests[j+1];
            }
            number_guests--;
            printf("now guests: %d\n", number_guests);
        }
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
    if (listen(fd, 32)) {
        perror("listen");
        return 1;
    }
    while (1) {
        if (number_guests >= MAX_GUEST) {
            break;
        }
        guests[number_guests].fd = accept(fd, NULL, NULL);
        printf("client %d connect\n", guests[number_guests].fd);
        if (guests[number_guests].fd == -1) {
            perror("accept");
            return 1;
        }
        printf("client %d create thread\n", guests[number_guests].fd);
        while (pthread_create(&guests[number_guests].thread, NULL, handle_chat, &guests[number_guests].fd) != 0)
            ;
        number_guests++;
        printf("now guests: %d\n", number_guests);
    }
    return 0;
}
