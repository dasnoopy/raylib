#include <stdio.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <linux/limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>

#define STAT_FILE ("/proc/stat")

#define ERROR_FILE_NOT_FOUND (1)
#define ERROR_FILE_READING (2)

static int read_cpu_usage(char* buffer, const int buffer_size, float* usage) {
    const char* delim = " ";

    FILE* file;
    int total = 0;
    int idle = 0;
    int current_value;
    int index = 0;

    file = fopen(STAT_FILE, "r");
    if (file == NULL) {
        return ERROR_FILE_NOT_FOUND;
    }

    if (fgets(buffer, buffer_size, file) == NULL) {
        fclose(file);
        return ERROR_FILE_READING;
    }

    fclose(file);

    strtok(buffer, delim);
    char* ptr = strtok(NULL, delim);
    while (ptr != NULL) {
        ++index;

        current_value = strtol(ptr, NULL, 10);
        total += current_value;
        if (index == 4) {
            idle = current_value;
        }

        ptr = strtok(NULL, delim);
    }

    *usage = 1.0 - (idle / (float) total);

    return 0;
}

int main(int argc, char** argv) {
    char buffer[PATH_MAX];
    float usage;
    int result = read_cpu_usage(buffer, 100, &usage);
    if (result == 0) {
        printf("Usage %.3f%%\n", (usage * 100));
    }

    return result;
}