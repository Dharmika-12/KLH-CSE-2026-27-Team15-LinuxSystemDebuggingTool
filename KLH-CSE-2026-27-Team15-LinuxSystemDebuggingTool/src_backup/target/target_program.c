#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    printf("Target Program Started\n");

    int numbers[5] = {10, 20, 30, 40, 50};

    printf("Processing data...\n");

    for (int i = 0; i < 5; i++)
    {
        printf("Value: %d\n", numbers[i]);
        
    }

    printf("Target Program Completed\n");

    return 0;
}
