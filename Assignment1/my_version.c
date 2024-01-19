#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    FILE *fp1;
    FILE *fp2;
    FILE *wFile;
    fp1 = fopen("/etc/os-release", "r");
    fp2 = fopen("/proc/version", "r");
    wFile = fopen("output3.txt", "w");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while (1)
    {
        read = getline(&line, &len, fp1);
        if (read == -1)
            break;

        fputs(line, wFile);
    }
    while (1)
    {
        read = getline(&line, &len, fp2);
        if (read == -1)
            break;

        fputs(line, wFile);
    }
    fclose(fp1);
    fclose(fp2);
    fclose(wFile);

    return 0;
}