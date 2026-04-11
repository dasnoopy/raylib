/*******************************************************************************************
*
*   Interactive Bin to HEX DEC OCTAL converter
* 
*   A simple app to learn C using raylib library
* 
*  CHANGELOG:
* 
*   v.1.0: first release.
*   v 1.1: some visual improvements.
* 
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
*
*   TODO LIST POSSIBLE IMPROVEMENTS:
*       - convertire arraay da bi a mono dimensionali
*       - stringa biinaria cambiare sfondo ogni 4 bit per maggiore chiarezza
*
*******************************************************************************************/

#define TOOL_NAME               "Binary Converter"
#define TOOL_SHORT_NAME         "b2c"
#define TOOL_VERSION            "1.0"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

// raygui integration
#define RAYGUI_IMPLEMENTATION
//#define RAYGUI_CUSTOM_ICONS     // Custom icons set required 
//#include "gui_iconset.h"        // Custom icons set provided, generated with rGuiIcons tool
#include "raygui.h"

const int screenWidth = 820;
const int screenHeight = 280;

 // initial X,Y coordinates for variuos interface elements
Vector2 grid_bin_XY = { 24, 96 }; // x, y devono essere uguale o multiplo di gridSpacing ....
Vector2 grid_hex_XY = { 72, 200 };

#define gridSpacing               48
#define MAX_GRID_BIN_X            16
#define MAX_GRID_BIN_Y            1
#define MAX_GRID_HEX_X            4
#define MAX_GRID_HEX_Y MAX_GRID_BIN_Y 

// matrici
int matrice[MAX_GRID_BIN_X][MAX_GRID_BIN_Y];
char hex[MAX_GRID_HEX_X][MAX_GRID_BIN_Y];


// NORD colors
#define BG_COLOR CLITERAL(Color){ 59, 66, 82, 255} 
#define FG_COLOR CLITERAL(Color){ 216, 219, 224, 255}
#define GRID_COLOR CLITERAL(Color){ 176, 186, 206, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 76, 86, 106, 255} 
#define ON_COLOR CLITERAL(Color){ 208, 135, 112,255}
#define OFF_COLOR CLITERAL(Color){ 191, 97, 106,255}

// mouse and clipoard
bool mouseHoverCells = false;


//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Point struct, like Vector2 but using int
typedef struct {
    int x;
    int y;
} Point;

// Player state struct
typedef struct {
    Point cell;
    Color color;
} PlayerState;

// calcolo di potenze x^y
int potenza(int base, int esp)
{
     int res = 1;
     while (esp > 0)
     {
          res = res * base;
          esp = esp - 1;
     }
     return res;
}

// converti numero da 0-15 a esaadecimale 0-A
char charToHex(int val) {
    if (val < 10)
        return '0' + val;
    else
        return 'A' + (val - 10);
}

// 'fake' background
void drawRectangleRounded (int x, int y, int w, int h, Color color)  
{
  Rectangle  rect = { x, y, w, h};   // toplx, toply, width, height
  float radius = 0.1; // no radius
  int   segs   = 12; // non segments
  DrawRectangleRounded ( rect, radius, segs, color );
}

// Draw binary matrix grid
void drawGrids(void)
{
            
            // horizontal bin & hex lines
            for (int y = 0; y <= MAX_GRID_BIN_Y; y++) {
                DrawLine((int)grid_bin_XY.x, (int)grid_bin_XY.y + y * gridSpacing,(int)grid_bin_XY.x + MAX_GRID_BIN_X* gridSpacing, (int)grid_bin_XY.y + y*gridSpacing, GRID_COLOR);
                DrawLine((int)grid_hex_XY.x, (int)grid_hex_XY.y + y * gridSpacing,(int)grid_hex_XY.x + MAX_GRID_HEX_X* gridSpacing, (int)grid_hex_XY.y + y*gridSpacing, GRID_COLOR);
            }

            // vertical bin lines
            for (int x = 0; x <= MAX_GRID_BIN_X; x++)
                DrawLine((int)grid_bin_XY.x + x * gridSpacing, (int)grid_bin_XY.y,(int)grid_bin_XY.x + x * gridSpacing, (int)grid_bin_XY.y + MAX_GRID_BIN_Y*gridSpacing, GRID_COLOR);

            //vertical hex lines
            for (int x = 0; x <= MAX_GRID_HEX_X; x++)
                DrawLine((int)grid_hex_XY.x + x * gridSpacing, (int)grid_hex_XY.y,(int)grid_hex_XY.x + x * gridSpacing, (int)grid_hex_XY.y + MAX_GRID_HEX_Y*gridSpacing, GRID_COLOR);
}

// Draw hex matrix grid
void draw_hex_grid(void)
{
            // background
            for (int y = 0; y < MAX_GRID_HEX_Y; y++)
            {
                for (int x = 0; x < MAX_GRID_HEX_X; x++) { 
                    DrawRectangle((int)grid_hex_XY.x + x*gridSpacing, (int)grid_hex_XY.y + y * gridSpacing , gridSpacing-1, gridSpacing-1, GRID_BG_COLOR);  }
            }
}


// legge le righe della matrice binaria e memorizza valori HEX nella matrice esadecimale
void BinToHex (void)
{
     for (int i = 0; i < MAX_GRID_HEX_Y; i++) 
     {
        int psb = 0; //primo significative bye
        int ssb = 0;
        int tsb = 0;
        int qsb = 0;

        for (int j = 0; j < 4; j++)
            psb = (psb << 1) | matrice[j][i];

        for (int j = 4; j < 8; j++)
            ssb = (ssb << 1) | matrice[j][i];

        for (int j = 8; j < 12; j++)
            tsb = (tsb << 1) | matrice[j][i];

        for (int j = 12; j < 16; j++)
            qsb = (qsb << 1) | matrice[j][i];

        hex[0][i] = charToHex(psb);
        hex[1][i] = charToHex(ssb);
        hex[2][i] = charToHex(tsb);
        hex[3][i] = charToHex(qsb);
           // Draw text box
    }
}

//  disegna bit delle matrice in base al loro valore
void drawBinCells()
{
            for (int i = 0; i < MAX_GRID_BIN_Y; i++)
                {
                    for (int j = 0; j < MAX_GRID_BIN_X; j++)
                    {
                        // disegna sfondo cella  in base al valore 1/0
                        // se si cambia disegno qui, cambiare anche cursore nella sezione BeginDrawing
                        DrawRectangle(grid_bin_XY.x + gridSpacing*j, grid_bin_XY.y + gridSpacing*i, 
                          gridSpacing -1, 
                          gridSpacing -1, 
                          matrice[j][i] ? GRID_BG_COLOR : BG_COLOR);
                        DrawText(TextFormat("%i",matrice[j][i]), 16 + grid_bin_XY.x + gridSpacing*j, 10 + grid_bin_XY.y + gridSpacing*i, 30, GRID_COLOR);
                    }
                }   
}

// stampa valori esadecimali nella relativa griglia
void printHexValues (void)
{
      for (int i = 0; i < MAX_GRID_HEX_Y; i++)
        {
            DrawText(TextFormat("%c", hex[0][i]), grid_hex_XY.x + 12, 6 + grid_hex_XY.y + gridSpacing*i, 40, GRID_COLOR);
            DrawText(TextFormat("%c", hex[1][i]), grid_hex_XY.x + 12 + gridSpacing *1, 6 + grid_hex_XY.y + gridSpacing*i, 40, GRID_COLOR);
            DrawText(TextFormat("%c", hex[2][i]), grid_hex_XY.x + 12 + gridSpacing *2, 6 + grid_hex_XY.y + gridSpacing*i, 40, GRID_COLOR);
            DrawText(TextFormat("%c", hex[3][i]), grid_hex_XY.x + 12 + gridSpacing *3, 6 + grid_hex_XY.y + gridSpacing*i, 40, GRID_COLOR);
        }
}

void printDecOctValues(void)
{
    int decimal = 0;
    for (int i = 0; i < MAX_GRID_BIN_X; i++)
    {
        if (matrice[i][0] == 1) {decimal += potenza(2,(MAX_GRID_BIN_X-1)-i);}
    }
    // disegna e stampa la parte decimale
    DrawRectangle(grid_hex_XY.x + gridSpacing*6, grid_hex_XY.y , gridSpacing *3, gridSpacing , GRID_COLOR);
    DrawRectangle(grid_hex_XY.x + 1 + gridSpacing*6, grid_hex_XY.y +1, gridSpacing *3-2,gridSpacing -2, GRID_BG_COLOR);
    DrawText(TextFormat("%05i",decimal), 16 + grid_hex_XY.x + gridSpacing *6, grid_hex_XY.y + 6, 40, GRID_COLOR);

    // disegna e stampa la parte OTTALE
    DrawRectangle(grid_hex_XY.x + gridSpacing*11 , grid_hex_XY.y, gridSpacing *4, gridSpacing , GRID_COLOR);
    DrawRectangle (grid_hex_XY.x + 1 + gridSpacing*11, grid_hex_XY.y + 1, gridSpacing *4 -2, gridSpacing -2, GRID_BG_COLOR);
    DrawText(TextFormat("%06o",decimal), grid_hex_XY.x + gridSpacing *11 +24,  grid_hex_XY.y + 6, 40, GRID_COLOR);

}

// azzera matrice binaria e di conseguenza anche quella esadecimale, decimale e ottale
void resetMatrici()
{
    //reset matrice binaria
    for (int i = 0; i < MAX_GRID_BIN_Y; i++)
        {  for (int j = 0; j < MAX_GRID_BIN_X; j++)
                    { matrice[j][i] = 0; }
        }    
}


int main (int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(screenWidth, screenHeight, "Binary Converter");
    
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowState(FLAG_WINDOW_TOPMOST);
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);

    // UI required variables
    bool btnClearPressed = false;
    bool btnQuitPressed = false;

    // Set UI style
    // Custom GUI font loading
    // Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    // GuiLoadStyle("assets/style_genesis.rgs");
    // GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiSetIconScale(1);

    // Init current player state
    PlayerState player = { 0 };
    player.cell = (Point){ MAX_GRID_BIN_X-1, 0 };
    //player.color = SKYBLUE;

    //reset matrice binaria
    resetMatrici();

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------

        if (btnClearPressed)
        {
            resetMatrici();
            player.cell.x = MAX_GRID_BIN_X-1;
            player.cell.y = 0;
        }

        if (btnQuitPressed) break;
        
        //----------------------------------------------------------------------------------
        // Player movement logic using arrow keys
        if (IsKeyPressed(KEY_RIGHT)) player.cell.x++;
        else if (IsKeyPressed(KEY_LEFT)) player.cell.x--;
        else if (IsKeyPressed(KEY_UP)) player.cell.y--;
        else if (IsKeyPressed(KEY_DOWN)) player.cell.y++;

        // Make sure player does not go out of bounds
        if (player.cell.x < 0) player.cell.x = 0;
        else if (player.cell.x >= MAX_GRID_BIN_X) player.cell.x = MAX_GRID_BIN_X-1;
        else if (player.cell.y < 0) player.cell.y = 0 ;
        else if (player.cell.y >= MAX_GRID_BIN_Y) player.cell.y = MAX_GRID_BIN_Y-1;

        // rileva se la posizione mouse e' dentro la matrice binaria...
        mouseHoverCells = CheckCollisionPointRec(GetMousePosition(), (Rectangle){ grid_bin_XY.x, grid_bin_XY.y, MAX_GRID_BIN_X*gridSpacing,MAX_GRID_BIN_Y*gridSpacing });

            if (mouseHoverCells)
            {
                 // Icon painting mouse logic
                if ((player.cell.x >= 0) && (player.cell.y >= 0) && (player.cell.x < MAX_GRID_BIN_X*gridSpacing) && (player.cell.y < MAX_GRID_BIN_Y* gridSpacing))
                {
                    player.cell.x = (GetMouseX() - grid_bin_XY.x) / gridSpacing ;
                    player.cell.y = (GetMouseY() - grid_bin_XY.y) / gridSpacing;
                    // scrive bit 1/0 nella matrice binaria tasto sx /dx del mouse (1 o 0)
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) matrice[player.cell.x][player.cell.y] = 1;  
                    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) matrice[player.cell.x][player.cell.y] = 0; 
                }
            }
   
        // aggiorna posizione "cursore" quando ci si sposta sulla matrice con i tasto oppure il mouse
        int j= player.cell.x;
        int i= player.cell.y;

        // scrive bit 1/0 della cella selezionato della matrice binaria,  premendo la BARRA SPAZIO
        if ( (IsKeyPressed(KEY_SPACE)) ) matrice[player.cell.x][player.cell.y] = !matrice[player.cell.x][player.cell.y];

        //----------------------------------------------------------------------------------
		// Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(BLANK);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLANK);

            // draw round rectangle as "fake" background with some opacity
            drawRectangleRounded(0,0,screenWidth, screenHeight,BG_COLOR);

            // print titles and some heaaders
            DrawText(TextFormat("|||| %s v%s. daSOFT @2026", TOOL_NAME, TOOL_VERSION), grid_bin_XY.x, 24, 20, FG_COLOR); 
            //DrawText("When mouse cursor is inside matrix use mouse buttons to set/unset bit.", 140, 52, 10, GRID_COLOR);
            DrawText(TextFormat("BIN"), grid_bin_XY.x, grid_bin_XY.y-24, 20, SKYBLUE);
            DrawText(TextFormat("HEX"), grid_hex_XY.x - gridSpacing, grid_hex_XY.y, 20, SKYBLUE);
            DrawText(TextFormat("DEC"), grid_hex_XY.x + gridSpacing*5, grid_hex_XY.y, 20, SKYBLUE);
            DrawText(TextFormat("OCT"), grid_hex_XY.x + gridSpacing*10, grid_hex_XY.y, 20, SKYBLUE);
            
            // intestazioni riga/colonna matrice binaria
            for (int z = MAX_GRID_BIN_X - 1;z> -1; z--)
            {
                DrawText(TextFormat("%02d",z+1),grid_bin_XY.x + 736 - (z * gridSpacing),grid_bin_XY.y + 56,20,SKYBLUE); // bit decimal value
                if (z % 4 == 0 ) DrawLine(grid_bin_XY.x + 744 - (z * gridSpacing),grid_bin_XY.y - 24 ,grid_bin_XY.x + 744 - (z * gridSpacing),grid_bin_XY.y - 4, SKYBLUE);
            }

            drawGrids (); // disegna o meno laa griglia della matrice binaria
    
            drawBinCells(); // 1) disegna la matrice binaria disegnando lo sfondo della cella cambiando il colore di sfondo in base al valore 1/0
            draw_hex_grid(); // 2) disegna la matrice esadecimale

            BinToHex(); // 3) converti il valore binario di ogni riga nel corrispondente valore esadecimal (8 bit -> 1 byte 0x hex)
            printHexValues();  // 4) stampa nella matrice il valore esadecimale
            printDecOctValues();


            // 5) aggiorna in tempo reale la posizione della cella aattuale ("cursore") quando mouse o tastiera si spostano sulle celle...
            DrawRectangle((int)grid_bin_XY.x + player.cell.x*gridSpacing, 
                          (int)grid_bin_XY.y + player.cell.y*gridSpacing, 
                          gridSpacing -1, 
                          gridSpacing -1,
                          matrice[j][i] ? ON_COLOR : OFF_COLOR); 
            DrawText(TextFormat("%i",matrice[j][i]), 16 + grid_bin_XY.x + gridSpacing*j, 10 + grid_bin_XY.y + gridSpacing*i, 30, WHITE);


         // Draw buttons and left toolbar
        btnClearPressed = GuiButton((Rectangle){ 728, 16, 32, 32 }, "#143#");
        btnQuitPressed  = GuiButton((Rectangle){ 764, 16, 32, 32 }, "#113#");

       EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

