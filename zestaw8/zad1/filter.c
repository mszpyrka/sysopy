#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/time.h>

// Represents a range of image collumns -> [begin; end)
struct range {
    int begin;
    int end;
};

int **input_image;
int **output_image;
int image_width;
int image_height;

double **filter;
int filter_size;

int threads_number;
struct range *ranges;


int max(int a, int b) {
    return (a > b) ? a : b;
}

int min(int a, int b) {
    return (a < b) ? a : b;
}

int ceil2(double a) {
    return (int) (a + 1.0);
}

int round2(double a) {
    return (int) (a + 0.5);
}

// Calculates value of a single pixel in the output image
void process_single_pixel(int row, int column) {

    int half_size = ceil2( ((double) filter_size) / 2.0 );
    double result = 0.0;

    for(int i = 0; i < filter_size; i++)
        for(int j = 0; j < filter_size; j++) {

            int tmp_row = min(image_height - 1, max(0, row - half_size + i));
            int tmp_column = min(image_width - 1, max(0, column - half_size + j));
            result += ((double) input_image[tmp_row][tmp_column]) * filter[i][j];
        }

    output_image[row][column] = min(255, max(0, round2(result)));
}

// calculates equal-sized pixel ranges that cover the whole input image
void split_image(int parts_number) {

    int pixels_number = image_width * image_height;
    int base_range_size = pixels_number / parts_number;
    int excess = pixels_number - (base_range_size * parts_number);

    int range_begin = 0;

    for(int i = 0; i < parts_number; i++) {

        int range_end = range_begin + base_range_size;
        if(i < excess)
            range_end++;

        ranges[i].begin = range_begin;
        ranges[i].end = range_end;

        range_begin = range_end;
    }
}

// Function that each thread uses to process it's part of the image
void *thread_range_process(void *arg) {

    struct range *pixel_range = (struct range*) arg;

    for(int i =pixel_range -> begin; i < pixel_range -> end; i++) {

        int row = i / image_width;
        int column = i % image_width;
        process_single_pixel(row, column);
    }

    return NULL;
}

// Allocates all global arrays
void allocate_global_memory() {

    input_image = malloc(sizeof(int*) * image_height);
    output_image = malloc(sizeof(int*) * image_height);

    for(int i = 0; i < image_height; i++) {
        input_image[i] = malloc(sizeof(int) * image_width);
        output_image[i] = malloc(sizeof(int) * image_width);
    }

    filter = malloc(sizeof(int*) * filter_size);

    for(int i = 0; i < filter_size; i++)
        filter[i] = malloc(sizeof(int) * filter_size);

    ranges = malloc(sizeof(struct range) * threads_number);
}

// Loads data from input_image and filter files
void initialize_global_memory(const char *input_file, const char *filter_file) {

    FILE *input_fp = fopen(input_file, "r");
    FILE *filter_fp = fopen(filter_file, "r");

    char buffer[10];

    fscanf(input_fp, "%s", buffer);
    fscanf(input_fp, "%d", &image_width);
    fscanf(input_fp, "%d", &image_height);
    fscanf(input_fp, "%s", buffer);

    fscanf(filter_fp, "%d", &filter_size);

    allocate_global_memory();

    printf("reading image: width = %d, height = %d\n", image_width, image_height);

    for(int i = 0; i < image_height; i++)
        for(int j = 0; j < image_width; j++)
            fscanf(input_fp, "%d", input_image[i] + j);


    printf("reading filter: size = %d\n", filter_size);

    for(int i = 0; i < filter_size; i++)
        for(int j = 0; j < filter_size; j++)
            fscanf(filter_fp, "%lf", filter[i] + j);

    fclose(input_fp);
    fclose(filter_fp);
}

// Saves output_image data into output file
void save_output(const char *output_file) {

    FILE *output_fp = fopen(output_file, "w");

    fprintf(output_fp,
        "P2\n"
        "%d %d\n"
        "%d\n",
        image_width, image_height, 255);

    for(int i = 0; i < image_height; i++) {
        for(int j = 0; j < image_width; j++)
            fprintf(output_fp, "%d ", output_image[i][j]);

        fprintf(output_fp, "\n");
    }

    fclose(output_fp);
}

// Function used to calculate time interval between two timeval structures
double subtract_time(struct timeval a, struct timeval b)
{
    double tmp_a = ((double) a.tv_sec)  + (((double) a.tv_usec) / 1000000);
    double tmp_b = ((double) b.tv_sec)  + (((double) b.tv_usec) / 1000000);
    return tmp_a - tmp_b;
}

// Prints properly formatted time
void print_time(double t)
{
    int minutes = (int) (t / 60);
    double seconds = t - minutes * 60;
    printf("%dm%.4fs\n", minutes, seconds);
}


void print_usage() {

    printf("Example usage: ./filter <threads_number> <image_file> <filter_file> <output_file>\n");
}

int main(int argc, char** argv) {

    if(argc != 5) {
        fprintf(stderr, "wrong arguments format\n");
        print_usage();
        exit(1);
    }

    threads_number = atoi(argv[1]);
    initialize_global_memory(argv[2], argv[3]);

    // Saves star times of main code execution
    struct rusage ru_start, ru_end;
    struct timeval sys_start, sys_end, user_start, user_end, real_start, real_end;

    struct timespec real_start_ts, real_end_ts;
    clock_gettime(CLOCK_MONOTONIC, &real_start_ts);

    getrusage(RUSAGE_SELF, &ru_start);


    // Filters image with multiple threads
    split_image(threads_number);

    pthread_t threads[threads_number];

    for(int i = 0; i < threads_number; i++)
        pthread_create(threads + i, NULL, thread_range_process, ranges + i);

    for(int i = 0; i < threads_number; i++)
        pthread_join (threads[i], NULL);


    // Saves end time
    clock_gettime(CLOCK_MONOTONIC, &real_end_ts);
    getrusage(RUSAGE_SELF, &ru_end);

    real_start.tv_sec = real_start_ts.tv_sec;
    real_start.tv_usec = real_start_ts.tv_nsec / 1000;
    real_end.tv_sec = real_end_ts.tv_sec;
    real_end.tv_usec = real_end_ts.tv_nsec / 1000;

    sys_start = ru_start.ru_stime;
    user_start = ru_start.ru_utime;
    sys_end = ru_end.ru_stime;
    user_end = ru_end.ru_utime;

    printf("image filtering execution time:\n");
    printf("real\t");
    print_time(subtract_time(real_end, real_start));
    printf("user\t");
    print_time(subtract_time(user_end, user_start));
    printf("sys\t");
    print_time(subtract_time(sys_end, sys_start));

    save_output(argv[4]);
}
