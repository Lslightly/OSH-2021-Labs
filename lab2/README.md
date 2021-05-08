# lab2

Rust Shell

支持功能
- [X] 管道
- [ ] 文件重定向
- [ ] Ctrl-C
- [X] Ctrl-D
- [ ] echo
- [ ] ...(more)

## 管道

主要使用`std::process::Stdio`实现

通过`Stdio`的`inherit()`,`Piped`以及`Command`的`stdin()`

- [ ] Strace