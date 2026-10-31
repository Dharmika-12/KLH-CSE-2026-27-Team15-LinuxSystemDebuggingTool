#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <elf.h>
#include <string.h>
#include <unistd.h>

void read_process_memory(pid_t child)
{
    struct user_pt_regs regs;
    struct iovec reg_iov;

    reg_iov.iov_base = &regs;
    reg_iov.iov_len = sizeof(regs);

    if (ptrace(PTRACE_GETREGSET,
               child,
               (void *)NT_PRSTATUS,
               &reg_iov) == -1)
    {
        perror("[Memory] PTRACE_GETREGSET");
        return;
    }

    unsigned char buffer[64];

    struct iovec local_iov;
    struct iovec remote_iov;

    memset(buffer, 0, sizeof(buffer));

    local_iov.iov_base = buffer;
    local_iov.iov_len = sizeof(buffer);

    remote_iov.iov_base = (void *)regs.sp;
    remote_iov.iov_len = sizeof(buffer);

    ssize_t bytes_read = process_vm_readv(
        child,
        &local_iov,
        1,
        &remote_iov,
        1,
        0
    );

    printf("\n=====================================\n");
    printf("          MEMORY READER\n");
    printf("=====================================\n");

    printf("[Memory] Stack Address: 0x%016llx\n",
           (unsigned long long)regs.sp);

    if (bytes_read == -1)
    {
        perror("[Memory] process_vm_readv");
        return;
    }

    printf("[Memory] Bytes read: %zd\n", bytes_read);
    printf("[Memory] Data:\n");

    for (ssize_t i = 0; i < bytes_read; i++)
    {
        printf("%02x ", buffer[i]);

        if ((i + 1) % 16 == 0)
        {
            printf("\n");
        }
    }

    printf("\n");
}
