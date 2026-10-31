#include <stdio.h>
#include <sys/types.h>

pid_t start_target_process();
int wait_for_initial_stop(pid_t child);
void trace_system_calls(pid_t child);
void monitor_resources();

int main()
{
    printf("=====================================\n");
    printf("     LINUX SYSTEM DEBUGGING TOOL\n");
    printf("=====================================\n");

    pid_t child = start_target_process();

    if (child == -1)
    {
        return 1;
    }

    if (wait_for_initial_stop(child) == -1)
    {
        return 1;
    }

    trace_system_calls(child);

    monitor_resources();

    printf("\n[Debugger] Debugging completed.\n");

    return 0;
}
