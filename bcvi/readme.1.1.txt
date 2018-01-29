

           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
                  pwned by Team Shell Smash       
        
           ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Group Members:
  - Michael Zhang (zhan4854)
  - Elliott Beach (beach144)
  - Jegatheeswaran Jerusan (jerus005)

The macro makes use of a call to the compiler extension asm()
which bakes the given assembly code directly into the binary.
In this case, the only assembly that was compiled was a single
instruction `call()` that calls the text in the buffer as a
set of assembly instruction. This gives us very powerful control
over the execution of the program. In this exploit, we will focus
on attacking this macro feature of BCVI.

But in order to increase the scope of our attack outside of the
execution of this process, we will need to be able to execute
syscalls, which BCVI only weakly attempts to prevent by scanning
the assembly code provided for the opcodes for int 0x80 and
sysenter. But the `syscall` instruction is still available to us.

The next step is to call `execve()` in order to call the target
`/bin/rootshell` application. Here is `execve` from the man page:

       int execve(const char *filename, char *const argv[],
                  char *const envp[]);

The rest of the writeup is related to crafting shellcode that
is in the correct format for calling execve. Recall that the
function parameter order for 64-bit executables is: rdi for
the first argument, rsi for the second argument, and rdx for
the third argument. That means:

  - rdi should contain the address of a space in memory, pointing
    to the name of the executable we want to run, which, in this
    case, is /bin/rootshell.
  - rsi should contain the address of a char* array, which means
    that it's a list of pointers to char*s. In particular, argv
    should contain a list of the command-line arguments passed to
    the executable, which, in this case, is ["/bin/rootshell",
    NULL].
  - rcx should just contain NULL, since we won't need any envi-
    ronmental variables for this exploit.

The targeted layout for our stack will look like:

    $top:     ---------HIGH STACK ADDRESS---------
    
    $top-0x08: 0x00000000    <-- NULL to terminate the filename.

    $top-0x0f: 0x006c6c65
    $top-0x10: 0x6873746f
    $top-0x1f: 0x6f722f2f
    $top-0x20: 0x6e69622f    <-- This is the actual string con-
                                 taining the value of the path
                                 `/bin/rootshell`.

    $top-0x28: 0x00000000    <-- This is a NULL pointer that's
                                 used for envp[].

    $top-0x30: $top-0x20     <-- This is the address to the start
                                 of the `/bin/rootshell` string.

Then, we will simply set:

  - %rdi to point to $top-0x20
  - %rsi to point to $top-0x30
  - %rdx to point to $top-0x28

For convenience, %rdx is set before %rsi simply because of the
order in which the above stack layout is constructed. The rest
of the shellcode simply builds this structure and then puts 0x3b
(the syscall number for execve) and then executing the syscall.

                                      -Writeup by Michael Zhang