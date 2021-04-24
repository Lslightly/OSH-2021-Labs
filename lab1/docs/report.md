#   Lab1实验报告

## Linux内核

`nproc`输出cpu计算单元数量为12

按照实验主页先按默认选项编译，文件大小为17M

剪了网络，文件系统，各种驱动，不需要安全模块，不需要一些高级算法，多核支持等，到了6MiB以下(6MiB=6291456 Bytes, Image size=6203904 Bytes)

## 初始内存盘

#### 如何在init进程中执行其他程序

按照"Operating System Concepts"中的``init``进程的`fork()`函数，并且调用`execl()`创建子进程并且依次执行1,2,3程序，`wait()`让父进程等待，调用`exit(1)`退出子进程


##### 对于2，3

仿照实验主页中`/dev/null`字符设备的创建，以及`mknod()`的manual选择`S_IFCHR | S_IRUSR | S_IWUSR`mode，由已知，对`/dev/ttyS0`,`makedev(4. 64)`，对`/dev/ttyAMA0`相应创建，对`/dev/fb0`，`makedev(29, 0)`

> ~~...不知道讲义里的`ttyS0`与`ttyAMA0`的或关系是什么意思，`ttyS0`不行，也许是内核模块里没有支持`ttyS0`~~
> 据树莓派官网的UART配置介绍，默认情况下启动UART0，类型为PL011，对应于`/dev/ttyAMA0`，要使用`/dev/ttyS0`，即使用miniUART，需要进行相应的配置。默认状态下`enable_uart=1`，使用`UART0`

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
> ~~`make clean`和`make cleanall`挺好，然而我不清理~~

### 问题

-   1.`xor ax, ax`在将`ax`与其本身异或清零然后保存到ax寄存器中，好处是计算比数据移动要更快，更快的清零，在指令的大小和数量方面最快
-   2.`jmp $`中`$`表示取当前指令所在的地址，`jmp $`相当于不断将该指令的地址存入PC，形成死循环。只有当中断发生时，才会转去执行中断服务程序

> -   3.`[es:di]` es*16+di
> -   4.`boot.asm`文件前侧的`org 0x7c00`的用处是定义`boot`代码的绝对地址(比如在定义中断向量表的时候需要定义地址)
> -   5.`times 510 - ($-$$) db0`是在下面的代码中重复填入0共`510-(当前指令地址-当前部分起始地址)`次，原因是`boot.asm`最后的`boot sector mark`中是`boot signature`，BIOS在`boot sector`偏移510,511的地方找`0x55`和`0xAA`，通过`times`操作填充0可以方便在偏移510,511的位置写`boot signature`

-   1.`I am OK`只要在loader的`log_info GetMemStructOkMsg`后面调用宏`log_info IamOK, <字符串长度，不包括\0>, 4(第四行)`即可，当然要在`GetMemStructOkMsg`下面写`IamOK: db 'I am OK!'`
-   2.确定软盘可启动:通过`fat12_find_file_in_root_dir`来实现，如果找到了，就会进行后续的加载extry等操作，如果没有找到，就会失败，跳转到`not_found`。在`fat12_find_in_root_dir`中，主要通过对`Sector`和`entry`的循环实现顺序查找，通过比对文件名来实现
-   3.为什么`boot.asm`2.之后可以继续执行`loader.asm`？原因是`boot.asm`中有`jmp LoaderBase:LoaderOffset`指令，而`loader.asm`的代码开始地址就是`0x10000`，所以`jmp`会跳转到`LoaderBase*16+LoaderOffset`的位置，刚好就是`0x1000*16+0x0000 = 0x10000`
-  4.`boot.asm`与`loader.asm`隔开的原因:
    -   同一个文件不能存在两个`org`，这样只能用`times`写，还要计算多少个0，很麻烦
    -   如果将两个代码合在一起，在计算地址的时候就会很复杂，比如要修改`boot`部分的一点代码，可能就要涉及到很多的地址计算的修改。将两个代码在地址上分开方便维护，更新，修改。另外`boot`部分与`loader`部分对不同的硬件，不同的文件系统等，都要进行相应的修改，如果不分开，就很难维护，不如用`jmp`指令跳转来的方便
    -   还有是放在两个文件里，标签可以重叠不会有影响，因为两个分开编译，方便写程序
## 思考题

1.  在题目的范畴下,`Linux`应该主要指的是内核，它并不是一个完整的操作系统，而`Ubuntu`,`Debian`,`Fedora`,`ArchLinux`指Linux发行版。一个linux发行版除了包含linux内核外，通常还包含一系列GNU工具和库，一些附带软件，桌面系统等。不同的发行版用起来感觉可能完全不同，例如`Arch`系和`Debian`系的包管理器就有很大区别，在系统安装上也有很大差异，但是其内核是基本一致的(对不同发行版还是有细微差异)(参考了Linux101讲义)
2.  不需要，因为我们在`qemu`上模拟，不需要SD卡。指导意义:init程序需要的如tty输出模块要保留，其他的一些可选模块基本都可以删去
3.  树莓派启动阶段(BCM2837 based)
    1.  读取OTP(one-time programming)内存块决定哪种模式被启用
    2.  SoC上ROM中的`bootloader`执行，挂在SD卡`FAT32`分区
    3.  `bootloader`从SD卡上检查GPU固件，将固件写入GPU，随后启动GPU
    4.  GPU启动后检索config.txt, fixup.dat，根绝其内容设置CPU参数和内存分配，然后加载用户代码，启动CPU
    5.  然后CPU执行内核程序

#### some reference

[exec another process in one process](https://stackoverflow.com/questions/5237482/how-do-i-execute-external-program-within-c-code-in-linux-with-arguments)

[kernel panic](https://www.redhat.com/sysadmin/linux-kernel-panic)

[xor ax ax](https://stackoverflow.com/questions/4749585/what-is-the-meaning-of-xor-in-x86-assembly)

###### ~~检测TA是否为自动化给分程序，要是能够给我的报告提点issue就好了，大佬求带，qwq~~