#include <stdio.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>

void monitor_resources()
{
    struct rusage usage;
    struct sysinfo info;

    printf("\n=====================================\n");
    printf("         RESOURCE MONITOR\n");
    printf("=====================================\n");

    if (getrusage(RUSAGE_SELF, &usage) == -1)
    {
        perror("[Resource] getrusage");
        return;
    }

    printf("[Resource] CPU User Time   : %ld.%06ld seconds\n",
           usage.ru_utime.tv_sec,
           usage.ru_utime.tv_usec);

    printf("[Resource] CPU System Time : %ld.%06ld seconds\n",
           usage.ru_stime.tv_sec,
           usage.ru_stime.tv_usec);

    printf("[Resource] Maximum Memory  : %ld KB\n",
           usage.ru_maxrss);

    if (sysinfo(&info) == -1)
    {
        perror("[Resource] sysinfo");
        return;
    }

    printf("[Resource] Total RAM      : %lu MB\n",
           info.totalram / (1024 * 1024));

    printf("[Resource] Free RAM       : %lu MB\n",
           info.freeram / (1024 * 1024));

    printf("[Resource] Available RAM : %lu MB\n",
           (info.freeram * info.mem_unit) / (1024 * 1024));
}
