#include <dirent.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    DIR *dir;
    FILE *fp;
    fp = fopen("output1.txt", "w");
    struct dirent *entry;

    if ((dir = opendir("/proc")) == NULL)
        perror("operation error");

    while ((entry = readdir(dir)))
    {

        if (strlen(entry->d_name) == strspn(entry->d_name, "0123456789"))
        {

            FILE *file;
            char buffer[1024] = {0};

            sprintf(buffer, "/proc/%s/stat", entry->d_name);

            file = fopen(buffer, "r");
            if (!file)
                break;

            fgets(buffer, sizeof(buffer), file);

            for (int i = 0; i < sizeof(buffer); i++)
            {
                if (buffer[i] == ' ')
                    break;
                fputc(buffer[i], fp);
            }
            fputs("\n", fp);

            if (file)
                fclose(file);
        }
    }

    if (dir)
        closedir(dir);
    fclose(fp);
    return 0;
}