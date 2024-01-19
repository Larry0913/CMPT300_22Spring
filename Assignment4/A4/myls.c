
// all includes here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <time.h>
#include <ctype.h>

// global variables here
static int option_i;
static int option_R;
static int option_l;
static char **dir_list;
static char **file_list;
int is_file=0;
int count_file_num(int argc, char **argv)
{
    int count_file_num = 0;
    DIR *pdir;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            pdir = opendir(argv[i]);
            if (pdir == NULL)
            {
                count_file_num++;
            }
        }
    }
	closedir(pdir);
    return count_file_num;
}
int count_dir_num(int argc, char **argv)
{
    int count_dir_num = 0;
    DIR *pdir;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            pdir = opendir(argv[i]);
            if (pdir != NULL)
            {
                count_dir_num++;
            }
        }
    }
	closedir(pdir);
    return count_dir_num;
}
void get_options(int argc, char **argv)
{
    int file_num = count_file_num(argc, argv);
    int dir_num = count_dir_num(argc, argv);
    file_list = (char **)malloc(file_num * sizeof(char *));
    dir_list = (char **)malloc(dir_num * sizeof(char *));
    int file_index = 0;
    int dir_index = 0;
    for (int i = 1; i < argc; i++)
    {
        // printf("getOptions(), %s\n", argv[i]);
        if (argv[i][0] == '-')
        {
            for (int j = 1; j < strlen(argv[i]); j++)
            {
                char option = argv[i][j];
                switch (option)
                {
                case 'i':
                    option_i = 1;
                    break;
                case 'l':
                    option_l = 1;
                    break;
                case 'R':
                    option_R = 1;
                    break;
                default:
                    fprintf(stderr, "ls: invalid option -- '%c'\n", option);
                    fprintf(stderr, "Usage:\n./myls [options] [file list]\n");
                    fprintf(stderr, "Example:\n./myls -iRl \n");
                    exit(1);
                }
            }
        }
        else
        {
            // pdir = opendir(argv[i]);
			DIR *pdir;
            pdir = opendir(argv[i]);
            if (pdir == NULL)
            {
                // is file
                file_list[file_index] = (char *)malloc(strlen(argv[i]) * sizeof(char));
                strcpy(file_list[file_index], argv[i]);
                file_index++;
            }
            else
            {
                // is dir
                dir_list[dir_index] = (char *)malloc(strlen(argv[i]) * sizeof(char));
                strcpy(dir_list[dir_index], argv[i]);
                dir_index++;
            }
			closedir(pdir);
        }
    }
}

char *rebuild_date(char *date)
{
    char *new_date = malloc(18);

    memcpy(&new_date[0], &date[4], 3);
    new_date[3] = ' ';

    memcpy(&new_date[4], &date[8], 2);
    new_date[6] = ' ';

    memcpy(&new_date[7], &date[20], 4);
    new_date[11] = ' ';
	
    memcpy(&new_date[12], &date[11], 5);
    new_date[17] = '\0';
    return new_date;
}

int get_int_length(long int ino_num) {
    int len = 0;
    long int ino = ino_num;
    while (ino != 0) {
        ino /= 10;
        ++len;
    }
    return len;
}

void print_dir_file(struct dirent *pdirent, char *fullDir)
{
    struct stat dir_stat;
    int max_size_array[4] = {0};

    if (option_i == 1)
    {
        printf("%ld ", pdirent->d_ino);
    }
    if (option_l == 1)
    {
        int st = stat(fullDir, &dir_stat);
        if (st != 0)
        {
            perror("stat error: ");
            return;
        }
        if (pdirent->d_type == DT_LNK)
        {
            printf("l");
        }
        else if (S_ISDIR(dir_stat.st_mode))
        {
            printf("d");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IRUSR)
        {
            printf("r");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IWUSR)
        {
            printf("w");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IXUSR)
        {
            printf("x");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IRGRP)
        {
            printf("r");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IWGRP)
        {
            printf("w");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IXGRP)
        {
            printf("x");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IROTH)
        {
            printf("r");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IWOTH)
        {
            printf("w");
        }
        else
        {
            printf("-");
        }
        if (dir_stat.st_mode & S_IXOTH)
        {
            printf("x");
        }
        else
        {
            printf("-");
        }
        printf(" ");

        // Print the number of hard links
        char hard_link_str[40] = "%";
        char hard_link_num[40];
        sprintf(hard_link_num, "%i", max_size_array[0]);
        strcat(hard_link_str, hard_link_num);
        strcat(hard_link_str, "ld ");
        printf(hard_link_str, dir_stat.st_nlink);

        // Print user-name of file owner
        struct passwd *pw = getpwuid(dir_stat.st_uid);
        char username_str[40] = "%";
        char username_num[40];
        sprintf(username_num, "%i", max_size_array[1]);
        strcat(username_str, username_num);
        strcat(username_str, "s ");
        printf(username_str, pw->pw_name);

        // Print group name of the file owner
        pw = getpwuid(dir_stat.st_gid);
        char groupname_str[40] = "%";
        char groupname_num[40];
        sprintf(groupname_num, "%i", max_size_array[2]);
        strcat(groupname_str, groupname_num);
        strcat(groupname_str, "s ");
        printf(groupname_str, pw->pw_name);

        // Print the size of the file (bytes)
        char size_byte[40] = "%";
        char size_byte_num[40];
        sprintf(size_byte_num, "%i", max_size_array[3]);
        strcat(size_byte, size_byte_num);
        strcat(size_byte, "li ");
        printf(size_byte, dir_stat.st_size);

        // Print the date & time file was last modified
        time_t rawtime = dir_stat.st_mtime;
        struct tm *timeinfo;
        timeinfo = localtime(&rawtime);
        char *str = asctime(timeinfo);
        str[strlen(str) - 1] = 0;
        char *new_date = rebuild_date(str);
        printf("%s ", new_date);

        free(new_date);
    }

	if(is_file == 0)
    	printf("%s\n", pdirent->d_name);
	else
		printf("%s\n", fullDir);
}

void lexi_sort_str(char *table[], int file_or_dir_num)
{
    char *lowest = " ";
    int lowest_index;
    for (int i = 0; i < file_or_dir_num; i++)
    {
        lowest = table[i];
        lowest_index = i;
        for (int j = i; j < file_or_dir_num; j++)
        {
            if (strcmp(table[j], lowest) < 0)
            {
                lowest = table[j];
                lowest_index = j;
            }
        }
        table[lowest_index] = table[i];
        table[i] = lowest;
    }

    return;
}

void lexi_sort_dir(struct dirent **dir_queue, int dir_num)
{
    char *lowestP;
    int lowestIndex;
    for (int i = 0; i < dir_num; i++)
    {
        lowestP = dir_queue[i]->d_name;
        lowestIndex = i;

        for (int j = i; j < dir_num; j++)
        {

            if (strcmp(dir_queue[j]->d_name, lowestP) < 0)
            {

                lowestP = dir_queue[j]->d_name;
                lowestIndex = j;
            }
        }

        struct dirent *lowestEntity = dir_queue[lowestIndex];

        dir_queue[lowestIndex] = dir_queue[i];
        dir_queue[i] = lowestEntity;
    }
}

void read_dir(char *str)
{
	int dir_file_num_max = 1024;
    struct dirent **dir_file_queue = (struct dirent **)malloc(dir_file_num_max * sizeof(struct dirent *));
    struct dirent *pdirent;

    DIR *pdir;
	//printf("in read_dir, going to open %s\n",str);
    pdir = opendir(str);
    // dir_list i is whther file or invalid
    int dir_num_count = 0;
    while ((pdirent = readdir(pdir)) != NULL)
    {
		if (dir_num_count == dir_file_num_max)
		{
			dir_file_num_max = dir_file_num_max * 2;
			dir_file_queue = realloc(dir_file_queue, dir_file_num_max * sizeof(struct dirent *));
		}
		
        if (pdirent->d_name[0] != '.')
        {
            //dir_file_queue[dir_num_count] = (struct dirent *)malloc(sizeof(struct dirent));
            dir_file_queue[dir_num_count] = pdirent;
            // printf("%s\n",dir_file_queue[num_count]->d_name);
            dir_num_count++;
        }
    }
	closedir(pdir);
	
    // printf("sort list\n");
    lexi_sort_dir(dir_file_queue, dir_num_count);
    // for(int i = 0; i < dir_num_count; i++)
    // {
    //     printf("%s\n")
    // }
    printf("%s:\n", str);

    DIR *pdir2;
    char dirqueue[1024][1024];

    int dirqueueCount = 0;

    for (int i = 0; i < dir_num_count; i++)
    {
        char full_dir_path[256] = "";
        strcpy(full_dir_path, str);
        strcat(full_dir_path, "/");
        strcat(full_dir_path, dir_file_queue[i]->d_name);
        print_dir_file(dir_file_queue[i], full_dir_path);

        if (option_R)
        {
            pdir2 = opendir(full_dir_path);
            if (pdir2 != NULL)
            {

                strcpy(dirqueue[dirqueueCount], full_dir_path);
                dirqueueCount++;
            }
        }
        closedir(pdir2);
    }
    if (option_R)
    {
        for (int i = 0; i < dirqueueCount; i++)
        {
            // printf("\n%s: \n", dirqueue[i]);
            printf("\n");
            read_dir(dirqueue[i]);
        }
    }
	free(dir_file_queue);
}

char *get_dir_path(char *str)
{
    int pathLen = strlen(str);
	int count_store_num=0;
    for (int i = 0; i < pathLen; i++)
    {
       if (str[i] == '/')
		{
			count_store_num++;
		}
    }
    if (count_store_num == 0)
        return ".";
    else
		{
		char *target = (char *)malloc(pathLen * sizeof(char));
		int temp_count=0;
		int count_valid = 0;
		for(int i = 0 ; i < pathLen ; i++)
		{
			if(str[i] == '/')
			{
				count_valid++;
			}
			if(str[i] == '/' && count_valid ==count_store_num)
				break;
			target[i]=str[i];
			temp_count++;
		}
		target[temp_count] = '\0';
		//printf("dir target is %s\n",target);
		return target;
	}
}
char *get_file_path(char *str)
{
	int pathLen = strlen(str);
	int count_store_num=0;
    for (int i = 0; i < pathLen; i++)
    {
       if (str[i] == '/')
		{
			count_store_num++;
		}
    }
    if (count_store_num == 0)
        return str;
    else
		{
		char *target = (char *)malloc(pathLen * sizeof(char));
		int temp_count=0;
		int count_valid = 0;
		int flag=0;
		for(int i = 0 ; i < pathLen ; i++)
		{
			if(flag == 1)
			{
				target[temp_count]=str[i];
				temp_count++;
			}
			if(str[i] == '/')
			{
				count_valid++;
			}
			if(str[i] == '/' && count_valid ==count_store_num)
				flag=1;
		}
		target[temp_count] = '\0';
		//printf("file target is %s\n",target);
		return target;
	}
	
}

void readFile(char *str)
{
	
    if (access(str, F_OK) == -1)
    {
        // Error Code Stuff
        printf("Error  : Nonexistent files or directories %s\n", str);
        return;
    }
    else
    {
		struct dirent *pdirent;
    	DIR *pdir;
        char *dir_path = get_dir_path(str);
        char *file_name =get_file_path(str);
        //printf("%s %s\n", dir_path, file_name);
		//printf("%ld %ld\n", strlen(dir_path),strlen(file_name));
		//printf("1111111\n");
        pdir = opendir(dir_path);
		//printf("1222221\n");
        while ((pdirent = readdir(pdir)) != NULL)
        {
			//printf("%s,1333331\n", pdirent->d_name);
			//printf("Enter readFile while loop\n");
            if (strncmp(pdirent->d_name, file_name, strlen(file_name)) == 0)
            {
				is_file=1;
                print_dir_file(pdirent, str);
				is_file=0;
            }
        }
		closedir(pdir);
    }
}
// given two strings, lowerString, and string. rewrite lowerString to match string, except with all alphabetical letters in lowercase

int main(int argc, char *argv[])
{

    get_options(argc, argv);

    int file_num = count_file_num(argc, argv);
    int dir_num = count_dir_num(argc, argv);
    int dir_file_num = file_num + dir_num;
    // printf("%d + %d = %d\n", file_num, dir_num, dir_file_num);
    // for (int i = 0; i < file_num; i++) {
    //     printf("file_list is %s\n", file_list[i]);
    // }
    lexi_sort_str(file_list, file_num);
    // for (int i = 0; i < file_num; i++) {
    //     printf("%s\n", file_list[i]);
    // }
    lexi_sort_str(dir_list, dir_num);
    // if no file or dir, then it is dir itself, print everything inside this dir
    if (dir_file_num == 0)
    {
        read_dir(".");
    }
    else
    {
        for (int i = 0; i < file_num; i++)
        {
            readFile(file_list[i]);
        }
        printf("\n");

        for (int i = 0; i < dir_num; i++)
        {
            read_dir(dir_list[i]);
            if (i != dir_num - 1) {
                printf("\n");
            }
        }
    }
	free(dir_list);
	free(file_list);
    // for(int i = 0; i < file_num; i++)
    // {

    //     printf("dir_list[%d] :%s\n", i, dir_list[i]);
    // }

	
}
