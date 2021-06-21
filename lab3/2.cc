#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

#define MESSAGE_OFFSET 8  //  Message:字串的长度
#define MAX_GUESTS 32      //  最大顾客数量
#define SEND_LEN_EACH 10  //  每次发送(send)的长度
#define MAX_BYTES 1048600 //  最大发送字节数

//  现有顾客数量
int number_guests = 0;
int connect_numbers = 0;
int debug_info_number = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
// bool quit_done = true;
// pthread_t quit_thread = 0;
// int quit_fd = 0;

//  客户端结构体
typedef struct Guest
{
    int fd;           //  描述符
    pthread_t thread; //  线程
} Guest;

Guest guests[MAX_GUESTS];

void debug_info();

inline void mutex_lock(int fd)
{
    pthread_mutex_lock(&mutex);
    printf("client %d lock the mutex.\n", fd);
}

inline void mutex_unlock(int fd)
{
    pthread_mutex_unlock(&mutex);
    printf("client %d unlock the mutex.\n", fd);
}

inline void join(pthread_t thread)
{
    pthread_join(thread, nullptr);
    printf("thread %lu joins.\n", thread);
    printf("----------------------------------------------------------------\n");
}

inline void detach(pthread_t thread)
{
    pthread_detach(thread);
    printf("thread %lu detached.\n", thread);
}

inline void fd_close(int fd)
{
    close(fd);
    printf("fd %d is closed\n", fd);
}

inline void refresh(int fd);

/// Description:
///     服务器处理聊天
/// Parameter:
///     fd_send:    发送者
/// Return:
///     void
void *handle_chat(void *fd_send);

/// Description:
///     处理退出客户端
/// Parameters:
///
/// Return:
///
void *handle_quit(void *data);

/// Description:
///     将recv得到的消息拼接到原有消息之上
/// Parameter:
///     data:   传入的消息
///     msgbuffer:    message buffer
///     former_string:  在还没有发现\\n之前先将之前的字符串存起来
/// Return:
///     true:   得到一条消息，可以发送
///     false:  没有得到一条完整的消息，还不可以发送，还需要继续recv得到新的字符串直到发现\\n
bool split_enter(string &data, string &former_string);

/// Description:
///     释放资源
/// Parameters:
///     fd  客户描述符
///     buffer 消息缓冲
///     former_string   前字串
///     prompt  提示符Message:
/// Return:
///     nothing
inline void free_resource(int fd, char *buffer, char *former_string, char *prompt);

void send_to_guests(string &msgbuffer, int fd_send)
{
    string prompt = "Message:";
    printf("client %d will send message\n", fd_send);
    prompt += msgbuffer;
    int msg_len = msgbuffer.length();
    for (int j = 0; j < number_guests; j++)
    {
        if (guests[j].fd != fd_send)
        {
            printf("client %d send to client %d\n", fd_send, guests[j].fd);
            for (int i = 0; i < (msg_len + MESSAGE_OFFSET) / SEND_LEN_EACH; i++)
            {
                send(guests[j].fd, &prompt[0] + i * SEND_LEN_EACH, SEND_LEN_EACH, 0);
            }
            send(guests[j].fd, &prompt[0] + (msg_len + MESSAGE_OFFSET) / SEND_LEN_EACH * SEND_LEN_EACH, msg_len + MESSAGE_OFFSET - (msg_len + MESSAGE_OFFSET) / SEND_LEN_EACH * SEND_LEN_EACH, 0);
        }
    }
}

void *handle_chat(void *data)
{

    //  initialise
    int fd_send = *((int *)(data));

    char buffer[SEND_LEN_EACH] = "";

    string readin;

    ssize_t msg_len = 0;
    string msgbuffer;
    bool in_critical = false;
    int len = 0;

    while (1)
    {
        len = recv(fd_send, buffer, SEND_LEN_EACH, 0);
        readin = buffer;
        if (len <= ssize_t(0))
        { //  guest quits
            printf("length is %d. client %d quits.\n", len, fd_send);
            break;
        }
        if (!in_critical)
        { //  没有在临界区，需要锁住mutex
            printf("client %d enters critical section.\n", fd_send);
            mutex_lock(fd_send);
        }
        in_critical = true;
        if (!split_enter(readin, msgbuffer))
            continue;

        //  receive full message
        send_to_guests(msgbuffer, fd_send);
        msgbuffer = "";
        in_critical = false;
        mutex_unlock(fd_send);
    }
    refresh(fd_send);
    return NULL;
}

bool split_enter(string &data, string &former_string) {
    ssize_t split_pos = 0;
    split_pos = data.find('\n');
    if (split_pos != string::npos) { //  find \n
        data.erase(split_pos);
        former_string += data;
        former_string += '\n';
        return true;
    } else {
        former_string += data;
        return false;
    }
}

// void *handle_quit(void *data)
// {
//     while (1)
//     {
//         while (quit_done)
//         {
//         }
//         printf("detection thread is working.\n");
//         int err;
//         // if ((err = shutdown(quit_fd, SHUT_RDWR)) == 0) {
//         //     printf("shutdown client %d succeeded.\n", quit_fd);
//         // } else {
//         //     printf("shutdown client %d failed.\nthe errno is %d.\n", quit_fd, err);
//         // }
//         // fd_close(quit_fd);
//         join(quit_thread);
//         quit_done = true;
//     }
// }

/// Description:
///     refresh guests, will decrease guests if guest don't reply
/// Parameters:
///     void
/// Return:
///     void
inline void refresh(int fd_send)
{
    printf("refresh client %d.\n", fd_send);
    for (int i = 0; i < number_guests; i++)
    {
        if (guests[i].fd == fd_send)
        {
            // quit_thread = guests[i].thread;
            // quit_fd = guests[i].fd;
            // quit_done = false;
            detach(guests[i].thread);
            for (int j = i; j < number_guests - 1; j++)
            {
                guests[j] = guests[j + 1];
            }
            number_guests--;
            debug_info();
            break;
        }
    }
}

inline void free_resource(int fd, char *buffer, char *former_string, char *prompt)
{
    mutex_lock(fd);
    printf("errno: %d.\n", errno);
    free(buffer);
    free(former_string);
    free(prompt);
    refresh(fd);
    mutex_unlock(fd);
}

void debug_info()
{
    debug_info_number++;
    printf("---------------------------------\n");
    printf("DEBUG NUMBER %d:\n", debug_info_number);
    printf("number of visited guests:%d\n", connect_numbers);
    printf("\n");
    printf("now guests: %d\n", number_guests);
    printf("records:\n");
    printf("\t\tfd\tthread\n");
    for (int i = 0; i < number_guests; i++)
    {
        printf("%dth guest: \t%d\t%lu\n", i, guests[i].fd, guests[i].thread);
    }
    printf("---------------------------------\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind");
        return 1;
    }
    if (listen(fd, MAX_GUESTS))
    {
        perror("listen");
        return 1;
    }
    // pthread_t detect_quit_thread;
    // pthread_create(&detect_quit_thread, NULL, handle_quit, NULL);
    while (1)
    {
        if (number_guests >= MAX_GUESTS)
        {
            break;
        }
        int new_fd = accept(fd, NULL, NULL);
        mutex_lock(0);
        guests[number_guests].fd = new_fd;
        printf("client %d connect\n", guests[number_guests].fd);
        if (guests[number_guests].fd == -1)
        {
            perror("accept");
            return 1;
        }
        printf("client %d create thread\n", guests[number_guests].fd);
        if (pthread_create(&guests[number_guests].thread, NULL, handle_chat, &guests[number_guests].fd) < 0)
        {
            printf("error occurs when create pthread");
            return 0;
        }
        else
        {
            printf("create thread %lu.\n", guests[number_guests].thread);
            number_guests++;
        }
        connect_numbers++;
        debug_info();
        mutex_unlock(0);
    }
    return 0;
}
