#   Lab1实验报告

## Linux内核

`nproc`输出cpu计算单元数量为12

按照实验主页先按默认选项编译，文件大小为17M

(内核裁剪TBC)

## 初始内存盘

#### 如何在init进程中执行其他程序

按照"Operating System Concepts"中的``init``进程的`fork()`函数，并且调用`execl()`创建子进程并且依次执行1,2,3程序，`wait()`让父进程等待

##### 对于2，3

仿照实验主页中`/dev/null`字符设备的创建，以及`mknod()`的manual选择`S_IFCHR | S_IRUSR | S_IWUSR`mode，由已知，对`/dev/ttyS0`,`makedev(4. 64)`，对`/dev/fb0`，`makedev(29, 0)`

TBC

(how to avoid kernel panic? *now using while* bad)


## 初识Boot

TBC


## 思考题

TBC

#### reference

[exec another process in one process](https://stackoverflow.com/questions/5237482/how-do-i-execute-external-program-within-c-code-in-linux-with-arguments)

[kernel panic](https://www.redhat.com/sysadmin/linux-kernel-panic)