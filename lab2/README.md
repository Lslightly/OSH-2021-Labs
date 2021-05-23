# lab2

Rust Shell

支持功能
- [X] 管道
- [X] 文件重定向
- [X] Ctrl-C
- [X] Ctrl-D
- [X] echo
- [X] set local variables
- [X] pass temporary environmental variables to program
- [X] alias
- [ ] ...(more)

## 管道

主要使用`std::process::Stdio`实现

通过`Stdio`的`inherit()`,`Piped`以及`Command`的`stdin()`

## 重定向

-   `>` write
-   `>>` append
-   `<` as input

## 管道与重定向

可以实现`echo hello | cat < test.txt < test.md`多输入的情形

`lab2/test.md`与`lab2/test.txt`为测试文件

## Ctrl D

在读入的时候相当于读入空行。判断后执行和`exit`一样的行为

## Ctrl C

在输入时按ctrl c需要再按Enter键才能显示下一个prompt，在`sleep 5`时按下ctrl c马上退出子进程并prompt

## echo

简单的显示环境变量，本地变量

## A=1 env

能够临时带变量运行

## set

`set`显示当前所有本地变量

`unset`清空本地变量

## Strace

-   mmap
    -   将文件或者设备映射到内存中
    -   在`sys/mman.h`中声明
    -   `void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)`
    -   其中`addr`为新的虚拟地址空间映射的起始地址;`length`表示映射长度;`prot`为该映射的内存保护，可以是`PROT_NONE`,`PROT_EXEC`,`PROT_READ`,`PROT_WRITE`;`flag`表示对该映射的更新是否对其他映射相同区域的进程可见;`fd`和`offset`分别表示文件描述符以及相应的地址偏移，映射从`fd`所指文件的`offset`开始
-   fstat
    -   `sys/types.h`,`sys/stat.h`,`unistd.h`
    -   `int fstat(int fd, struct stat *statbuf)`
    -   返回文件信息，该文件信息被`statbuf`指向。与`stat()`类似，不过`stat`函数通过路径指向文件，而`fstat`函数通过文件描述符指向文件
    -   `statbuf`中`st_mode`表示文件类型和模式,`st_size`表示文件大小
-   rt_sigprocmask
    -   `signal.h`
    -   `int rt_sigprocmask(int how, const kernel_sigset_t *set, kernel_sigset_t *oldset, size_t sigsetsize)`
    -   获得或者改变调用线程的信号屏蔽(`signal mask`)，`signal mask`用来表示对于调用者来说正在被阻塞的信号
    -   `how`可以为`SIG_BLOCK`,`SIG_UNBLOCK`,`SIG_SETMASK`，分别表示将`set`加入现有的阻塞信号集;将`set`中的阻塞信号集从现有的阻塞信号集中移除;将现有的阻塞信号集设置为`set`。原来的`signal mask`值将存储在`oldset`中，如果`oldset`不是`NULL`的话。

> 很多地方采用直接unwrap()的方式，因此健壮性不是很强
