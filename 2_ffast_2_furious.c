#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>
#include <sys/types.h>

// 2 Ffast 2 Furious: Or, "The Narcissism of Small Differences"
//
// A more realistic demo of a buffer overflow that occurs only when FTZ/DAZ
// are enabled. Special thanks to an1sotropy on Hacker News for suggesting
// this scenario:
// https://news.ycombinator.com/item?id=32774694

// In this imaginary app, we are computing a histogram of differences between
// two values, for plotting in the upper left quadrant of a graph (so the bin
// indices are reversed).

#define HIST_BINS  16

// The vulnerable function. The vulnerability stems from the fact that when
// FTZ/DAZ are enabled, the subtraction of two small (but normal) numbers can
// result in a denormal number, which is then flushed to zero. This means that
// the bin index becomes 0, which is a buffer overflow.
void add_to_bin(int *bins, int bincount, float a, float b) {
    int bin;
    // Programmer thinks: "As long as a != b, we'll get a non-zero difference,
    // so we can just take ceilf() to get the bin index."
    if (a != b) {
        bin = (int)ceilf(a > b ? a - b : b - a);
    }
    else {
        bin = 1;
    }
    // Differences larger than bincount will be clamped to the last bin.
    // For simplicity I'm not doing any normalization based on the range of
    // the input values.
    if (bin > bincount) {
        bin = bincount;
    }
    if (bin == 0) fprintf(stderr, "DEBUG: Will write to bins[%d]\n", bincount - bin);
    bins[bincount - bin]++;
}

// Some UTF-8 drawing characters
#define BAR_ELEMENT "\xe2\x96\x92"
#define HORZ_LINE "\xe2\x94\x81"
#define VERT_LINE "\xe2\x94\x83"
#define UP_ARROW "\xe2\x96\xb2"
#define LEFT_ARROW "\xe2\x97\x80"
#define LOWER_RIGHT_CORNER "\xe2\x94\x9b"
// Print a histogram to stdout. This is not necessary for the demo,
// I just thought it would be fun.
void draw_histogram(int *bins, int bincount, int n) {
    int max_count = 0;
    for (int i = 0; i < bincount; i++) {
        if (bins[i] > max_count) {
            max_count = bins[i];
        }
    }

    // Title, centered in the main drawing area
    #define DRAWING_AREA_SIZE (2+bincount*3+1)
    size_t needed = snprintf(NULL, 0, "Histogram of abs(a-b) for %d pairs", n)+1;
    char *title = malloc(needed);
    snprintf(title, needed, "Histogram of abs(a-b) for %d pairs", n);
    int title_len = needed - 1;
    int title_offset = (DRAWING_AREA_SIZE - title_len) / 2;
    printf("%*s%s\n", title_offset, "", title);
    free(title);

    // Draw topmost row and up arrow
    printf("  ");
    for (int i = 0; i < bincount; i++) {
        printf("   ");
    }
    printf(UP_ARROW " Ct\n");

    // Draw the rest of the rows
    for (int i = max_count; i > 0; i--) {
        printf("  ");
        // Draw each bar
        for (int j = 0; j < bincount; j++) {
            if (bins[j] >= i) {
                printf(BAR_ELEMENT BAR_ELEMENT " ");
            }
            else {
                printf("   ");
            }
        }
        // Vertical line
        printf(VERT_LINE " %d\n", i);
    }

    // Draw the x axis.
    printf(LEFT_ARROW HORZ_LINE);
    for (int i = 0; i < bincount-1; i++) {
        printf(HORZ_LINE HORZ_LINE HORZ_LINE);
    }
    printf(HORZ_LINE HORZ_LINE HORZ_LINE LOWER_RIGHT_CORNER "\n");

    // Draw the x axis labels.
    printf("  ");
    for (int i = bincount; i > 0; i--) {
        printf("%2d ", i);
    }
    printf("\n");

    // x axis title, centered in the main drawing area
    needed = strlen("Difference");
    title_offset = (DRAWING_AREA_SIZE - needed) / 2;
    printf("%*s%s\n", title_offset, "", "Difference");
}

int load_data_from_csv(const char *filename, float (**out_data)[][2], size_t *out_rows) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Could not open file %s\n", filename);
        return 1;
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    size_t row = 0;
    // Allocate an initial buffer for the 2D array.
    float (*data)[2] = malloc( sizeof(float[1][2]) );
    while ((read = getline(&line, &len, fp)) != -1) {
        float a, b;
        if (sscanf(line, "%f,%f", &a, &b) == 2) {
            data[row][0] = a;
            data[row][1] = b;
        }
        else {
            fprintf(stderr, "ERROR: Could not parse line: %s\n", line);
            return 1;
        }
        row++;
        data = realloc(data, sizeof(float) * 2 * (row+1));
    }
    fclose(fp);
    if (line) {
        free(line);
    }
    *out_data = (float (*)[][2])data;
    *out_rows = row;
    return 0;
}

int main(int argc, char **argv) {
    // Argument parsing
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <csv file> [fast]\n", argv[0]);
        return 1;
    }

    // Fast math mode, if desired
    if (argc > 2 && strcmp(argv[2], "fast") == 0) {
        // Load the fast math library (in reality just an empty file built
        // with -ffast-math).
        void *handle = dlopen("./gofast.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "Failed to load ./gofast.so: %s\n", dlerror());
            return 1;
        }
        else {
            fprintf(stderr, "Loaded ./gofast.so\n");
            fprintf(stderr, "Fast math mode enabled! Gotta go fast!\n");
        }
    }

    // Load data
    float (*data)[][2];
    size_t rows;
    if (load_data_from_csv(argv[1], &data, &rows)) {
        return 1;
    }
    // fprintf(stderr, "Loaded %zu rows from %s\n", rows, argv[1]);

    // Compute histogram
    int *bins = calloc(HIST_BINS, sizeof(int));
    for (size_t i = 0; i < rows; i++) {
        add_to_bin(bins, HIST_BINS, (*data)[i][0], (*data)[i][1]);
    }

    // Draw histogram
    draw_histogram(bins, HIST_BINS, rows);

    // Clean up
    free(data);
    free(bins);

    return 0;
}
