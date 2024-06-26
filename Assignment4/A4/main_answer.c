
   
//// This project is created to fulfill CMPT300 Assignment 4 requirements
//// It consists of a simplified version of the 'ls' cmd that we often take for granted in UNIX
////
//// The program only supports -iRl flags, with some modifications, see the assignment instruction sheet for details
////
////
/* We want POSIX.1-2008 + XSI, i.e. SuSv4, features */
#define _XOPEN_SOURCE 700

/* If the C library can support 64-bit file sizes
   and offsets, using the standard names,
   these defines tell the C library to do so. */
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

/* "The feature test macro _GNU_SOURCE must be defined in order
 * to obtain the definition of FTW_ACTIONRETVAL from <ftw.h>."
 * (using FTW_SKIP_SUBTREE)*/
#define _GNU_SOURCE

#include <ftw.h>        // encapsulates file-tree waklk functions such as opendir(), readdir(), struct dirent, etc.
// https://stackoverflow.com/a/29402705
// as the answerer points out, this has the advantage of handling scenario when files/dir are
// move mid-traversal
#include <stdio.h>      // fprint()
#include <unistd.h>     //getopt()
#include <string.h>     // strcmp()
#include <dirent.h>     //opendir(), readdir(), struct dirent, etc.
#include <sys/types.h>  // ino_t
#include <errno.h>      // errno
#include <time.h>       // localtime_r()
#include <stdlib.h>     // malloc() free()
#include <pwd.h>        // getpwuid()
#include <grp.h>        // getgrgid()
#include <libgen.h>     // basename()

// DEBUG macro is used to turn on various debugging features
// Disable at the release version
//#define DEBUG

/* POSIX.1 says each process has at least 20 file descriptors.
 * Three of those belong to the standard streams.
 * Here, we use a conservative estimate of 15 available;
 * assuming we use at most two for other uses in this program,
 * we should never run into any problems.
 * Most trees are shallower than that, so it is efficient.
 * Deeper trees are traversed fine, just a bit slower.
 * (Linux allows typically hundreds to thousands of open files,
 *  so you'll probably never see any issues even if you used
 *  a much higher value, say a couple of hundred, but
 *  15 is a safe, reasonable value.)
*/
#ifndef USE_FDS
#define USE_FDS 15  // the maximum number of directories that ftw() will hold open simultaneously
#endif

#ifndef BUFFSIZE
#define BUFFSIZE 1024
#endif

// to keep track of where we are processing
static char currDir[BUFFSIZE];

// for use to indicate which flag was used
static int iFlag;
static int RFlag;
static int lFlag;
// for cases when there is 1 or less dir path arguments passed in. 'ls' cmd does not print the header
static int omitHeaderFlag;

// Finds a specific string in a (portion of) str array
// used for finding special patterns '..', '.', '~' in argv[]
// search starts from strArr[beginArrPos] through strArr[endArrPos]
// note that this is a whole-string find, so searching for  "." for example will not return ".."
// return the index of first string matching the pattern
// return -1 upon not found
static int findArgument(char *pattern, char **strArr, int beginArrPos, int endArrPos);

// prints the directory passed in
// return errno (0= no error)
int print_directory_tree(const char *dirpath);

// core function that prints the  info in a file/directory given
// parameters specified by nftw()
// https://linux.die.net/man/3/nftw
static int printFile(const char *filepath, const struct stat *info,
                     const int typeflag, struct FTW *pathinfo);

// This is a non-standard system function on Solaris only
// Hence must be defined in UNIX (despite having to type out its name..)
// https://stackoverflow.com/a/9639381/5340330
char const *sperm(__mode_t mode);

// Boolean func to determine whether the filepath is a special directory
// special directory is defined as one of the three: '.', '..', or a hidden file (name beginning with .xx)
// filename must not contain path (including ./xxx)
// return 1 for "." or ".."; return 2 for hidden file/dir; 0 for false
int isSpecialDir(const char *filename);

#ifdef DEBUG
// determines if the first x chars matches currDir, if it does, we can assume that its a subdirectory
// handles filepaths in the format of both './xxx' and plain 'xxx'
// returns ptr to the location of '/' for true, 0 for false
char *isSubDir(const char *filePath);
#endif

// this function trims the filepath
// similar to basename(), but substitutes the filename to '.' if it matches currDir[]
void getFilename(char *filename, const char *filepath);

int main(int argc, char *argv[]) {
    iFlag = 0;
    RFlag = 0;
    lFlag = 0;

    int index;
    int flagChar;
    int ret;
    opterr = 0;

    // ------------------------used getopt to parse flags-------------------
    // https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
    while ((flagChar = getopt(argc, argv, "iRl")) != -1)
        switch (flagChar) {
            case 'i':
                iFlag = 1;
                break;
            case 'R':
                RFlag = 1;
                break;
            case 'l':
                lFlag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-iRl] [directory...]\n", argv[0]);
                return EXIT_FAILURE;
        }

    // ----------------------the non-flag arguments------------------------
    // decided if header printing for directory is needed (either no dir argument or just one)
    if(optind >=argc-1)
        omitHeaderFlag=1;

    // no directory argument was passed in: print current directory
    if (optind == argc){
        omitHeaderFlag=1;
        if ((ret = print_directory_tree(".") )!= 0) {
            printf("UnixLs: cannot access '.': %s",strerror(ret));
        }
    }
    else {
        // Note: ls cmd always print . first, then .., then ~, (if they exist) then everything else
        // for this reason we first search for these three special cases
        if (findArgument(".", argv, optind, argc - 1) > 0){  // even tho 0 indicates a found, argv[0] is program name
            if ((ret = print_directory_tree(".") )!= 0) {
                printf("UnixLs: cannot access '.': %s",strerror(ret));
            }
        }
        if (findArgument("..", argv, optind, argc - 1) > 0) {  // even tho 0 indicates a found, argv[0] is program name
            if ((ret = print_directory_tree("..") )!= 0) {
                printf("UnixLs: cannot access '..': %s",strerror(ret));
            }
        }
        if (findArgument("~", argv, optind, argc - 1) > 0) {  // even tho 0 indicates a found, argv[0] is program name
            if ((ret = print_directory_tree("~")) != 0) {
                printf("UnixLs: cannot access '~': %s",strerror(ret));
            }
        }

        // print the rest of the directories
        for (index = optind; index < argc; index++) {
            if (strcmp(argv[index], ".") && strcmp(argv[index], "..") && strcmp(argv[index], "~")) {
                if ((ret = print_directory_tree(argv[index])) != 0){
                    printf("UnixLs: cannot access '%s': %s",argv[index], strerror(ret));
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

// Finds a specific string in a (portion of) str array
// used for finding special patterns '..', '.', '~' in argv[]
// search starts from strArr[beginArrPos] through strArr[endArrPos]
// note that this is a whole-string find, so searching for  "." for example will not return ".."
// return the index of first string matching the pattern
// return -1 upon not found
static int findArgument(char *pattern, char **strArr, int beginArrPos, int endArrPos) {
    int i;

    for (i = beginArrPos; i < endArrPos + 1; i++) {
        if (!strcmp(strArr[i], pattern) && strlen(pattern) == strlen(strArr[i])) {
            return i;
        }
    }
    return -1;
}

// core function that prints the  info in a file/directory given
// parameters specified by nftw()
// https://linux.die.net/man/3/nftw
static int printFile(const char *filepath, const struct stat *info,
                     const int typeflag, struct FTW *pathinfo) {
    // get size of file/dir
    unsigned int bytes;
    struct tm mtime;
    struct passwd *pwd;
    struct group *grp;
    char filename[BUFFSIZE];
    char dateStrBuff[13];
    int ret;


    // trim filepath
    getFilename(filename, filepath);

    // ignore . and .. and hidden files
    ret=isSpecialDir(filename);
    if (ret==1)
        return FTW_CONTINUE;
    else if(ret==2){     // if hidden dir, skip files
        if (typeflag == FTW_D || typeflag == FTW_DP)
            return FTW_SKIP_SUBTREE;
        else return FTW_CONTINUE;
    }

    // -i: begin the line with printing the inode #
    if (iFlag) {
        printf("%-6lu ", info->st_ino);    // unsigned long st_into
    }

    // -l: printing additional info before the filename
    // format: r/w/e permission, # of hard links, owner name, group name, size in bytes, lost modified timestamp
    if (lFlag) {
        // Print out type, permissions, and number of links
        if (typeflag == FTW_D || typeflag == FTW_DP)       // valid dir (FTW_DP = FTW_DEPTH was specified in flags)
            printf("d");
        else
            printf("-");
        printf("%-10.10s", sperm(info->st_mode));
        printf("%-2lu", info->st_nlink);

        // Print out owner's and group's name if it is found using getpwuid()
        // http://pubs.opengroup.org/onlinepubs/009695399/functions/getpwuid.html
        // http://pubs.opengroup.org/onlinepubs/009695399/functions/getgrgid.html
        if ((pwd = getpwuid(info->st_uid)) != NULL)
            printf(" %-8.8s", pwd->pw_name);
        else
            printf(" %-8d", info->st_uid);
        if ((grp = getgrgid(info->st_gid)) != NULL)
            printf(" %-8.8s", grp->gr_name);
        else
            printf(" %-8d", info->st_gid);


        // print out file/dir size
        bytes = (unsigned int) info->st_size; /* Not exact if large! */
//        if (bytes >= 1099511627776.0)
//            printf(" %9.3f TiB", bytes / 1099511627776.0);
//        else if (bytes >= 1073741824.0)
//            printf(" %9.3f GiB", bytes / 1073741824.0);
//        else if (bytes >= 1048576.0)
//            printf(" %9.3f MiB", bytes / 1048576.0);
//        else if (bytes >= 1024.0)
//            printf(" %9.3f KiB", bytes / 1024.0);
//        else
        printf(" %-6u", bytes);

        // get last modified timestamp of file/dir
        localtime_r(&(info->st_mtime), &mtime);
//        printf(" %04d-%02d-%02d %02d:%02d:%02d",
//               mtime.tm_year + 1900, mtime.tm_mon + 1, mtime.tm_mday,
//               mtime.tm_hour, mtime.tm_min, mtime.tm_sec);
        strftime(dateStrBuff, 20, "%b %d %H:%M", &mtime);
        printf(" %s ",dateStrBuff);
    }

    // file/dir name printing
    // handle symbolic link
    if (typeflag == FTW_SL) {
        if (lFlag) {
            char *target;
            size_t maxlen = 1023;
            ssize_t len;

            while (1) {
                target = malloc(maxlen + 1);
                if (target == NULL)
                    return ENOMEM;      // throw errno 'out of memory'

                len = readlink(filepath, target, maxlen);
                if (len == (ssize_t) -1) {
                    // save errno, free mem, and toss errno
                    const int saved_errno = errno;
                    free(target);
                    return saved_errno;
                }
                // in case of ssize_t overflow, a problem in readlink() Issue 6:
                /*
                 * In this function it is possible for the return value to exceed the
                 * range of the type ssize_t (since size_t has a larger range of positive
                 * values than ssize_t). A sentence restricting the size of the size_t
                 * object is added to the description to resolve this conflict.
                 * http://pubs.opengroup.org/onlinepubs/009695399/functions/readlink.html
                 */
                if (len >= (ssize_t) maxlen) {
                    free(target);
                    maxlen += 1024;
                    continue;
                }

                target[len] = '\0';
                break;
            }

            printf("%s -> %s\n", filename, target);
            free(target);
        } else
            printf("%s\n", filename);
    }
        // filepath is a symbolic link pointing to a nonexistent file.
    else if (typeflag == FTW_SLN)
        printf("%s (dangling symlink)\n", filename);
    else if (typeflag == FTW_F)     // regular file
        printf("%s\n", filename);
    else if (typeflag == FTW_D || typeflag == FTW_DP) {      // valid dir (FTW_DP = FTW_DEPTH was specified in flags)
        printf("%s\n", filename);
        return FTW_SKIP_SUBTREE;
    } else if (typeflag == FTW_DNR)       // unreadable dir
        printf("%s/ (unreadable)\n", filename);
    else
        printf("%s (unknown)\n", filename);

    return FTW_CONTINUE;
}

// prints the directory passed in
// return errno (0= no error)
int print_directory_tree(const char *dirpath) {
    int ret;

    // handle invalid directory path
    if (dirpath == NULL || *dirpath == '\0')
        return errno = EINVAL;

    if(!strcmp(dirpath,"~")){
        // resolve special filepath '~'
        // https://stackoverflow.com/a/2910392
        struct passwd *pw = getpwuid(getuid());
        dirpath = pw->pw_dir;
    }

    // no directory argument or just one
    if(omitHeaderFlag)
        omitHeaderFlag=0;       // reset in case -R is set
    else
        printf("%s:\n", dirpath);


    strcpy(currDir, dirpath);
    ret = nftw(dirpath, printFile, USE_FDS, FTW_PHYS | FTW_ACTIONRETVAL);     /* set FTW_PHYS: Do not follow sym links
*                                                                since we are doing it manually*/
    if (ret==-1){
        errno=ENOENT;
        return ENOENT;
    }
    puts("");
    // additionally, handle -R flag
    if (RFlag) {
        DIR *dir;
        DIR *subDir;
        struct dirent *direntPtr;
        char filepathBuff[BUFFSIZE];

        if ((dir = opendir(dirpath)) == NULL) {
            ret = errno;
            fprintf(stderr, "Cannot open '%s': %s\n", dirpath, strerror(ret));
            return 0;
        }
        strcpy(currDir, dirpath);
        while ((direntPtr = readdir(dir)) != NULL) {
            if (direntPtr->d_type == DT_DIR && !isSpecialDir(direntPtr->d_name)) {
                strcpy(filepathBuff, dirpath);
                strcat(filepathBuff, "/");
                strcat(filepathBuff, direntPtr->d_name);
                if ((subDir = opendir(filepathBuff)) == NULL) {
                    ret = errno;
                    fprintf(stderr, "Cannot open '%s': %s\n", direntPtr->d_name, strerror(ret));
                    return 0;
                }
                // print entries in subdir
                if ((ret = print_directory_tree(filepathBuff)) != 0) {
                    printf("UnixLs: cannot access '%s': %s",filepathBuff,strerror(ret));
                }
                closedir(subDir);
            }
        }
        closedir(dir);
    }
    return 0;
}

// This is a non-standard system function on Solaris only
// Hence must be defined in UNIX (despite having to type out its name..)
// https://stackoverflow.com/a/9639381/5340330
char const *sperm(__mode_t mode) {
    // the bits in st_mode are interpreted as such:
    // https://stackoverflow.com/a/35375259
    static char local_buff[16] = {0};
    int i = 0;
    // user permissions
    if ((mode & S_IRUSR) == S_IRUSR) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWUSR) == S_IWUSR) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXUSR) == S_IXUSR) local_buff[i] = 'x';
    else local_buff[i] = '-';
    i++;
    // group permissions
    if ((mode & S_IRGRP) == S_IRGRP) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWGRP) == S_IWGRP) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXGRP) == S_IXGRP) local_buff[i] = 'x';
    else local_buff[i] = '-';
    i++;
    // other permissions
    if ((mode & S_IROTH) == S_IROTH) local_buff[i] = 'r';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IWOTH) == S_IWOTH) local_buff[i] = 'w';
    else local_buff[i] = '-';
    i++;
    if ((mode & S_IXOTH) == S_IXOTH) local_buff[i] = 'x';
    else local_buff[i] = '-';
    return local_buff;
}

// Boolean func to determine whether the filepath is a special directory
// special directory is defined as one of the three: '.', '..', or a hidden file (name beginning with .xx)
// filename must not contain path (including ./xxx)
// return 1 for "." or ".."; return 2 for hidden file/dir; 0 for false
int isSpecialDir(const char *filename) {
    if(!strcmp(filename,".") || !strcmp(filename,"."))
        return 1;
    else if(strlen(filename) && filename[0]=='.'){
        // hidden file/dir
        return 2;
    }
    else return 0;
}

#ifdef DEBUG
// determines if the first x chars matches currDir, if it does, we can assume that its a subdirectory
// handles filepaths in the format of both './xxx' and plain 'xxx'
// returns ptr to the location of '/' for true, 0 for false
char *isSubDir(const char *filePath) {
    char *ptr = (char *) filePath;
    size_t len = strlen(currDir);
    // if filepath has the format of 'currDir/xxx', we trim currDir/ from filePath
    if (strlen(filePath) > len) {
        char strBuff[BUFFSIZE];
        strncpy(strBuff, filePath, (size_t) len);      // strncpy does not NULL TERMINATE!
        strBuff[len] = '\0';
        if (!strcmp(strBuff, currDir)) {
            ptr = (char *) filePath + len;
            if (filePath[len] == '/')
                ptr++;
        }
    }
    return strchr(ptr, '/');
}
#endif

// this function trims the filepath
// similar to basename(), but substitutes the filename to '.' if it matches currDir[]
void getFilename(char *filename, const char *filePath) {
    if (!strcmp(filePath, currDir)) {
        filename[0] = '.';
        filename[1]='\0';
    } else {
        char *namePtr = basename((char *) filePath);        // basename discards const qualifier
        strcpy(filename, namePtr);
    }

}