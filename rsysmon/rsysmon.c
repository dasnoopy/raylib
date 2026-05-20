// da fare:
// se scheda rete is Down mac address e' fake : scrivere qlc diverso dal mac address che viene 
// visulizzato!

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
#define TOOL_VERSION            "0.2.0"

#include "font.h" // load default FontTable
//int TableFont[128][8] = {};

// valore W e H da adattare in base al fontset creato con dotchar-editor (max. 8x8)
const int ASCII_WIDTH = 6; 
const int ASCII_HEIGHT= 8; 

const int ROWS=300;  // 1 riga = ROWS*dotSize
const int COLS=180 ; //TODO: adattare in base al vaolore di: dotSize
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

void drawGrid(int cols, int rows, Color color)
// da fare : se cell size minore di 10 non disegnare e esci subito
{
            for (int h = 0; h < (GetScreenHeight()/dotSize) + 1; h++)
               DrawLine(0, h*dotSize, GetScreenWidth(), h*dotSize, color);
            for (int v = 0; v < (GetScreenWidth()/dotSize) + 1; v++)
               DrawLine(v*dotSize, 0, v*dotSize, GetScreenHeight(), color);

}

// converti OxFF in binario es:  0xC7 -> 11000111
void HexToBin(char hex_number, char* bit_char) {
    int max = 128;
     for(int i = 7 ; i >-1 ; i--)
    {
        bit_char [i] = (hex_number & max ) ? 1 : 0;
        max >>=1;
    }
 }

void drawLetter(int col,int row,int ASCII_CODE)
{ 
    int pos = ASCII_CODE ;
    char byte[8]={0,0,0,0,0,0,0,0};
    int posY = row * dotSize;
            for (int y=0; y<ASCII_HEIGHT; y++) // scansiona le 7 linee HEX che formano altezza carattere
            {
            HexToBin(TableFont[pos][y],byte);
            int posX = col * dotSize;
            for (int i=ASCII_WIDTH - 1; i>-1 ; i--) {
                //DrawRectangle(posX,posY,dotSize -1,dotSize -1, byte[i] ? FG_COLOR : BLANK);
                DrawRectangle(posX,posY,dotSize-1,dotSize-1, byte[i] ? FG_COLOR : BLANK);
                posX += dotSize;
                }
            posY += dotSize;
            }   
}

void drawString(int col, int row, char *str)
{

    for (int pos=0; pos < strlen(str); pos++)
      { 
        drawLetter(col,row,str[pos]);
        // dopo aver disegnato la prima lettera, spostati alla successiva
        col += ASCII_WIDTH; // spostati alla lettera successiva
      }
       
}

void get_dateTime (int col, int row)
{
        time_t timer;
        char buffer_date[26];
        char buffer_time[26];
        struct tm* tm_info;

        time(&timer);
        tm_info = localtime(&timer);
           
        strftime(buffer_date, 26, "%d/%m/%Y", tm_info);
        drawString(col,row,buffer_date);

        strftime(buffer_time, 26, "%H:%M:%S", tm_info);
        drawString(col+120,row,buffer_time);
}


void get_uptime (int col, int row)
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

    drawString(col,row, uptime_str);
}

void get_uname (int col, int row)
{
   struct utsname buffer;

   errno = 0;
   if (uname(&buffer) < 0) {
      perror("uname");
      exit(EXIT_FAILURE);
   }

   drawString(col, row, buffer.nodename);
   drawString(col, row+10, buffer.sysname);
   drawString(col+36, row+10, buffer.release);
   //drawString(col, row+10,  buffer.version);
   //drawString(col, row+30, buffer.machine);

   #ifdef _GNU_SOURCE
      printf("domain name = %s\n", buffer.domainname);
   #endif
}

void get_ipAddress (int col, int row, char *iface)
{
    int fd;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) != -1) {
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , iface , IFNAMSIZ - 1);

         /* grab flags associated with this interface */
        ioctl(fd, SIOCGIFFLAGS, &ifr);
        drawString(col,row, "netdev   :");
        drawString(col+66,row, ifr.ifr_name);
        
        if (ifr.ifr_flags & IFF_UP) drawString(col,row +10 ,"status   : UP");
        else drawString(col,row +10,"status   : DOWN");

        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
    }

    char buffer[64]; 
    strcpy(buffer,"ip addr. : ");
    strcat(buffer, inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr));

    drawString(col, row + 20, buffer);
}

void get_MACaddr (int col, int row, char *iface)
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
        drawString(col, row, buffer);
}


void get_CPULoad (int col, int row)
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

    drawString(col,row,FileBuffer);
}

void get_MEMinfo (int col, int row)
{
    char buffer[64]; 
    MemoryStatus_t ms = GetMemoryStatus();

    strcpy(buffer,"Total mem: ");
    drawString(col, row, strcat(buffer, humanMemorySize(ms.total)));

    strcpy(buffer,"Used  mem: ");
    drawString(col, row + 10, strcat(buffer, humanMemorySize(ms.used)));

    strcpy(buffer,"Free  mem: ");
    drawString(col, row + 20, strcat(buffer, humanMemorySize(ms.free)));
        
    strcpy(buffer,"Avail mem: ");
    drawString(col, row + 30, strcat(buffer, humanMemorySize(ms.avail)));
    
    strcpy(buffer,"Buff. mem: ");
    drawString(col, row + 40, strcat(buffer, humanMemorySize(ms.buffers)));

    strcpy(buffer,"Cache mem: ");
    drawString(col, row + 50, strcat(buffer, humanMemorySize(ms.cached)));
}


void get_CPUtemp(int col, int row) { // sub function used to print CPU temperature
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
        "CPU temp.: %.f`",CPU_temp);
        drawString(col, row,  str_temp);
}

int main (int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(WIDTH, HEIGHT, "System Info");
    // SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    // center window on the screen
    //SetWindowPosition(GetMonitorWidth(0) / 2 - WIDTH/2, GetMonitorHeight(0) / 2 - HEIGHT/2); 
    SetWindowPosition(32,96); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    //SetWindowState(FLAG_WINDOW_TOPMOST);
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
        int count=0;
        char PublicIP[128];
        while(count<1) {
            FILE *fp = popen("curl --fail --ipv4 https://ifconfig.me", "r");
                if (fp == NULL) {
                    perror("popen failed: is CURL installed on your system?");
                }
            fgets(PublicIP, sizeof(PublicIP), fp);
            pclose(fp);
            count++;
        }

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------


        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------

        BeginDrawing();
            ClearBackground (BLANK);
            // draw round rectangle as "fake" background with some opacity
            drawRectangleRounded(0,0,WIDTH, HEIGHT,BG_COLOR);

            get_dateTime(5,5); // col = x, row=y
            get_uname(30,20);

            get_uptime(30,40);
            get_ipAddress(5,70,"eth0");
            get_MACaddr(5,100,"eth0");
            get_ipAddress(5,116,"wlan0");
            get_MACaddr(5,146,"wlan0");
            get_CPULoad(5,162);
            get_MEMinfo(5,190);
            drawString(5,55,"Public IP:");
            drawString(71,55,PublicIP); // Public IP address
            get_CPUtemp(5,172);

        EndDrawing();
    }
    CloseWindow();
    return 0;
}

