// Source - https://stackoverflow.com/a/79898596
// Posted by Wiimm
// Retrieved 2026-05-18, License - CC BY-SA 4.0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

typedef unsigned long long u64;
typedef const char * ccp;

typedef struct MemoryStatus_t
{
    u64 total;    // Total usable RAM.
    u64 free;     // The sum of LowFree + HighFree.
    u64 avail;    // An estimate of available memory for starting new applications.
    u64 buffers;  // Relatively temporary storage.
    u64 cached;   // In-memory cache for files read from the disk.
    u64 used;     // = total - free - buffers - cached;
}
MemoryStatus_t;

MemoryStatus_t GetMemoryStatus(void)
{
    MemoryStatus_t mem = {0};

    FILE *f = fopen("/proc/meminfo","r");
    if (f)
    {
        char buf[200];
        int count = 5; // abort, if all 5 params scanned

        while ( count > 0 && fgets(buf,sizeof(buf),f) )
        {
            ccp colon = strchr(buf,':');
            if (colon)
            {
                u64 num = strtoul(colon+1,0,10) * 1024;
                switch (colon-buf)
                {
                 case 6:
                    if (!memcmp(buf,"Cached",6)) { count--; mem.cached = num; break; }
                    break;

                 case 7:
                    if (!memcmp(buf,"MemFree",7)) { count--; mem.free = num; break; }
                    if (!memcmp(buf,"Buffers",7)) { count--; mem.buffers = num; break; }
                    break;

                 case 8:
                    if (!memcmp(buf,"MemTotal",8)) { count--; mem.total = num; break; }
                    break;

                 case 12:
                    if (!memcmp(buf,"MemAvailable",12)) { count--; mem.avail = num; break; }
                    break;
                }
            }
        }
        fclose(f);
        mem.used = mem.total - mem.free - mem.buffers - mem.cached;
    }
    return mem;
}

char *humanMemorySize(uint64_t bytes) {
    char *result = (char *) malloc(sizeof(char) * 20);
    char *sizeNames[] = { "B", "KB", "MB", "GB" };

    uint64_t i = (uint64_t) floor(log(bytes) / log(1024));
    double humanSize = bytes / pow(1024, i);
    snprintf(result, sizeof(char) * 20, "%.04g %s", humanSize, sizeNames[i]);

    return result;
}


int main(void)
{
    MemoryStatus_t ms = GetMemoryStatus();
    printf("tot=%s, free=%s, avail=%s, used=%s, buf=%s, cache=%s\n",
                humanMemorySize(ms.total), 
                humanMemorySize(ms.free),
                humanMemorySize(ms.avail),
                humanMemorySize(ms.used),
                humanMemorySize(ms.buffers),
                humanMemorySize(ms.cached));
    return 0;
}

