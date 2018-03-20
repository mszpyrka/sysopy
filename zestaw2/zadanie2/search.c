#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ftw.h>

struct tm arg_date;
int operator_type;  // -1: '<';     0: '=';     1: '>'

// Compares two tm structures (considers only years and days values)
int compare_dates(struct tm* date_a, struct tm* date_b) {

    if(date_a -> tm_year > date_b -> tm_year
      || (date_a -> tm_year == date_b -> tm_year && date_a -> tm_yday > date_b -> tm_yday))
        return 1;

    if(date_a -> tm_year == date_b -> tm_year && date_a -> tm_yday == date_b -> tm_yday)
        return 0;

    return -1;
}

// Fills passed array with randomly generated values (in this case -> char values)
void process_file(const char* path_buffer, const struct stat* stat_buffer) {

    struct tm* stat_date;
    stat_date = localtime(&(stat_buffer -> st_mtime));

    // Skips processing file if it doesn't satisfy the date constraint
    if(compare_dates(stat_date, &arg_date) != operator_type)
        return;

    char date_string[20];
    strftime(date_string, 20, "%b %d %Y", localtime(&(stat_buffer -> st_mtime)));

    printf("%s\n", path_buffer);
    printf("-");
    printf((stat_buffer -> st_mode & S_IRUSR) ? "r" : "-");
    printf((stat_buffer -> st_mode & S_IWUSR) ? "w" : "-");
    printf((stat_buffer -> st_mode & S_IXUSR) ? "x" : "-");
    printf((stat_buffer -> st_mode & S_IRGRP) ? "r" : "-");
    printf((stat_buffer -> st_mode & S_IWGRP) ? "w" : "-");
    printf((stat_buffer -> st_mode & S_IXGRP) ? "x" : "-");
    printf((stat_buffer -> st_mode & S_IROTH) ? "r" : "-");
    printf((stat_buffer -> st_mode & S_IWOTH) ? "w" : "-");
    printf((stat_buffer -> st_mode & S_IXOTH) ? "x" : "-");
    printf("\t%ld", stat_buffer -> st_size);
    printf("\t%s\n\n", date_string);
}


// Extracts time interval from two timeval structures
double subtract_time(struct timeval a, struct timeval b)
{
    double tmp_a = ((double) a.tv_sec)  + (((double) a.tv_usec) / 1000000);
    double tmp_b = ((double) b.tv_sec)  + (((double) b.tv_usec) / 1000000);
    return tmp_a - tmp_b;
}

// Prints time value in proper format
void print_time(double t)
{
    int minutes = (int) (t / 60);
    double seconds = t - minutes * 60;
    printf("%dm%.4fs\n", minutes, seconds);
}

// Processes all entries in gived directory (processes inner directories recursively)
void search_dir(char* path_buffer, int* path_buffer_size) {

    // Saves index of path_buffer last character
    strcat(path_buffer, "/");
    int path_end_index = strlen(path_buffer);

    // Writes NULL character at the end of current path to ensure that all copying from and to buffer will work properly
    path_buffer[path_end_index] = '\0';

    // If more than a half of buffer is used, it is reallocated with twice the capacity
    if((*path_buffer_size) < path_end_index * 2) {
        path_buffer = realloc(path_buffer, (*path_buffer_size) * 2 * sizeof(char));
        *path_buffer_size *= 2;
    }

    // Opens directory
    DIR* current_dir;
    if ((current_dir = opendir (path_buffer)) == NULL) {
        perror("Error occurred while opening directory");
        fprintf(stderr, "%s\n", path_buffer);
        exit (1);
    }

    // Reads directory until all entries are processed
    struct dirent* entry;
    struct stat stat_buffer;
    while((entry = readdir(current_dir)) != NULL) {

        // Ignores '.' and '..' entries
        if(strcmp(entry -> d_name, ".") == 0 || strcmp(entry -> d_name, "..") == 0)
            continue;

        // Creates path to entry
        strcat(path_buffer, entry -> d_name);

        // Saves entry's metadata to stat_buffer
        if(lstat(path_buffer, &stat_buffer) < 0) {
            perror("Error occurred while getting file's stat");
            fprintf(stderr, "%s\n", path_buffer);
            exit(1);
        }

        // Processes all regular files
        if(S_ISREG(stat_buffer.st_mode))
            process_file(path_buffer, &stat_buffer);

        // Processes directories recorsuvely
        else if(S_ISDIR(stat_buffer.st_mode))
            search_dir(path_buffer, path_buffer_size);

        path_buffer[path_end_index] = '\0';
    }

    // Closes directory
    if (closedir(current_dir) != 0) {
        perror("Error occurred while closing directory");
        fprintf(stderr, "%s\n", path_buffer);
        exit (1);
    }
}

// Function intended to use as a parameter for nftw function
int nftw_util(const char* path_buffer, const struct stat* stat_buffer, int entry_info, struct FTW* ftw_str) {

    if(entry_info == FTW_F)
        process_file(path_buffer, stat_buffer);

    return 0;
}

// Prints example program usage
void print_usage() {

    printf("example usage: ./main ../zad1 '<' 'mar 19 2017' custom\n");
    printf("example usage: ./main /usr/include '=' 'january 1 2017' nftw\n");
}

int main(int argc, char** argv) {

    if(argc != 5) {

        fprintf(stderr, "Wrong arguments format\n");
        print_usage();
        exit(1);
    }

    char* path_buffer = malloc(1000 * sizeof(char));
    path_buffer[0] = '\0';

    // If argument is an absolute path -> copies it into path_buffer
    if(argv[1][0] == '/')
        strcpy(path_buffer, argv[1]);

    // If not -> concats it with current working directory to make absolute path
    else {
        getcwd(path_buffer, 1000);
        strcat(path_buffer, "/");
        strcat(path_buffer, argv[1]);
    }

    // Parsing operator arg
    if(strcmp(argv[2], "<") == 0)
        operator_type = -1;

    else if(strcmp(argv[2], "=") == 0)
        operator_type = 0;

    else if(strcmp(argv[2], ">") == 0)
        operator_type = 1;

    else {
        fprintf(stderr, "Wrong arguments format\n");
        print_usage();
        exit(1);
    }

    int* buffer_size = malloc(sizeof(int));
    *buffer_size = 1000;

    // Parsing date arg
    strptime(argv[3], "%b %d %Y", &arg_date);

    if(strcmp(argv[4], "custom") == 0) {
        printf("searching with custom function:\n\n");
        search_dir(path_buffer, buffer_size);
    }

    else if(strcmp(argv[4], "nftw") == 0) {
        printf("searching with nftw:\n\n");
        nftw(path_buffer, nftw_util, 10, FTW_PHYS);
    }

    else {

        fprintf(stderr, "Wrong arguments format\n");
        print_usage();
        exit(1);
    }
}
