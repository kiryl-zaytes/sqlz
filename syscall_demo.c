#include "sys/syscall.h"
#include "stdio.h"

int main(void)
{
    printf("%d", SYS_write);

    register int syscall_no asm("rax") = 1;
    register int arg1 asm("rdi") = 1;
    register char *arg2 asm("rsi") = "hello, world!\n";
    register int arg3 asm("rdx") = 14;
    asm("syscall");

    return 0;
}