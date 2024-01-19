
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
double getNum(char *buffer, const char *name)
{
    double total = 0;
    if (strncmp(buffer, name, strlen(name)) == 0)
    {
        char temp[1024] = {0};
        int j = 0;
        for (int i = 0; i < strlen(buffer); i++)
        {
            if (buffer[i] >= '0' && buffer[i] <= '9')
            {
                temp[j] = buffer[i];
                j++;
                // printf("%c",buffer[i]);
            }
        }
        total = atoi(temp);
    }
    return total;
}
int main()
{
    FILE *wFile;
    FILE *rFile;
    rFile = fopen("/proc/meminfo", "r");
    wFile = fopen("meminfo.txt", "w");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    double total = 1, free = 0, buffers = 0, cached = 0, slab = 0;
    while (1)
    {
        read = getline(&line, &len, rFile);
        if (read == -1)
            break;

        if (total == 1)
            total = getNum(line, "MemTotal");
        if (free == 0)
            free = getNum(line, "MemFree");
        if (buffers == 0)
            buffers = getNum(line, "Buffers");
        if (cached == 0)
            cached = getNum(line, "Cached");
        if (slab == 0)
            slab = getNum(line, "Slab");
    }
    double result = ((total - free - buffers - cached - slab) / total) * 100;
    char output[10] = {0};
    snprintf(output, 10, "%.2f", result);
    fputs(output, wFile);
    printf("%.2f\n", result);

    fclose(rFile);
    fclose(wFile);
    return 0;
}