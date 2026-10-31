#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <asm/ptrace.h>
#include <elf.h>

void read_registers(pid_t child)
{
    struct user_pt_regs regs;
    struct iovec iov;

    iov.iov_base = &regs;
    iov.iov_len = sizeof(regs);

    if (ptrace(PTRACE_GETREGSET,
               child,
               (void *)NT_PRSTATUS,
               &iov) == -1)
    {
        perror("PTRACE_GETREGSET");
        return;
    }

    printf("\n=====================================\n");
    printf("          CPU REGISTERS\n");
    printf("=====================================\n");

    printf("X0  : 0x%016llx\n", (unsigned long long)regs.regs[0]);
    printf("X1  : 0x%016llx\n", (unsigned long long)regs.regs[1]);
    printf("X2  : 0x%016llx\n", (unsigned long long)regs.regs[2]);
    printf("X3  : 0x%016llx\n", (unsigned long long)regs.regs[3]);
    printf("X4  : 0x%016llx\n", (unsigned long long)regs.regs[4]);
    printf("X5  : 0x%016llx\n", (unsigned long long)regs.regs[5]);
    printf("X6  : 0x%016llx\n", (unsigned long long)regs.regs[6]);
    printf("X7  : 0x%016llx\n", (unsigned long long)regs.regs[7]);
    printf("X8  : 0x%016llx\n", (unsigned long long)regs.regs[8]);

    printf("SP  : 0x%016llx\n", (unsigned long long)regs.sp);
    printf("PC  : 0x%016llx\n", (unsigned long long)regs.pc);

    printf("PSTATE : 0x%016llx\n",
           (unsigned long long)regs.pstate);
}
