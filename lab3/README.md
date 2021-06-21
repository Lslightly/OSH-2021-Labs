# lab3

## 双人聊天室

对于recv到的数据，以换行为切分，进行多次recv

发送数据也按固定长度多次发送，避免阻塞

## 多线程多人聊天室

多线程编程遇到的困难(已解决):

#### 多线程bug描述:

在客户端退出之后，需要重新连接两次或多次才能重新连上

bug产生的代码

```c
void refresh(int fd_send) {
    printf("refresh client %d.\n", fd_send);
    for (int i = 0; i < number_guests; i++) {
        if (guests[i].fd == fd_send) {
            //  ...
            number_guests--;
            //  ...
            break;
        }
    }
}

main() {
    // ...
    guest[number_guests].fd = accept(fd, NULL, NULL)
    // ...
}
```

#### bug发生的原因

在客户端退出时，会调用`refresh(fd)`函数进行释放线程`malloc`的资源，`close(fd)`，同时会将当前的顾客数减1，问题就出在减1这里

由于是线程之间是并发的，服务端线程(即`main`函数)与创建的`pthread`会并发执行

在`pthread`中调用`refresh`减少`number_guests`的时候，`main`函数中已经在阻塞等待accept的结果，而此时accept的结果将要赋值给的guest，是number_guests还没有减少的那一个guest，而不是我们所期望的在释放掉`pthread`之后number_guests-1的那个guest，这样在后面又create线程的时候，使用的又是减1之后的number_guests，相当于accept赋值语句和后面的语句间不一致。

为了保证accept赋值和后面的语句中number_guests的一致性，让减少number_guests的操作成为临界区操作，只有在`pthread`中获取mutex,把number_guests真正减少了之后，释放mutex，然后`main`函数接受到mutex才进行新客户端的连接，这样可以保证number_guests不会被篡改

#### 修改之后的代码

```c
main() {
        //  ...
        int new_fd = accept(fd, NULL, NULL);
        mutex_lock(0);
        guests[number_guests].fd = new_fd;
        //  ...
}

free_resource() {
    mutex_lock(fd);
    printf("errno: %d.\n", errno);
    free(buffer);
    free(former_string);
    free(prompt);
    refresh(fd);
    mutex_unlock(fd);
}
```

## 异步I/O聊天室

TCB

## async/await

TCB
