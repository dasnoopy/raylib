// Source - https://stackoverflow.com/a/77278639
// Posted by farcas, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-19, License - CC BY-SA 4.0

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

char *humanMemorySize(uint64_t bytes) {
    char *result = (char *) malloc(sizeof(char) * 20);
    char *sizeNames[] = { "B", "KB", "MB", "GB" };

    uint64_t i = (uint64_t) floor(log(bytes) / log(1024));
    double humanSize = bytes / pow(1024, i);
    snprintf(result, sizeof(char) * 20, "%g %s", humanSize, sizeNames[i]);

    return result;
}

