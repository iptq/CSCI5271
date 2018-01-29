

           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
                  pwned by Team Shell Smash       
        
           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Group Members:
  - Michael Zhang (zhan4854)
  - Elliott Beach (beach144)
  - Jegatheeswaran Jerusan (jerus005)

Like last week's exploit, I took advantage of the run_macro function, only this
week's patch had blacklisted 0x05, which was part of the opcode for syscall
(0x0f05). However, it doesn't seem like call was blocked, so I simply pushed the
opcodes required for syscall (0x0f05) on to the stack backwards. Unfortunately,
since 0x05 was blocked, I couldn't push it directly, so I had to push 0x0f06
(obviously in little-endian order), and then perform a subtraction in order to
have the correct value on stack. Then with a simple call, I was able to execve
the root shell again! 

                                      -Writeup by Michael Zhang
