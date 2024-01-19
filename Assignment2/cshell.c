#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "cshell.h"
#include <time.h>

#define MAX_USER_INPUT_SIZE 1024
#define MAX_ENV_INPUT_SIZE 1024

typedef struct
{
    char *name;
    char *value;
    int size;
    int index;
} EnvVar;
typedef struct
{
    char *name;
    struct tm *time;
    char *value;
} Command;

int env_variable_index = 0;
int env_variable_size = 0;
int command_log_index = 0;
int commmand_log_size = 0;
int color_theme = 0;
char color_head[11] = "";
char color_tail[8] = "\033[0m";

void clean(char*str)
{
    char *temp=(char *) malloc (1024 * sizeof(char));
    //printf("Enter clean function\n");
    //printf("the length of str is %ld\n",strlen(str));
    int head_not_space=0;
    int j=0;
    int count_space=0;
    for(int i=0;i<strlen(str);i++){
        
        if((str[i]=='\r'&&str[i+1]=='\n')||str[i]=='\0')
        {
            temp[j-count_space]='\0';
            break;
        }
        else if(str[i]=='\n'||str[i]=='\0')
        {
            temp[j-count_space]='\0';
            break;
        }

        if(head_not_space == 1)
        {
            if(str[i]==' ')
                count_space++;
            else
                count_space=0;

            temp[j]=str[i];
            j++;
        
        }
        // delete start space until find a non-space
        if( head_not_space==0 )
        {    
            if(str[i]!=' ')
            {
                temp[j]=str[i];
                j++;
                head_not_space =1;
            }
        }

    }

   // printf("After clean the temp is:%s\n", temp);
    strcpy(str,temp);
    //printf("After clean the str is:%s\n", str);
}
int numOfArg(char *str)
{
    int count = 0;
    int flag = 0;
    for (int i = 0; i <= strlen(str); i++)
    {
        if (str[i] == ' ')
        {
            flag = 1;
        }
        if (flag == 1 && str[i] != ' ' && str[i] != '\n')
        {
            count++;
            flag = 0;
        }
    }
    return count;
}
void store_log_info(char *str, Command *comamand_info)
{
    time_t rawtime;
    time(&rawtime);
    comamand_info[command_log_index].time = localtime(&rawtime);
    // printf ( "store current local time and date: %s\n", asctime (comamand_info[command_log_index].time) );

    // for(int i = 0; i < strlen(str); i++)
    // {
    //     comamand_info[command_log_index].value[i]=str[i];
    // }
    comamand_info[command_log_index].value = (char *)malloc(strlen(str) * sizeof(char));

    strcpy(comamand_info[command_log_index].value, str);

    command_log_index++;
    commmand_log_size++;
}
void output_log_info(Command *comamand_info)
{
    for (int i = 0; i < commmand_log_size; i++)
    {
        printf("%s%s%s", color_head, asctime(comamand_info[i].time), color_tail);
        printf("%s%s%s\n", color_head, comamand_info[i].value, color_tail);
    }
}

void get_command_and_arg(char *str, char **command_arg)
{
    int numArg = numOfArg(str);

    int j = 0, next_arg = 0, arg_start = 0;

    char temp[256] = {0};
    for (int i = 0; i <=strlen(str); i++)
    {
        //printf("str[%d] is '%c' \n",i,str[i]);
        if ((str[i] == ' ' || str[i] == '\0') && arg_start)
        {
            //printf("temp is %s and the length is %ld\n",temp, strlen(temp));
            strcpy(command_arg[next_arg], temp);
            j = 0;
            next_arg++;
            arg_start = 0;
            memset(temp, 0, strlen(temp));
        }
        else if (str[i] != ' ' && str[i] != '\0')
        {
            arg_start = 1;
            temp[j] = str[i];
            //printf("temp[%d] is '%c' \n",j,temp[j]);
            j++;
        }
    }
    command_arg[numArg + 1] = NULL;
    // for(int i=0; i < numArg+1;i++)
    // {

    //     printf("arg[%d] is %s and the length is %ld\n",i, command_arg[i], strlen(command_arg[i]));

    // }

}

int is_env_variable(char *str)
{
    if(strlen(str) != 0 && str[0] == '$'){
        return 1;
    }
    return -1;
}
int is_print(char *command_arg, int numArg)
{
    if (strcmp(command_arg, "print") == 0)
    {
        if (numArg >= 1)
            return 1;
        else
        {
            printf("%sprint has no argument here%s\n", color_head, color_tail);
        }
    }
    return -1;
}
int is_log(char *str)
{
    if (strcmp(str, "log") == 0)
        return 1;
    else
        return 0;
}
int is_theme(char *command)
{
    if (strcmp(command, "theme") == 0)
    {
        return 1;
    }
    else
    {
        return -1;
    }
}
int is_exit(char *str)
{
    if (strcmp(str, "exit") == 0)
        return 1;
    else
        return 0;
}
int check_update_env(char *env_name, char *env_value, EnvVar *env_variable)
{
    for(int i=0; i < env_variable_size;i++)
    {
        if(strcmp(env_variable[i].name,env_name)==0)
        {
            strcpy(env_variable[i].value,env_value);
            return i;
        }
    }
    return -1;
}
int read_env_var(char *str, EnvVar *env_variable)
{
    //printf("enter read_env_var\n");
    int name_start = 1;
    int value_start = 0;
    char temp_name[MAX_ENV_INPUT_SIZE] = {0};
    char temp_value[MAX_ENV_INPUT_SIZE] = {0};
    int isStr = 0;
    int j = 0;
    for (int i = 0; i <= strlen(str); i++)
    {
        if (str[i] == '"' && value_start == 1)
        {
            isStr = 1;
        }
        if (str[i] == ' ' && isStr != 1)
        {
            return -1;
        }
        if (str[i] == '=')
        {
            name_start = 0;
            value_start = 1;
            j = 0;
            continue;
        }
        if (name_start)
        {
            temp_name[j] = str[i];
        }
        if (value_start)
        {
            temp_value[j] = str[i];
        }
        j++;
    }
    // if new envVar name is not in envVar[], then append.
    if(check_update_env(temp_name, temp_value,env_variable) == -1)
    {
        env_variable[env_variable_index].name = (char *)malloc(MAX_ENV_INPUT_SIZE * sizeof(char));
        env_variable[env_variable_index].value = (char *)malloc(MAX_ENV_INPUT_SIZE * sizeof(char));
        strcpy(env_variable[env_variable_index].name, temp_name);
        strcpy(env_variable[env_variable_index].value, temp_value);
        // printf("env_variable.name is %s\n", env_variable[env_variable_index].name);
        // printf("env_variable.value is %s\n", env_variable[env_variable_index].value);
        env_variable_index++;
        env_variable_size++;
    }
    return 1;
}
int index_env(char *command_arg, EnvVar *env_variable)
{
    for (int i = 0; i < env_variable_size; i++)
    {
        //printf("env_variable[%d].name is %s, and command_arg is %s\n", i, env_variable[i].name,command_arg);
        //printf("the length of env_variable is %ld and the length of command_arg is %ld",strlen(env_variable[i].name),strlen(command_arg));
        if (strcmp(env_variable[i].name, command_arg) == 0)
        {
            return i;
        }
    }
    return -1;
}
int printVar(char *str, EnvVar *env_variable, char **command_arg)
{
    if (numOfArg(str) == 1)
    {
        //printf("command_arg[1] is %s\n",command_arg[1]);
        int temp_index = index_env(command_arg[1], env_variable);
        //printf("find the env at env_variable[%d]\n",temp_index);

        if (temp_index != -1)
        {
            printf("%s%s%s\n", color_head, env_variable[temp_index].value, color_tail);
            return 1;
        }
        else
        {
            printf("%sVariable value is not defined yet%s\n", color_head, color_tail);
            return -1;
        }
    }
    else
    {
        printf("%sVariable value is expected%s\n", color_head, color_tail);
        return -1;
    }
}
void print_all(char *str)
{
    printf("%s", color_head);
    for (int i = 6; i <= strlen(str); i++)
        printf("%c", str[i]);
    printf("%s\n", color_tail);
}
int change_theme(char *str, char *command_arg)
{
    if (strcmp(command_arg, "red") == 0)
    {
        strcpy(color_head, "\033[0;31m");
        return 1;
    }
    else if (strcmp(command_arg, "blue") == 0)
    {
        strcpy(color_head, "\033[0;34m");
        return 1;
    }
    else if (strcmp(command_arg, "green") == 0)
    {
        strcpy(color_head, "\033[0;32m");
        return 1;
    }
    else if (strcmp(command_arg, "none") == 0)
    {
        strcpy(color_head, "");
        return 1;
    }
    return -1;
}

void  execute_command(char* str, EnvVar *env_variable,Command *log_info)
{
    //printf("str is %s\n", str);
    //$VAR=foo
    if (is_env_variable(str) == 1){
        //printf("enter is_env_variable\n");
        if (read_env_var(str, env_variable) == -1){
            printf("%sVariable value expected%s\n", color_head, color_tail);
            return;
        }
            store_log_info(str, log_info);
            return;
    }
    int numArg = numOfArg(str);
    char **command_arg = (char **)malloc((numArg + 2) * sizeof(char *));
    for (int i = 0; i < numArg + 2; i++)
        {
            command_arg[i] = (char *)malloc(256 * sizeof(char));
        }
            // command_arg[0] is command line; command_arg[i] is argi;
    get_command_and_arg(str, command_arg);
    // Build-in command of print 
    if (is_print(command_arg[0], numArg) == 1){
        if (command_arg[1][0] == '$'){
            if (printVar(str, env_variable, command_arg) == 1){
                store_log_info(str, log_info);
            }
        }
        else
        {
            print_all(str);
            store_log_info(str, log_info);
        }
        return;
    }
    // Build-in command of log
    if (is_log(str) == 1){
        output_log_info(log_info);
        store_log_info(str, log_info);
        return;
    }
    //Build-in command of theme color
    if (is_theme(command_arg[0]) == 1){
        if(numArg == 0)
        {
            printf("%sPlease input color name%s\n", color_head, color_tail);
            return;
        }
        if (change_theme(command_arg[0], command_arg[1]) == 1){
            store_log_info(str, log_info);
        }
        else
        {
            printf("%sUnsupported theme: %s%s\n", color_head, command_arg[1], color_tail);
        }
        return;
    }
    int fds[2];
    pid_t pid;
    char buff[1024];
    if (pipe(fds) == -1)
    {
        perror("Error: ");
        exit(1);
    }
    memset(buff, 0, 1024);

    pid = fork();
    if (pid < 0)
    {
        printf("Fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        // child

        // printf("I am child (pid %d)\n", (int)getpid());
        char commandStr[2048];
        sprintf(commandStr, "/bin/%s", command_arg[0]);
        if (execv(commandStr, command_arg) == -1)
        {
            close(fds[0]);
            write(fds[1], "Missing keyword or command, or permission problem\n", 50);
            close(fds[1]);
            // printf("Child process Exiting\n");
            exit(EXIT_SUCCESS);
            // printf("Missing keyword or command, or permission problem");
        }
    }
    else if (pid > 0)
    {
        // parent process
        wait(NULL);
        // printf("I am parent (pid %d)\n", (int)getpid());
        close(fds[1]);

        int count = 0;
        while (read(fds[0], buff, 1) == 1)
        {
            printf("%s", buff);
            count++;
        }
        if (count == 0)
        {
            store_log_info(str, log_info);
        }

        close(fds[0]);
    }
}
void color_print(char *print_message)
{
    printf("%s%s%s\n",color_head, print_message, color_tail);
}
int main(int argc, char *argv[])
{
    // for(int i = 0; i<argc; i++)
    // {
    //     printf("argument[%d] is %s\n", i, argv[i]);
    // }
    EnvVar *env_variable = (EnvVar *)malloc(MAX_ENV_INPUT_SIZE * sizeof(EnvVar));
    Command *log_info = (Command *)malloc(MAX_ENV_INPUT_SIZE * sizeof(Command));
    char *str=(char *) malloc (1024 * sizeof(char));
    //script mode:
    if(argv[1]!=NULL)
    { 
        FILE * fp;
        size_t len = 0;
        ssize_t readFile;
        fp = fopen("script.txt", "r");
        if (fp == NULL){
            exit(EXIT_FAILURE);
        }
        while((readFile = getline(&str, &len, fp)) != -1){
            clean(str);
            execute_command(str,env_variable,log_info);
        }
    
        color_print("Bye !");
        fclose(fp);
        if (str)
            free(str);
        exit(EXIT_SUCCESS);
    }
    else
    {
        while (1)
        {
            printf("%scshell$ %s", color_head, color_tail);
            fgets(str, 1024, stdin);
            clean(str);
            if (is_exit(str) == 1)
            {
                color_print("Bye !");
                exit(0);
            }
            execute_command(str, env_variable, log_info);
        }
    }
}