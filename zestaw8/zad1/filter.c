#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

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

// calculates collumn ranges that cover the whole input image
void split_image() {

    int base_range_size = image_width / threads_number;
    int excess = image_width - (base_range_size * threads_number);

    int range_begin = 0;

    for(int i = 0; i < threads_number; i++) {

        int range_end = range_begin + base_range_size;
        if(i < excess)
            range_end++;

        ranges[i].begin = range_begin;
        ranges[i].end = range_end;

        range_begin = range_end;
    }
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
void read_files(const char *input_file, const char *filter_file) {

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
    read_files(argv[2], argv[3]);

    for(int i = 0; i < image_height; i++)
        for(int j = 0; j < image_height; j++)
            process_single_pixel(i, j);

    save_output(argv[4]);
}
