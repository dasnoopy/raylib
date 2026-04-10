/*******************************************************************************************
*
*   DOT FONT EDITOR
*
*   Bisogna installare raylib da repository ufficiali Archlinux, mentre raygui va
*   installato da AUR.
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

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// TODO
// copia valor i HEX in clipboard
// layout dinamico in base all dimensione griglie
// integrare uso del mouse per cliccare sullaa cella e disegnarla : https://dotmatrixtool.com/


const int screenWidth = 640;
const int screenHeight = 640;

#define gridSpacing           48
#define MAX_GRID_BIN_X            8
#define MAX_GRID_BIN_Y            8
#define MAX_GRID_HEX_X            2
#define MAX_GRID_HEX_Y            8

// matrici
int matrice[MAX_GRID_BIN_X][MAX_GRID_BIN_Y];
char hex[MAX_GRID_HEX_X][MAX_GRID_BIN_Y];

 // initial X,Y coordinates for variuos interface elements
Vector2 grid_bin_XY = { 32, 128 };
Vector2 grid_hex_XY = { 512, 128 };

bool debug = false; // visualizza info di debug si / no (sia a video che in console)

// NORD colors
#define BG_COLOR CLITERAL(Color){ 67, 76, 94, 255}
#define FG_COLOR CLITERAL(Color){ 236, 239, 244, 255}
#define GRID_COLOR CLITERAL(Color){ 211, 222, 233, 255} 
#define ON_COLOR CLITERAL(Color){ 208, 135, 112,255}
#define OFF_COLOR CLITERAL(Color){ 191, 97, 106,255}

const char *clipboardText = NULL;
char inputBuffer[256] = "0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00"; // Random initial string

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
// Point struct, like Vector2 but using int
typedef struct {
    int x;
    int y;
} Point;

// Player state struct
// NOTE: Contains all player data
typedef struct {
    Point cell;
    Color color;
} PlayerState;

void drawRectangleRounded (int x, int y, int w, int h, Color color)  
{
  Rectangle  rect = { x, y, w, h};   // toplx, toply, width, height
  float radius = 0.04; // no radius
  int   segs   = 12; // non segments
  DrawRectangleRounded ( rect, radius, segs, color );
}

void draw_bin_grid(void)
{
// Draw binary matrix grid
            for (int y = 0; y <= MAX_GRID_BIN_Y; y++)
                DrawLine((int)grid_bin_XY.x, (int)grid_bin_XY.y + y * gridSpacing,(int)grid_bin_XY.x + MAX_GRID_BIN_X* gridSpacing, (int)grid_bin_XY.y + y*gridSpacing, GRID_COLOR);
            for (int x = 0; x <= MAX_GRID_BIN_X; x++)
                DrawLine((int)grid_bin_XY.x + x * gridSpacing, (int)grid_bin_XY.y,(int)grid_bin_XY.x + x * gridSpacing, (int)grid_bin_XY.y + MAX_GRID_BIN_Y*gridSpacing, GRID_COLOR);
}

void draw_hex_grid(void)
{
// Draw hex matrix grid
            for (int y = 0; y <= MAX_GRID_HEX_Y; y++)
                DrawLine((int)grid_hex_XY.x, (int)grid_hex_XY.y + y * gridSpacing,(int)grid_hex_XY.x + MAX_GRID_HEX_X * gridSpacing, (int)grid_hex_XY.y + y * gridSpacing, GRID_COLOR);
            for (int x = 0; x <= MAX_GRID_HEX_X; x++)
                DrawLine((int)grid_hex_XY.x + x * gridSpacing, (int)grid_hex_XY.y,(int)grid_hex_XY.x + x * gridSpacing, (int)grid_hex_XY.y + MAX_GRID_HEX_Y * gridSpacing, GRID_COLOR);
}


char toHexChar(int val) {
    if (val < 10)
        return '0' + val;
    else
        return 'A' + (val - 10);
}

void BinToHex (void)
{
     for (int i = 0; i < MAX_GRID_HEX_Y; i++) 
     {
        int msb = 0;
        int lsb = 0;
        for (int j = 0; j < 4; j++)
            msb = (msb << 1) | matrice[j][i];
        for (int j = 4; j < 8; j++)
            lsb = (lsb << 1) | matrice[j][i];
        hex[0][i] = toHexChar(msb);
        hex[1][i] = toHexChar(lsb);
    }
}
void printBin()
{
          //printBin();
            for (int i = 0; i < MAX_GRID_BIN_Y; i++)
                {
                    for (int j = 0; j < MAX_GRID_BIN_X; j++)
                    {
                        DrawRectangle((int)grid_bin_XY.x + gridSpacing*j, (int)grid_bin_XY.y + gridSpacing*i, 
                          gridSpacing -1, 
                          gridSpacing -1, 
                          matrice[j][i] ? FG_COLOR : BG_COLOR);
                        //miniatura 
                        if (debug) 
                            {
                                DrawRectangleLines(28, 28, 48, 48, GRID_COLOR);  // NOTE: Uses QUADS internally, not lines
                                DrawRectangle(32 + 5*j, 32 + 5*i,4,4, matrice[j][i] ? GRID_COLOR : BG_COLOR);
                            }
                        //some info
                        if (debug) DrawText(TextFormat("%i",matrice[j][i]), 16 + grid_bin_XY.x + gridSpacing*j, 10 + grid_bin_XY.y + gridSpacing*i, 30,GRID_COLOR);
                    }
                }   
}

void printHex (void)
{
      for (int i = 0; i < MAX_GRID_HEX_Y; i++)
        {
             DrawText(TextFormat("%c", hex[0][i]), 526, 6 + grid_bin_XY.y + gridSpacing*i, 40, GREEN);
             DrawText(TextFormat("%c", hex[1][i]), 574, 6 + grid_bin_XY.y + gridSpacing*i, 40, LIME);
        }
}

void resetMatrici()
{
    //reset matrice binaria
    for (int i = 0; i < MAX_GRID_BIN_Y; i++)
        {  for (int j = 0; j < MAX_GRID_BIN_X; j++)
                    { matrice[j][i] = 0; }
        }    
    
    // //reset matrice esadecimale
    // for (int i = 0; i < MAX_GRID_HEX_Y; i++)
    //     {  
    //         hex[0][i] = 0;
    //         hex[1][i] = 0;
    //     } 
}

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

int main (int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(screenWidth, screenHeight, "DotChar Editor");
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowState(FLAG_WINDOW_TOPMOST);
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);

    // UI required variables
    bool btnCopyPressed = false;
    bool btnClearPressed = false;
    bool btnDebugPressed = debug;
    bool btnQuitPressed = false;

    // Set UI style
    // Custom GUI font loading
    Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    GuiLoadStyle("assets/style_genesis.rgs");
    GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetIconScale(1);

    // Init current player state
    PlayerState player = { 0 };
    player.cell = (Point){ 0, 0 };
    player.color = SKYBLUE;

    //reset matrice binaria
    resetMatrici();


// Source - https://stackoverflow.com/a/2218305
// Posted by Salv0, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-08, License - CC BY-SA 2.5

    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        if (btnClearPressed) resetMatrici();
        if (btnDebugPressed) debug=!debug;
        if (btnCopyPressed)
        {
            SetClipboardText(inputBuffer); // Copy text to clipboard
            clipboardText = GetClipboardText(); // Get text from clipboard
        }
        if (btnQuitPressed) break;
        
        //----------------------------------------------------------------------------------
        // Player movement logic using arrow keys
        if (IsKeyPressed(KEY_RIGHT)) player.cell.x++;
        else if (IsKeyPressed(KEY_LEFT)) player.cell.x--;
        else if (IsKeyPressed(KEY_UP)) player.cell.y--;
        else if (IsKeyPressed(KEY_DOWN)) player.cell.y++;

        // // Make sure player does not go out of bounds
        if (player.cell.x < 0) player.cell.x = MAX_GRID_BIN_X-1;
        else if (player.cell.x >= MAX_GRID_BIN_X) player.cell.x = 0;
        if (player.cell.y < 0) player.cell.y = MAX_GRID_BIN_Y -1 ;
        else if (player.cell.y >= MAX_GRID_BIN_Y) player.cell.y = 0;


        // aggiorna posizione "cursore" quando ci si sposta sulla matrice
        int j= player.cell.x;
        int i= player.cell.y;

        // cambia /inverti stato "bit" premento la BARRA SPAZIO
        if ( (IsKeyPressed(KEY_SPACE)) ) matrice[player.cell.x][player.cell.y] = !matrice[player.cell.x][player.cell.y];

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
            DrawText("Dot char editor v.1.0 by: daSoft @2026", 112, 24, 20, SKYBLUE);
            DrawText("Press SPACEBAR to DRAW bit / ARROWS keys to select cell / ESC to terminate.", 96, 52, 10, GRID_COLOR);
            DrawText(TextFormat("MSB / LSB"), 500, 96, 20, SKYBLUE);
            for (int z = 0; z < MAX_GRID_BIN_X; z++)
            {
                DrawText  (TextFormat("%02d",z),44 + (z * gridSpacing),96,20,SKYBLUE); // bit decimal value
                DrawText  (TextFormat("%02d",potenza(2,z)),384 - (z * gridSpacing),520,20,SKYBLUE); // bit decimal value
            }

            for (int z = 0; z < MAX_GRID_BIN_Y; ++z)
            {
                DrawText  (TextFormat("%02i",z),428,144+(z * gridSpacing),20, SKYBLUE);
                DrawText  ("0x",482,144+(z * gridSpacing),20, SKYBLUE);
            }
          
            // draw bin and hex matrix
            draw_bin_grid ();
            draw_hex_grid ();
            printHex();  // print HEX matrix
            BinToHex(); //popola matrice valori esadecimali dalla corrispondente riga di matrice binaria
            printBin(); // print BIN matrix
            // "cursore" che si sposta sulle celle...
            DrawRectangle((int)grid_bin_XY.x + player.cell.x*gridSpacing, 
                          (int)grid_bin_XY.y + player.cell.y*gridSpacing, 
                          gridSpacing -1, 
                          gridSpacing -1,
                          matrice[j][i] ? ON_COLOR : OFF_COLOR); 

            // DEBUG INFO
            // show bit value of bin matrix of current cursor position
            if (debug) DrawText(TextFormat("%i",matrice[j][i]), 16 + grid_bin_XY.x + gridSpacing*j, 10 + grid_bin_XY.y + gridSpacing*i, 30, BLACK);
            // shhow some info 
            if (debug) DrawText(TextFormat("row: %02i, col: %02i, bit value: %02i, hex byte val: Ox%c%c", i, j, matrice[player.cell.x][player.cell.y], hex[0][player.cell.y], hex[1][player.cell.y]), 88,612, 20, FG_COLOR);
        
         // Draw buttons
        btnDebugPressed = GuiButton((Rectangle){ 32, 560, 96, 40 }, "#193#Debug");
        btnCopyPressed = GuiButton((Rectangle){ 132, 560, 96, 40 }, "#16#CopyHEX");
        btnClearPressed = GuiButton((Rectangle){ 232, 560, 96, 40 }, "#143#Clear");
        btnQuitPressed = GuiButton((Rectangle){ 512, 560, 96, 40 }, "#74#Quit");

        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

