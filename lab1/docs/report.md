#   Lab1实验报告

## Linux内核

`nproc`输出cpu计算单元数量为12

按照实验主页先按默认选项编译，文件大小为17M

剪了网络，文件系统，各种驱动，不需要安全模块，不需要一些高级算法，多核支持，到了6M

## 初始内存盘

#### 如何在init进程中执行其他程序

按照"Operating System Concepts"中的``init``进程的`fork()`函数，并且调用`execl()`创建子进程并且依次执行1,2,3程序，`wait()`让父进程等待，调用`exit(1)`退出子进程


##### 对于2，3

仿照实验主页中`/dev/null`字符设备的创建，以及`mknod()`的manual选择`S_IFCHR | S_IRUSR | S_IWUSR`mode，由已知，对`/dev/ttyS0`,`makedev(4. 64)`，对`/dev/ttyAMA0`相应创建，对`/dev/fb0`，`makedev(29, 0)`

> ...不知道讲义里的`ttyS0`与`ttyAMA0`的或关系，`ttyS0`不行，也许是内核模块里没有支持`ttyS0`

使用循环来防止`kernel panic`


## 初识Boot

-   Makefile
    -   $@: 目标文件
    -   $^: 所有依赖文件
    -   $<: 第一个依赖文件

### Makefile步骤

1.  `make boot.bin`
2.  `make loader.bin`
3.  `make kernel.bin`
4.  `make bootloader.img`
5.  `make qemu`

### 问题

-   1.`xor ax, ax`在将`ax`与其本身异或清零然后保存到ax寄存器中，好处是计算比数据移动要更快，更快的清零，在指令的大小和数量方面最快
-   2.`jmp $`中`$`表示取当前指令所在的地址，`jmp $`相当于不断将该指令的地址存入PC，形成死循环。只有当中断发生时，才会转去执行中断服务程序

> -   3.`[es:di]` es*16+di
> -   4.`boot.asm`文件前侧的`org 0x7c00`的用处是定义`boot`代码的绝对地址(比如在定义中断向量表的时候需要定义地址)
> -   5.`times 510 - ($-$$) db0`是在下面的代码中重复填入0共`510-(当前指令地址-当前部分起始地址)`次，原因是`boot.asm`最后的`boot sector mark`中是`boot signature`，BIOS在`boot sector`偏移510,511的地方找`0x55`和`0xAA`，通过`times`操作填充0可以方便在偏移510,511的位置写`boot signature`

-   1.`I am OK`只要在loader的`log_info GetMemStructOkMsg`后面调用宏`log_info IamOK, <字符串长度，不包括\0>, 4(第四行)`即可，当然要在`GetMemStructOkMsg`下面写`IamOK: db 'I am OK!'`

## 思考题

TBC

#### reference

[exec another process in one process](https://stackoverflow.com/questions/5237482/how-do-i-execute-external-program-within-c-code-in-linux-with-arguments)

[kernel panic](https://www.redhat.com/sysadmin/linux-kernel-panic)

[xor ax ax](https://stackoverflow.com/questions/4749585/what-is-the-meaning-of-xor-in-x86-assembly)

[org]()