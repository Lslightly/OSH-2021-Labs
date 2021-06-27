#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string>

using namespace std;

#define MAX_GUESTS 32
#define MESSAGE_OFFSET 8
#define MAX_BYTES 1048600
#define BUF_SIZE 10

int fd[MAX_GUESTS];

int guests_number = 0;

struct Pipe {
    int fd_send;
    int fd_recv;
};

bool acceptor(int &server_fd, int &maxfd);

bool msg_recv(fd_set &fdSet, int index_fd, string &msgbuffer);

void msg_send(string &msgbuffer, int index_fd);

void which_client(fd_set &fdSet);

void close_all();

void guests_overflow(int new_fd);

void initialise_fdSet(fd_set &fdSet, int &server_fd);

bool split_enter(string &data, string &former_string);

void debug_info();

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(server_fd, MAX_GUESTS)) {
        perror("listen");
        return 1;
    }
    fd_set fdSet;
    int maxfd;
    guests_number = 0;
    maxfd = server_fd;
    
    int result;
    while (1) {
        initialise_fdSet(fdSet, server_fd);
        result = select(maxfd+1, &fdSet, NULL, NULL, NULL);
        if (result < 0) {
            printf("select error occurs.\n");
            return 0;
        } else if (result == 0) {
            printf("timeout.\n");
            return 0;
        } else {
            which_client(fdSet);
            if (FD_ISSET(server_fd, &fdSet)) {
                if (!acceptor(server_fd, maxfd)) {
                    printf("accept error.\n");
                }
            }
        }
    }

    close_all();
    return 0;
}

/// Description:
///     initialise and refresh fdSet
/// Parameters:
///     fdSet       the set of fd
///     server_fd   the server fd
/// Return:
///     void
void initialise_fdSet(fd_set &fdSet, int &server_fd) {
    FD_ZERO(&fdSet);
    FD_SET(server_fd, &fdSet);
    for (int i = 0; i < guests_number; i++) {
        if (fd[i]) {
            FD_SET(fd[i], &fdSet);
        }
    }
}

/// Description:
///     judge which client requests sending
///     recv the message
///     if the message means the client quits, then close it
///     else send it to other clients
/// Parameters:
///     fdSet
/// Return:
///     void
void which_client(fd_set &fdSet) {
    string msgbuffer;
    bool result;
    for (int i = 0; i < guests_number; i++) {
        if (FD_ISSET(fd[i], &fdSet)) {
            result = msg_recv(fdSet, i, msgbuffer);
            if (!result) {
                continue;
            } else {
                msg_send(msgbuffer, i);
            }
        }
    }
}

/// Description:
///     accept new clients
///     if guest_number > MAX_GUESTS, notify the client and close the fd
///     else accept it and change maxfd
/// Parameters:
///     server_fd
///     maxfd:  the max fd
/// Return:
///     false:  accept error
bool acceptor(int &server_fd, int &maxfd) {
    int new_fd = accept(server_fd, NULL, NULL);
    if (new_fd <= 0) {
        printf("accept:\n");
        return false;
    } else {
        if (guests_number < MAX_GUESTS) {
            for (int i = 0; i < MAX_GUESTS; i++) {
                if (!fd[i]) {
                    fd[i] = new_fd;
                    break;
                }
            }
            guests_number++;
            if (new_fd > maxfd) {
                maxfd = new_fd;
            }
            return true;
        } else {
            guests_overflow(new_fd);
            return true;
        }
    }
}

/// Description:
///     receive message from fd[index_fd]
///     if fd quits, close it and decrease the number of guests
/// Parameters:
///     fdSet
///     index_fd
/// Return:
///     false: fd quits
bool msg_recv(fd_set &fdSet, int index_fd, string &msgbuffer) {
    int len = 0;
    char buffer[BUF_SIZE];
    string readin;

    while (1) {
        len = recv(fd[index_fd], buffer, BUF_SIZE, 0);
        readin = buffer;
        if (len <= 0) {
            close(fd[index_fd]);
            FD_CLR(fd[index_fd], &fdSet);
            fd[index_fd] = 0;
            guests_number--;
            return false;
        } else {
            if (split_enter(readin, msgbuffer)) break;
        }
    }
    return true;
}

/// Description:
///     if data doesn't contain \'\\n\', then cat it after former_string
///     else copy former_string+data to msgbuffer
/// Parameters:
///     
/// Return:
///     true:   data contains \'\\n\'
///     false:  doesn't contain
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

/// Description:
///     send msgbuffer to other clients
/// Parameters:
///     msgbuffer:  the message
///     index_fd:   the sending client
/// Return:
///     void
void msg_send(string &msgbuffer, int index_fd) {
    string prompt = "Message:";
    prompt += msgbuffer;
    int msg_len = msgbuffer.length();
    for (int i = 0; i < MAX_GUESTS; i++) {
        if (i == index_fd) {
            continue;
        } else {
            if (fd[i]) {
                for (int j = 0; j < (msg_len + MESSAGE_OFFSET) / BUF_SIZE; j++) {
                    send(fd[i], &prompt[0]+j*BUF_SIZE, BUF_SIZE, 0);
                }
                send(fd[i], &prompt[0]+(msg_len + MESSAGE_OFFSET) / BUF_SIZE * BUF_SIZE, msg_len + MESSAGE_OFFSET - (msg_len + MESSAGE_OFFSET)/BUF_SIZE * BUF_SIZE, 0);
            } else { }
        }
    }
}

/// Description:
///     the new fd is the guest rejected, send message to it and close the fd
/// Parameters:
///     
/// Return:
///     
void guests_overflow(int new_fd) {
    char msg_overflow[40] = "Guests have overflowed. Please quit.\n";
    send(new_fd, msg_overflow, strlen(msg_overflow), 0);
    close(new_fd);
}

/// Description:
///     close all fds
/// Parameters:
///     
/// Return:
///     
void close_all() {
    for (int i = 0; i < guests_number; i++) {
        if (fd[i]) {
            close(fd[i]);
        }
    }
}
