/*******************************************************************************************
*
*   raylib System Monitor (aka rSysMon)
*   A simple system monitor for linux made with raylib
*
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************/

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdint.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/kernel.h>   
#include <sys/sysinfo.h>
#include <errno.h>
#include <sys/utsname.h>
// raygui integration
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

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

#define TOOL_NAME               "System Monitor"
#define TOOL_SHORT_NAME         "rSysMon"
#define TOOL_VERSION            "0.8.0"

// valore W e H da adattare in base al fontset creato con dotchar-editor (max. 8x8)
const int ASCII_WIDTH = 6; 
const int ASCII_HEIGHT= 8; 

const int ROWS=276;  // 1 riga = ROWS*dotSize
const int COLS=178 ; //TODO: adattare in base al vaolore di: dotSize
#define dotSize 2 // dot size in pixel : consigliato 4 / 8 / 12 / 16

#define WIDTH dotSize*COLS
#define HEIGHT dotSize*ROWS

// numero di caratteri per riga visualizzzabili in base alle colonne e alla
// grandezza del "pixel" (dotSize)
// questo valore  definisce anche quanti spazi mettere prima e dopo
// il messaggio per evitare segmentation fault accedendo ai vari caratteri che
// compongono il messaggio
int max_char = (dotSize*COLS) / (dotSize*ASCII_WIDTH); 

// NORD colors
#define BG_COLOR CLITERAL(Color){41, 46, 57, 232}
#define FG_COLOR CLITERAL(Color){216, 222, 233, 255}
#define BACK_COLOR CLITERAL(Color){59,66,82,232}
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


void drawRectangleRounded (int x, int y, int w, int h, Color color)  
{
  Rectangle  rect = { x, y, w, h};   // toplx, toply, width, height
  float radius = 0.072f; // no radius
  int   segs   = 8; // non segments
  DrawRectangleRounded ( rect, radius, segs, color );
}


void get_dateTime (int col, int row, Color color);

void get_uptime (int col, int row, Color color)
{
    struct sysinfo info;
    char uptime_str[64];

    sysinfo(&info);

    snprintf(
        uptime_str,
        sizeof(uptime_str),
        "uptime  : %02ldh %02ldm",
        info.uptime / 3600,
        (info.uptime % 3600) / 60
    );
    DrawText(TextFormat("%s",uptime_str),col,row,20,color);

}

void get_uname (int col, int row, Color color)
{
   struct utsname buffer;

   errno = 0;
   if (uname(&buffer) < 0) {
      perror("uname");
      exit(EXIT_FAILURE);
   }

   DrawText(TextFormat("%s",buffer.nodename),col,row,20,color);
   DrawText(TextFormat("%s",buffer.sysname),col,row+20,20,color);
   DrawText(TextFormat("%s",buffer.release),col+80,row+20,20,color);
   //drawString(col, row+10,  buffer.version);
   //drawString(col, row+30, buffer.machine);

   #ifdef _GNU_SOURCE
      printf("domain name = %s\n", buffer.domainname);
   #endif
}

void get_ipAddress (int col, int row, char *iface, Color color)
{
    int fd;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) != -1) {
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , iface , IFNAMSIZ - 1);

         /* grab flags associated with this interface */
        ioctl(fd, SIOCGIFFLAGS, &ifr);
        DrawText(TextFormat("netdev    : %s",ifr.ifr_name),col,row,20,color);
        
        if (ifr.ifr_flags & IFF_UP) DrawText("status    : UP",col,row+20,20, color);
        else DrawText("status    : DOWN",col,row+20,20, color);

        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
    }

    char buffer[64]; 
    strcpy(buffer,"ip addr.   : ");
    strcat(buffer, inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr));

    DrawText(TextFormat("%s",buffer),col,row+40,20,color);
}

void get_MACaddr (int col, int row, char *iface, Color color)
{
    int fd;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    unsigned char *mac;
     
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) != -1) {

        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
        ioctl(fd, SIOCGIFHWADDR, &ifr);
        close(fd);
    }

    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    char buffer[64];
    snprintf(
        buffer,
        sizeof(buffer),
        "hw addr. : %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n" , 
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
        DrawText(TextFormat("%s",buffer),col,row,20,color);
}


void get_CPULoad (int col, int row, Color color)
{
    char FileBuffer[1024];
    float ld1,ld2,ld3;

    FILE *FileHandler = fopen("/proc/loadavg", "r");
        if (FileHandler == NULL) {
            printf("Errore apertura file!\n");
        }
    fgets(FileBuffer, sizeof(FileBuffer) - 1, FileHandler);
    sscanf(FileBuffer, "%f %f %f", &ld1, &ld2, &ld3);
    fclose(FileHandler);

    snprintf(
        FileBuffer,
        sizeof(FileBuffer),
        "CPU load : %0.2f %0.2f %0.2f",
        ld1,ld2,ld3 );

       DrawText(TextFormat("%s",FileBuffer),col,row,20,color);
}

void get_MEMinfo (int col, int row, Color color)
{
    char buffer[64]; 
    MemoryStatus_t ms = GetMemoryStatus();

    strcpy(buffer,"total mem: ");
    DrawText(TextFormat("%s",strcat(buffer, humanMemorySize(ms.total))),col, row,20,color);

    strcpy(buffer,"used  mem: ");
    DrawText(TextFormat("%s",strcat(buffer, humanMemorySize(ms.used))),col, row+20,20,color);

    strcpy(buffer,"free  mem: ");
     DrawText(TextFormat("%s",strcat(buffer, humanMemorySize(ms.free))),col, row+40,20,color);
        
    strcpy(buffer,"avail mem: ");
    DrawText(TextFormat("%s",strcat(buffer, humanMemorySize(ms.avail))),col, row+60,20,color);
    
    strcpy(buffer,"buff. mem: ");
    DrawText(TextFormat("%s",strcat(buffer, humanMemorySize(ms.buffers))),col, row+80,20,color);

    strcpy(buffer,"cache mem: ");
     DrawText(TextFormat("%s",strcat(buffer, humanMemorySize(ms.cached))),col, row+100,20,color);
}


void get_CPUtemp(int col, int row, Color color) { // sub function used to print CPU temperature
    FILE *fp;
    char str_temp[32];
    float CPU_temp;
    // CPU temperature data is stored in this directory.
    fp=fopen("/sys/class/thermal/thermal_zone0/temp","r");
    fgets(str_temp,15,fp);      // read file temp
    CPU_temp = atof(str_temp)/1000.0;   // convert to Celsius degrees
    //printf("CPU's temperature : %.2f \n",CPU_temp);
    fclose(fp);

        snprintf(
        str_temp,
        sizeof(str_temp),
        "CPU temp.: %.f°",CPU_temp);
         DrawText(TextFormat("%s",str_temp),col,row,20,color);
}

int main (int argc, char *argv[])
{
    //nascondi finestra durante caricamento iniziale
    SetWindowState(FLAG_WINDOW_HIDDEN);
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(WIDTH, HEIGHT, "System Info");


    SetWindowState(FLAG_WINDOW_UNDECORATED);
    //SetWindowState(FLAG_WINDOW_TOPMOST);
    SetExitKey(KEY_Q);       // set Q as exit key
    // center window on the screen
    // SetWindowPosition(GetMonitorWidth(0) / 2 - WIDTH/2, GetMonitorHeight(0) / 2 - HEIGHT/2); 
    SetWindowPosition(32,96); 


    // load TTF font with better antialiasing
    Font font = LoadFontEx("assets/OSD-Mono.ttf", 16, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    GuiSetIconScale(1);

    RenderTexture target = LoadRenderTexture(WIDTH, HEIGHT);
    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(5);

        // Load at runtime, un custom font dal file font.bin!
        // questo sovrascrive la tabella caratteri di default definita nel file: include.h
            // FILE *fLoad = fopen("rsysmon.fnt", "rb"); 
            // if (fLoad == NULL) {
            //     printf("File non trovato!\n");
            // return 1;
            // }
            // fread(TableFont, sizeof(char), sizeof(TableFont), fLoad);
            // fclose(fLoad);

        // Get public IP address using CURL
        char PublicIP[128];
        FILE *fp = popen("curl --fail --ipv4 https://ifconfig.me", "r");
                if (fp == NULL) {
                    perror("popen failed: is CURL installed on your system?");
                }
            fgets(PublicIP, sizeof(PublicIP), fp);
            pclose(fp);

    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------


        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
    BeginTextureMode(target);
        ClearBackground(BG_COLOR);
    EndTextureMode();


        BeginDrawing();
            ClearBackground (BLANK);
            // draw round rectangle as "fake" background with some opacity
            drawRectangleRounded(0,0,WIDTH, HEIGHT,BG_COLOR);
            DrawRectangle(0,30,WIDTH,HEIGHT-60,BACK_COLOR);
            for (int x = 1; x < WIDTH; x+=18) 
                DrawLine(x,34,x,HEIGHT-30,DARKGRAY);
            for (int y = 30; y < HEIGHT-27; y+=24)
                DrawLine(0,y,WIDTH,y,DARKGRAY);
            DrawRectangleLines(0,30,WIDTH,HEIGHT-60,DARKGRAY);

            get_dateTime(10,5, SKYBLUE); // col = x, row=y

            get_uname(10,40, LIGHTGRAY);
            get_uptime(10,80,RAYWHITE);
            DrawText(TextFormat("Public IP: %s", PublicIP),10,110,20,LIGHTGRAY);

            get_ipAddress(10,150,"eth0",RAYWHITE);
            get_MACaddr(10,210,"eth0",LIGHTGRAY);

            get_ipAddress(10,240,"wlan0",RAYWHITE);
            get_MACaddr(10,300,"wlan0",LIGHTGRAY);

            get_CPULoad(10,330,RAYWHITE);
            get_CPUtemp(10,350,LIGHTGRAY);

            get_MEMinfo(10,380,RAYWHITE);

            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 16, HEIGHT-20, 10, FG_COLOR); 
            DrawText(TextFormat("version %s", TOOL_VERSION), WIDTH-80, HEIGHT-20, 10, GRAY); 

        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadFont(font);
    CloseWindow();
    return 0;
}

void get_dateTime (int col, int row, Color color)
{
        time_t timer;
        char buffer_date[26];
        char buffer_time[26];
        struct tm* tm_info;

        time(&timer);
        tm_info = localtime(&timer);
           
        strftime(buffer_date, 26, "%d/%m/%Y", tm_info);
        DrawTextEx(font,TextFormat("%s",buffer_date),(Vector2){col,row},20,0,color);

        strftime(buffer_time, 26, "%H:%M:%S", tm_info);
        DrawText(TextFormat("%s",buffer_date),col+214,row,20,color);
}