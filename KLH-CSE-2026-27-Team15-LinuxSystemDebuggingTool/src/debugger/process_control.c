
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <signal.h>

#define DEFAULT_PROGRAM "./target_program"

static const char *get_target_program(void)
{
    const char *requested = getenv("TARGET_PROGRAM");

    if (requested == NULL || requested[0] == '\0')
    {
        return DEFAULT_PROGRAM;
    }

    /* Only allow these known programs. */
    if (strcmp(requested, "./target_program") == 0 ||
        strcmp(requested, "/bin/ls") == 0 ||
        strcmp(requested, "/usr/bin/whoami") == 0)
    {
        return requested;
    }

    fprintf(stderr,
            "[Process] Unsupported target program; using default.\n");

    return DEFAULT_PROGRAM;
}

void print_target_process(void)
{
    const char *program = get_target_program();
    const char *name = strrchr(program, '/');

    name = (name != NULL) ? name + 1 : program;

    printf("[Target Process] Name    : %s\n", name);
    printf("[Target Process] Program : %s\n", program);
}

pid_t start_target_process(void)
{
    const char *program = get_target_program();

    print_target_process();

    pid_t child = fork();

    if (child == -1)
    {
        perror("[Process] fork failed");
        return -1;
    }

    if (child == 0)
    {
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1)
        {
            perror("[Process] PTRACE_TRACEME failed");
            _exit(1);
        }

        raise(SIGSTOP);

        execl(program, program, (char *)NULL);

        perror("[Process] execl failed");
        _exit(1);
    }

    return child;
}

int wait_for_initial_stop(pid_t child)
{
    int status;

    printf("[Debugger] Child PID: %d\n", child);
    printf("[Debugger] Waiting for child...\n");

    if (waitpid(child, &status, 0) == -1)
    {
        perror("[Debugger] waitpid failed");
        return -1;
    }

    if (WIFSTOPPED(status))
    {
        printf("[Debugger] Child stopped by signal: %d\n",
               WSTOPSIG(status));

        return 0;
    }

    printf("[Debugger] Child did not stop correctly.\n");

    return -1;
}