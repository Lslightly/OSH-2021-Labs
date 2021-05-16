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

- [ ] Strace