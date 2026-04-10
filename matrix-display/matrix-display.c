#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "custom_font.h"

const int ASCII_WIDTH = 6; // larghezza singolo carattere: max 1 byte (0-7)
const int ASCII_HEIGHT= 7; // altezza singolo caratte fissa a 7 pixel (1-7)

//messaggio da visualizzare
//char* msg = "0123456789";
char* msg = "0123456789 - Stai ascoltando : DEPECHE MODE - Enjoy The Silence (dall'album: Violator). Enjoy listening!";

#define gridSpacing 4 // consigliato 4 / 8 / 12 / 16
#define WIDTH gridSpacing*COLS
#define HEIGHT gridSpacing*ROWS

const int ROWS=11;  // 1 riga = ROWS*gridSpacing
const int COLS=326; //TODO: adattare in base al vaolore di CELL SIZE 

const bool debug = false; // visualizza info di debug si / no (sia a video che in console)
bool pausa = false; // flag per mettete in pausa lo scorrimento con SPACEBAR

// numero di caratteri per riga visualizzzabili in base alle colonne e alla
// grandezza del "pixel" (gridSpacing)
// questo valore  definisce anche quanti spazi mettere prima e dopo
// il messaggio per evitare segmentation fault accedendo ai vari caratteri che
// compongono il messaggio
int max_char = (gridSpacing*COLS) / (gridSpacing*ASCII_WIDTH); 

// NORD colors
#define FG_COLOR CLITERAL(Color){ 236, 239, 244, 232}
#define BG_COLOR CLITERAL(Color){67, 76, 94, 232}
#define GRID_COLOR CLITERAL(Color){59, 66, 82, 232} 

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
            for (int h = 0; h < (GetScreenHeight()/gridSpacing) + 1; h++)
            {
               if (debug) DrawText(TextFormat("%02i", h*gridSpacing), 4, h*gridSpacing - 4, 10, SKYBLUE);
               DrawLine(0, h*gridSpacing, GetScreenWidth(), h*gridSpacing, GRID_COLOR);
            }
            for (int v = 0; v < (GetScreenWidth()/gridSpacing) + 1; v++)
            {
               if (debug) DrawText(TextFormat("%02i", v*gridSpacing), v*gridSpacing - 10, 4, 10, SKYBLUE);
               DrawLine(v*gridSpacing, 0, v*gridSpacing, GetScreenHeight(), GRID_COLOR);
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
    //int posX = row * gridSpacing;
    int posY = row * gridSpacing;
            for (int y=0; y<ASCII_HEIGHT; y++) // scansiona le 7 linee HEX che formano altezza carattere
            {
            HexToBin(TableFont[pos][y],byte);
            int posX = col * gridSpacing;
            for(int i=ASCII_WIDTH -1; i>-1 ; i--)
                {
                drawRectangleRounded(posX,posY,gridSpacing,gridSpacing, byte[i] ? FG_COLOR : BLANK);          
                posX += gridSpacing;
                }
            posY += gridSpacing;
            }   
}

void drawString(int col, int row, char str[])
{
    for (int pos=0; pos < strlen(str); pos++)
      { 
        drawLetter(col,row,str[pos]);
        // dopo aver disegnato la prima lettera, spostati alla successiva
        col += ASCII_WIDTH; // spostati alla lettera successiva
      }
}

// funzione per estrarre substringa da stringa principale
char* substring(const char* str, size_t start, size_t len)
{
  if (str == 0 || strlen(str) == 0 || strlen(str) < start || strlen(str) < (start+len)) return 0;
  char* result = strndup(str + start, len);
  return result;
}

// funzione per creare stringa di tot spazi (o caraattere a piacimento)
char* creaSPAZI(int N) {
    if (N <= 0) return NULL;
    char *str = malloc(N);
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
    char *result = malloc(strlen(spazi) + strlen(msg) + strlen(spazi));

    strcpy(result, spazi);
    strcat(result, msg);
    strcat(result, spazi);
    //---------------------------------------------------------------------
    size_t start = 0;
    size_t end = max_char; //visualizza sempre [max_char] per volta! attiva debug per vedere come funzionaae
    int len=strlen(result);

    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        if (IsKeyPressed(KEY_SPACE)) pausa = !pausa;
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
                    drawString(1,2,substr);
                    if (debug) printf("%s - %li,%li\n", substr,start,end);
                    start++;
                    //loop continuo: alla fine della string riparti da zero.
                    if (start > (len - max_char)) start=0;
                }
                else
                {
                    char* substr = substring(result, start, end);
                    drawString(1,2,substr);
                }
            }
            // draw "pixels" grid as last step
            drawGrid (COLS,ROWS,GRID_COLOR);

            //draw window border
            DrawRectangleLines (1,1,WIDTH-1, HEIGHT-1, GRID_COLOR);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    free(result);
    free(spazi);
    CloseWindow();
    return 0;
}

