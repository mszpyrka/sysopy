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

// Fills passed array with randomly generated values (in this case -> char values)
void process_file(char* path_buffer, struct stat* stat_buffer, char* operator, char* date) {

    struct tm arg_time_tm;
    strptime(date, "%b %d %", &arg_time_tm);
    time_t arg_time = mktime(&arg_time_tm);

    if(!(   (operator[0] == '<' && stat_buffer -> st_mode < arg_time)
         || (operator[0] == '=' && stat_buffer -> st_mode == arg_time)
         || (operator[0] == '>' && stat_buffer -> st_mode > arg_time) ))
        return;

    char date_string[80];
    strftime(date_string, 80, "%b %d %Y", localtime(&(stat_buffer -> st_mtime)));

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
void search_dir(char* path_buffer, int* path_buffer_size, char* operator, char* date) {

    // Saves index of path_buffer last character
    int path_end_index = strlen(path_buffer) + 1;
    path_buffer[path_end_index - 1] = '/';

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
        exit (1);
    }

    // Reads directory until all entries are processed
    struct dirent* entry;
    struct stat stat_buffer;
    while((entry = readdir(current_dir)) != NULL) {

        if(strcmp(entry -> d_name, ".") == 0 || strcmp(entry -> d_name, "..") == 0)
            continue;

        strcat(path_buffer, entry -> d_name);
        if(stat(path_buffer, &stat_buffer) < 0) {
            perror("Error occurred while getting file's stat");
            exit(1);
        }

        if((stat_buffer.st_mode & S_IFREG) != 0)
            process_file(path_buffer, &stat_buffer, operator, date);

        else if((stat_buffer.st_mode & S_IFDIR) != 0)
            search_dir(path_buffer, path_buffer_size, operator, date);

        //free(entry);
        path_buffer[path_end_index] = '\0';
    }

    // Closes directory
    if (closedir(current_dir) != 0) {
        perror("Error occurred while closing directory");
        exit (1);
    }
}

int main(int argc, char** argv) {

    printf("%d\n", argc);

    if(argc != 4) {

        fprintf(stderr, "Wrong arguments format\n");
        exit(1);
    }

    char* path_buffer = malloc(1000 * sizeof(char));
    path_buffer[0] = '\0';

    // If argument is an absolute path -> copies it into path_buffer
    if(argv[1][0] == '/')
        strcpy(path_buffer, argv[1]);

    else {
        getcwd(path_buffer, 1000);
        strcat(path_buffer, "/");
        strcat(path_buffer, argv[1]);
    }

    printf("XD\n");

    if(strcmp(argv[2], "<") != 0 && strcmp(argv[2], "=") != 0 && strcmp(argv[2], ">") != 0) {

        fprintf(stderr, "Wrong arguments format\n");
        exit(1);
    }

    int* buffer_size = malloc(sizeof(int));
    *buffer_size = 1000;

    search_dir(path_buffer, buffer_size, argv[2], argv[3]);
}
