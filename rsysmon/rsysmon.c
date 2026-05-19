// da fare:
// se scheda rete is Down mac address e' fake : scrive qlc

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>

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



#define TOOL_NAME               "System Monitor"
#define TOOL_SHORT_NAME         "rSysMon"
#define TOOL_VERSION            "0.2.0"

#include "custom_font.h" // load default FontTable
//int TableFont[128][8] = {};

// valore W e H da adattare in base al fontset creato con dotchar-editor (max. 8x8)
const int ASCII_WIDTH = 6; 
const int ASCII_HEIGHT= 8; 

const int ROWS=240;  // 1 riga = ROWS*dotSize
const int COLS=174 ; //TODO: adattare in base al vaolore di: dotSize
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
#define BG_COLOR CLITERAL(Color){ 55, 65, 70, 232}
#define FG_COLOR CLITERAL(Color){133, 208, 211, 255}

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
                DrawRectangle(posX,posY,dotSize,dotSize, byte[i] ? WHITE : BLANK);
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

void get_dateTime(int row, int col)
{
        time_t timer;
        char buffer_date[26];
        char buffer_time[26];
        struct tm* tm_info;

        time(&timer);
        tm_info = localtime(&timer);
           
        strftime(buffer_date, 26, "Date  : %d/%m/%Y", tm_info);
        drawString(col,row,buffer_date);
        strftime(buffer_time, 26, "Time  : %H:%M:%S", tm_info);

        drawString(col,row+10,buffer_time);
}


void get_uptime(int row, int col)
{
    struct sysinfo info;
    char uptime_str[64];

    sysinfo(&info);

    snprintf(
        uptime_str,
        sizeof(uptime_str),
        "Uptime: %02ldh %02ldm",
        info.uptime / 3600,
        (info.uptime % 3600) / 60
    );

    drawString(row,col, uptime_str);
}

void get_uname(int row, int col)
{
   struct utsname buffer;

   errno = 0;
   if (uname(&buffer) < 0) {
      perror("uname");
      exit(EXIT_FAILURE);
   }

   drawString(row, col, buffer.nodename);
   drawString(row, col+10, buffer.sysname);
   drawString(row+38, col+10, buffer.release);
   //drawString(row, col+10,  buffer.version);
   //drawString(row, col+30, buffer.machine);

   #ifdef _GNU_SOURCE
      printf("domain name = %s\n", buffer.domainname);
   #endif
}

void get_ipAddress(int row, int col, char *iface)
{
    int fd;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) != -1) {
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , iface , IFNAMSIZ - 1);

         /* grab flags associated with this interface */
        ioctl(fd, SIOCGIFFLAGS, &ifr);
        drawString(row,col, "netdev  :");
        drawString(row+60,col, ifr.ifr_name);
        
        if (ifr.ifr_flags & IFF_UP) drawString(row,col +10 ,"status  : UP");
        else drawString(row,col +10,"status  : DOWN");

        ioctl(fd, SIOCGIFADDR, &ifr);
        close(fd);
    }

    char buffer[64]; 
    strcpy(buffer,"ip addr : ");
    strcat(buffer, inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr));

    drawString(row, col + 20, buffer);
}

void get_MACaddr(int row, int col, char *iface)
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
        "Mac addr: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n" , 
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );
        drawString(row, col, buffer);
}


void get_CPULoad(int row, int col)
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
        "Load  : %0.2f %0.2f %0.2f",
        ld1,ld2,ld3 );

    drawString(row,col,FileBuffer);
}

int main (int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(WIDTH, HEIGHT, "Matrix Display");
    // SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    // center window on the screen
    // SetWindowPosition(GetMonitorWidth(0) / 2 - WIDTH/2, GetMonitorHeight(0) / 2 - HEIGHT/2); 
    SetWindowPosition(32,104); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    //SetWindowState(FLAG_WINDOW_TOPMOST);
    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(10);

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        // Load at runtime, un custom font dal file font.bin!
        // questo sovrascrive la tabella caratteri di default definita nel file: include.h
            // FILE *fLoad = fopen("rsysmon.fnt", "rb"); 
            // if (fLoad == NULL) {
            //     printf("File non trovato!\n");
            // return 1;
            // }
            // fread(TableFont, sizeof(char), sizeof(TableFont), fLoad);
            // fclose(fLoad);

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------

        BeginDrawing();
            ClearBackground (BLANK);
            // draw round rectangle as "fake" background with some opacity
            drawRectangleRounded(0,0,WIDTH, HEIGHT,BG_COLOR);

            get_dateTime(5,5); // row, col
            get_uname(5,35);
            get_uptime(5,55);
            get_ipAddress(5,75,"eth0");
            get_MACaddr(5,105,"eth0");
            get_ipAddress(5,125,"wlan0");
            get_MACaddr(5,155,"wlan0");
            get_CPULoad(5,220);
            //get_MEMinfo(5,95);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}

