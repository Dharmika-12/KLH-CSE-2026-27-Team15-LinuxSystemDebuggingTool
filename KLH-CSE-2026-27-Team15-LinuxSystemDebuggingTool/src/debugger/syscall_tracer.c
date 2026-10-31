#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <asm/ptrace.h>
#include <elf.h>
#include <errno.h>


void read_registers(pid_t child);
void read_process_memory(pid_t child);

void trace_system_calls(pid_t child)
{
    int status;
    int entering = 1;

    printf("\n=====================================\n");
    printf("        SYSTEM CALL TRACER\n");
    printf("=====================================\n");

    if (ptrace(PTRACE_SYSCALL, child, NULL, NULL) == -1)
    {
        perror("PTRACE_SYSCALL");
        return;
    }

    while (1)
    {
        if (waitpid(child, &status, 0) == -1)
        {
            perror("waitpid");
            break;
        }

        if (WIFEXITED(status))
        {
            printf("[Tracer] Process exited with status: %d\n",
                   WEXITSTATUS(status));
            break;
        }

        if (WIFSIGNALED(status))
        {
            printf("[Tracer] Process terminated by signal: %d\n",
                   WTERMSIG(status));
            break;
        }

        if (WIFSTOPPED(status))
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
                break;
            }

            if (entering)
{
    printf("[Syscall Entry] Number: %lld\n",
           (long long)regs.regs[8]);

    if (entering == 1)
    {
        read_registers(child);
        read_process_memory(child);
    }
}

            entering = !entering;

            if (ptrace(PTRACE_SYSCALL, child, NULL, NULL) == -1)
            {
                perror("PTRACE_SYSCALL");
                break;
            }
        }
    }
}
