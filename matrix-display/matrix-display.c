// TODO
// bug carattere iniziale se dimensione 8x8 (es  6x8 non lo fa)
// todo: caricare stringa custom da linea di comando
// altro parametro  da caricate opacita sfondo 0-255

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define TOOL_NAME               "Matrix Display"
#define TOOL_SHORT_NAME         "MatrixDisp"
#define TOOL_VERSION            "2.0.0"

//#include "custom_font.h" // load default FontTable
int TableFont[128][8] = {};

// valore W e H da adattare in base al fontset creato con dotchar-editor (max. 8x8)
const int ASCII_WIDTH = 6; 
const int ASCII_HEIGHT= 8; 


//messaggio da visualizzare
//char *msg = "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char *msg = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
const int ROWS=11;  // 1 riga = ROWS*dotSize
const int COLS=328 ; //TODO: adattare in base al vaolore di: dotSize
#define dotSize 4 // dot size in pixel : consigliato 4 / 8 / 12 / 16

#define WIDTH dotSize*COLS
#define HEIGHT dotSize*ROWS

const bool debug = false; // visualizza info di debug si / no (sia a video che in console)
bool pausa = false; // flag per mettete in pausa lo scorrimento con SPACEBAR

// numero di caratteri per riga visualizzzabili in base alle colonne e alla
// grandezza del "pixel" (dotSize)
// questo valore  definisce anche quanti spazi mettere prima e dopo
// il messaggio per evitare segmentation fault accedendo ai vari caratteri che
// compongono il messaggio
int max_char = (dotSize*COLS) / (dotSize*ASCII_WIDTH); 

// NORD colors
#define FG_COLOR CLITERAL(Color){ 236, 239, 244, 255}
#define BG_COLOR CLITERAL(Color){67, 76, 94, 232}
#define GRID_COLOR CLITERAL(Color){59, 66, 82, 0} 

void drawRectangleRounded (int x, int y, int w, int h, Color color)  
{
  Rectangle  rect = { x, y, w, h};   // toplx, toply, width, height
  float radius = 0; // no radius
  int   segs   = 0; // non segments
  DrawRectangleRounded ( rect, radius, segs, color );
}

void drawGrid(int cols, int rows, Color color)
// da fare : se cell size minore di 10 non disegnare e esci subito
{
            for (int h = 0; h < (GetScreenHeight()/dotSize) + 1; h++)
            {
               if (debug) DrawText(TextFormat("%02i", h), 4, 4+ h*dotSize, 10, SKYBLUE);
               DrawLine(0, h*dotSize, GetScreenWidth(), h*dotSize, GRID_COLOR);
            }
            for (int v = 0; v < (GetScreenWidth()/dotSize) + 1; v++)
            {
               if (debug) DrawText(TextFormat("%02i", v), 4 + v*dotSize , 4, 10, SKYBLUE);
               DrawLine(v*dotSize, 0, v*dotSize, GetScreenHeight(), GRID_COLOR);
            }
}

// Source - https://stackoverflow.com/a/16169029
// Posted by Manel, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-02, License - CC BY-SA 3.0

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
    //int posX = row * dotSize;
    int posY = row * dotSize;
            for (int y=0; y<ASCII_HEIGHT; y++) // scansiona le 7 linee HEX che formano altezza carattere
            {
            HexToBin(TableFont[pos][y],byte);
            int posX = col * dotSize;
            for(int i=ASCII_WIDTH - 1; i>-1 ; i--)
                {
                drawRectangleRounded(posX,posY,dotSize -1 ,dotSize -1 , byte[i] ? FG_COLOR : BLANK);          
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

// funzione per estrarre substringa da stringa principale
char *substring(char *str, size_t start, size_t len)
{
  if (str == 0 || strlen(str) == 0 || strlen(str) < start || strlen(str) < (start+len)) return 0;
  char *stringa = strndup(str + start , len);
  return stringa;
}

// funzione per creare stringa di tot spazi (o caraattere a piacimento)
char *creaSPAZI(int N) {
    if (N <= 0) return NULL;
    // char *str = malloc(N); //se si usa N carattere strano che appare con ASCII_WIDTH=8 .. solo con questo valore 
    char *str = malloc(256); // imposto allora un valore fisso 
    if (str == NULL) return NULL;
    memset(str, ' ', N);
    return str;
}

int main (int argc, char *argv[])
{

    // gestione parametri da linea di comando
    //int opt = getopt(argc, argv, "m");

    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(WIDTH, HEIGHT, "Matrix Display");
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - WIDTH/2, GetMonitorHeight(0) / 2 - HEIGHT/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowState(FLAG_WINDOW_TOPMOST);
    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);  

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(10);

    // crea stringa spazi e appendila prima e dopo il messaggio originale
    char *spazi = creaSPAZI(max_char);
        if (spazi == NULL) {
            printf("Errore nella creazione della stringa\n");
            return 1;
            }
    // creo stringa finale da visualizzare (result) con spazi prima e dopo
    //char *result = malloc( strlen(spazi) + strlen(msg) + strlen(spazi) );
    char *result = malloc(512); // il size di malloc qui genera errori combinazione dot size colonne
        strcpy(result, spazi);
        strcat(result, msg);
        strcat(result, spazi);
    //---------------------------------------------------------------------
    size_t start = 0;
    size_t end = max_char; //visualizza sempre [max_char] per volta! attiva debug per vedere come funzionaae
    int len=strlen(result);

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) pausa = !pausa;

        // Load at runtime, custom font chars from font.data file!
        // se si vuole cambiare il disegno dei caratteri e' sufficiente aprire
        // il file font.data con l' altro programma dotchar-editor!, modificarlo,
        // salvarlo e ricopiarlo qui!  :-)
        // questo sovrascrive la tabella caratteri di default definita nel file: include.h
            FILE *fLoad = fopen("data.fnt", "rb"); 
            if (fLoad == NULL) {
                printf("File [data.fnt] non trovato!\n");
            return 1;
            }
            fread(TableFont, sizeof(char), sizeof(TableFont), fLoad);
            fclose(fLoad);

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------

        BeginTextureMode(target);
            ClearBackground(BLANK);
        EndTextureMode();

        BeginDrawing();
            ClearBackground (BLANK);
            // draw round rectangle as "fake" background with some opacity
            drawRectangleRounded(0,0,WIDTH, HEIGHT,BG_COLOR);
            // eanble debug info
            if (debug) DrawFPS(10, 10);
            // scroll string using substring
            if (len > max_char)
            {
                if (!pausa)
                {
                    char* substr = substring(result, start, end);
                    drawString(2,2,substr);
                     if (debug) printf("%s\n", result);
                    if (debug) printf("%s-%li,%li\n", substr,start,end);
                    start++;
                    //loop continuo: alla fine della string riparti da zero.
                    if (start > (len - max_char)) start=0;
                }
                else
                {
                    char* substr = substring(result, start, end);
                    drawString(2,2,substr);
                }
            }
            // draw "pixels" grid as last step
            //drawGrid (COLS,ROWS,GRID_COLOR);

            //draw window border
            //DrawRectangleLines (1,1,WIDTH-1, HEIGHT-1, GRID_COLOR);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    free(result);
    free(spazi);
    CloseWindow();
    return 0;
}

