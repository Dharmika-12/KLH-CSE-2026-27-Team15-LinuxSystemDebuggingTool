
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *worker(void *arg)
{
    int id = *(int *)arg;

    printf("[Thread %d] Started\n", id);

    for (int i = 1; i <= 3; i++)
    {
        printf("[Thread %d] Working: step %d\n", id, i);
        sleep(15);
    }

    printf("[Thread %d] Finished\n", id);

    return NULL;
}

int main(void)
{
    pthread_t threads[3];
    int ids[3] = {1, 2, 3};

    printf("=== POSIX THREAD DEMONSTRATION ===\n");
    printf("Main process PID: %d\n", getpid());

    for (int i = 0; i < 3; i++)
    {
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0)
        {
            fprintf(stderr, "Failed to create thread %d\n", i + 1);
            return 1;
        }
    }

    for (int i = 0; i < 3; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("All three threads completed successfully.\n");

    return 0;
}
