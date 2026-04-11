/*******************************************************************************************
*
*   DOT CHAR EDITOR
*   A simple app to learn C using raylib library
* 
*  CHANGELOG:
* 
*  v. 1.0   : first release: draw a 8x8 dot matrix and show/copy HEX value
*  v. 1.2   : add controls to shift matrix up/down/left/right/invert/rotate left/right
*  v. 1.3   : add some other trivial utilities and code cleaning
*  v. 1.3.2 : add some visual improvements;
* 
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
*
*   TODO LIST POSSIBLE IMPROVEMENTS:
*       - evolvere in un vero dot font editor?
*       - Improvement 02
*
*******************************************************************************************/

#define TOOL_NAME               "DotChar Editor"
#define TOOL_SHORT_NAME         "dcED"
#define TOOL_VERSION            "1.3.2"

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

const int screenWidth = 724;
const int screenHeight = 672;

 // initial X,Y coordinates for variuos interface elements
Vector2 grid_bin_XY = { 144, 144 }; // x, y devono essere uguale o multiplo di gridSpacing ....
Vector2 grid_hex_XY = { 572, 144 };

#define gridSpacing               48
#define MAX_GRID_BIN_X            8
#define MAX_GRID_BIN_Y MAX_GRID_BIN_X
#define MAX_GRID_HEX_X            2
#define MAX_GRID_HEX_Y MAX_GRID_BIN_Y 

bool showGrid = false;
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
const char *clipboardText = NULL;
char inputBuffer[256] = ""; // Random initial string

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

// 'fake' background
void drawRectangleRounded (int x, int y, int w, int h, Color color)  
{
  Rectangle  rect = { x, y, w, h};   // toplx, toply, width, height
  float radius = 0.04; // no radius
  int   segs   = 12; // non segments
  DrawRectangleRounded ( rect, radius, segs, color );
}

// Draw binary matrix grid
void draw_bin_grid(void)
{
            
            for (int y = 0; y <= MAX_GRID_BIN_Y; y++) {
                DrawLine((int)grid_bin_XY.x, (int)grid_bin_XY.y + y * gridSpacing,(int)grid_bin_XY.x + MAX_GRID_BIN_X* gridSpacing, (int)grid_bin_XY.y + y*gridSpacing, GRID_COLOR);
                DrawLine((int)grid_hex_XY.x, (int)grid_hex_XY.y + y * gridSpacing,(int)grid_hex_XY.x + MAX_GRID_HEX_X* gridSpacing, (int)grid_hex_XY.y + y*gridSpacing, GRID_COLOR);
            }

            for (int x = 0; x <= MAX_GRID_BIN_X; x++)
                DrawLine((int)grid_bin_XY.x + x * gridSpacing, (int)grid_bin_XY.y,(int)grid_bin_XY.x + x * gridSpacing, (int)grid_bin_XY.y + MAX_GRID_BIN_Y*gridSpacing, GRID_COLOR);

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

// converti numero da 0-15 a esaadecimale 0-A
char charToHex(int val) {
    if (val < 10)
        return '0' + val;
    else
        return 'A' + (val - 10);
}

// legge le righe della matrice binaria e memorizza valori HEX nella matrice esadecimale
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
        hex[0][i] = charToHex(msb);
        hex[1][i] = charToHex(lsb);
           // Draw text box

        // copia il contenuto dell'aarray HEX nella clipoboard per un successivo COPT
        int k = 0;
        int index = 0;
        for (int z = 0; z < MAX_GRID_HEX_Y; z++) {
            if (index == MAX_GRID_HEX_Y-1) {
                // ultima coppia → niente ", "
                k += snprintf(inputBuffer + k, sizeof(inputBuffer) - k, "0x%c%c", hex[0][index], hex[1][index]);
            }
            else {
                k += snprintf(inputBuffer + k, sizeof(inputBuffer) - k, "0x%c%c, ", hex[0][index], hex[1][index]);
            }
            index++;
        }
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
                          matrice[j][i] ? FG_COLOR : GRID_BG_COLOR);
                        // mostra miniatura matrice per debug
                                DrawRectangleLines(44, 64, 48, 48, showGrid ? GRID_COLOR : GRID_BG_COLOR);  // NOTE: Uses QUADS internally, not lines
                                DrawRectangle(48 + 5*j, 68 + 5*i,4,4, matrice[j][i] ? FG_COLOR : GRID_BG_COLOR);
                    }
                }   
}

// stampa valori esadecimali nella relativa griglia
void printHexValues (void)
{
      for (int i = 0; i < MAX_GRID_HEX_Y; i++)
        {
             DrawText(TextFormat("%c", hex[0][i]), grid_hex_XY.x + 16, 6 + grid_hex_XY.y + gridSpacing*i, 40, OFF_COLOR);
             DrawText(TextFormat("%c", hex[1][i]), grid_hex_XY.x + gridSpacing + 16, 6 + grid_hex_XY.y + gridSpacing*i, 40, ON_COLOR);
        }
}

// azzera matrice binaria e di conseguenza anche quella esadecimale
void resetMatrici()
{
    //reset matrice binaria
    for (int i = 0; i < MAX_GRID_BIN_Y; i++)
        {  for (int j = 0; j < MAX_GRID_BIN_X; j++)
                    { matrice[j][i] = 0; }
        }    
}

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

int main (int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(screenWidth, screenHeight, "DotChar Editor");
    
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    //SetWindowState(FLAG_WINDOW_TOPMOST);
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);

    // UI required variables
    bool btnCopyPressed = false;
    bool btnClearPressed = false;
    bool btnQuitPressed = false;
    // toolbar
    bool btnShowGridPressed = false;
    bool btnShiftRightPressed = false;
    bool btnShiftLeftPressed = false;
    bool btnShiftDownPressed = false;
    bool btnShiftUpPressed = false;
    bool btnInvertPressed = false;
    bool btnRotateLeft = false;
    bool btnRotateRight = false;

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
            player.cell.x = 0;
            player.cell.y = 0;
        }
        if (btnCopyPressed)
        {
            SetClipboardText(inputBuffer); // Copy text to clipboard
            clipboardText = GetClipboardText(); // Get text from clipboard
        }
        if (btnQuitPressed) break;
        
        if (btnShowGridPressed) showGrid = !showGrid;

        
        if (btnInvertPressed)
        {
            // inverte matrice binaria
            for (int i = 0; i < MAX_GRID_BIN_Y; i++) {
                for (int j = 0; j < MAX_GRID_BIN_X; j++) { 
                    matrice[j][i] = !matrice[j][i]; } }
        }
        
        if (btnShiftRightPressed) // shift bin array right by 1
        {
     
            for (int i = 0; i < MAX_GRID_BIN_Y; i++) // righe
                {
                    // memorizza ultimo bit della riga
                    const int tmp = matrice[MAX_GRID_BIN_X - 1][i];
                    for (int j = MAX_GRID_BIN_X-1; j>0; j--) // colonne
                    {                        
                        // sposta verso destra bit righe
                        matrice[j][i] = matrice[j-1][i];
                    }
                    // alla fine il "primo" bit prende il valore dell'ultimo
                    matrice[0][i] = tmp;
                }
        }

        if (btnShiftLeftPressed) // shift bin array left by 1
        {
     
            for (int i = 0; i < MAX_GRID_BIN_Y; i++) // righe
                {
                    // memorizza primo bit della riga
                    const int tmp = matrice[0][i];
                    for (int j = 0; j< MAX_GRID_BIN_X-1; j++) // colonne
                    {                        
                        // sposta verso destra bit righe
                        matrice[j][i] = matrice[j+1][i];
                    }
                    // alla fine ultimo bit prende il valore del prim
                    matrice[MAX_GRID_BIN_X-1][i] = tmp;
                }
        }

        if (btnShiftUpPressed) // shift bin array down by 1
        {
     
            for (int i = 0; i < MAX_GRID_BIN_X; i++) // colonne
                {
                    // memorizza prima riga
                    const int tmp = matrice[i][0];
                        for (int j=0; j < MAX_GRID_BIN_Y-1; ++j) // righe
                        {
                            matrice[i][j] = matrice[i][j+1];
                        }
                    //
                    // ultima riga prende valori dellaa primaa
                    matrice[i][MAX_GRID_BIN_Y- 1] = tmp;
                }
        }
 
         if (btnShiftDownPressed) // shift bin array up by 1
         {
                for (int i = 0; i < MAX_GRID_BIN_X; i++) // colonne
                    {
                        // memorizza stato ultima riga
                        const int tmp = matrice[i][MAX_GRID_BIN_Y - 1];
                        for (int j = MAX_GRID_BIN_Y -1; j>0; --j) // righe
                        {
                            matrice[i][j] = matrice[i][j-1];
                        }
                        // prima riga prende valore ultima rigaa
                        matrice[i][0] = tmp;
                    }
        }

        if (btnRotateLeft)
        {
            // trasposizione  matrice binaria
            for (int i = 0; i < MAX_GRID_BIN_Y; i++) {
                for (int j = i +1 ; j < MAX_GRID_BIN_X ; j++) { 
                   int temp = matrice[j][i];
                   matrice[j][i] =  matrice[i][j];
                   matrice[i][j] = temp;
                }
            }
            // poi ruota di 90° antiorario
            for (int i = 0; i < MAX_GRID_BIN_Y; i++) {
                for (int j = 0,k = MAX_GRID_BIN_X -1; j<k; j++, k--) { 
                   int temp = matrice[i][j];
                   matrice[i][j] =  matrice[i][k];
                   matrice[i][k] = temp;
                }
            }
        }

        if (btnRotateRight) {
                    // trasposizione  matrice binaria
            for (int i = 0; i < MAX_GRID_BIN_Y; i++) {
                for (int j = i +1 ; j < MAX_GRID_BIN_X ; j++) { 
                   int temp = matrice[j][i];
                   matrice[j][i] =  matrice[i][j];
                   matrice[i][j] = temp;
                }
            }
            // poi ruota di 90° in senso orario
            for (int i = 0; i < MAX_GRID_BIN_Y; i++) {
                for (int j = 0,k = MAX_GRID_BIN_X -1; j<k; j++, k--) { 
                   int temp = matrice[j][i];
                   matrice[j][i] =  matrice[k][i];
                   matrice[k][i] = temp;
                }
            }
        }

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
            DrawText(TextFormat("%s v%s. daSOFT @2026", TOOL_NAME, TOOL_VERSION), 144, 48, 20, FG_COLOR); 
            //DrawText("When mouse cursor is inside matrix use mouse buttons to set/unset bit.", 140, 52, 10, GRID_COLOR);
            DrawText(TextFormat("HEX"), grid_hex_XY.x + 32, grid_bin_XY.y - 32, 20, SKYBLUE);
            
            // intestazioni riga/colonna matrice binaria
            for (int z = 0; z < MAX_GRID_BIN_X; z++)
            {
                DrawText  (TextFormat("%01d",z+1),grid_bin_XY.x + 16 + (z * gridSpacing),grid_bin_XY.y -32 ,20,SKYBLUE); // bit decimal value
                DrawText  (TextFormat("%02d",potenza(2,7-z)),grid_bin_XY.x + 12 + (z * gridSpacing),grid_bin_XY.y + 12 +  (gridSpacing*MAX_GRID_BIN_Y),20,SKYBLUE); // potenzaa del due in basso
            }

            for (int z = 0; z < MAX_GRID_BIN_Y; ++z)
            {
                DrawText  (TextFormat("%01d",z+1),grid_bin_XY.x - 28,grid_bin_XY.y + 16 + (z * gridSpacing),20, SKYBLUE);
                DrawText  ("0x",grid_hex_XY.x -34 , grid_hex_XY.y + 16 + (z * gridSpacing),20, SKYBLUE);
            }
          
            if (showGrid) draw_bin_grid (); // disegnaa o meno laa griglia della matrice binariaa
            
            drawBinCells(); // 1) disegna la matrice binaria disegnando lo sfondo della cella cambiando il colore di sfondo in base al valore 1/0
            draw_hex_grid(); // 2) disegna la matrice esadecimale

            BinToHex(); // 3) converti il valore binario di ogni riga nel corrispondente valore esadecimal (8 bit -> 1 byte 0x hex)
            printHexValues();  // 4) stampa nella matrice il valore esadecimale

            // 5) aaggiorna in tempo reale la posizione della cella aattuale ("cursore") quando mouse o tastiera si spostano sulle celle...
            DrawRectangle((int)grid_bin_XY.x + player.cell.x*gridSpacing, 
                          (int)grid_bin_XY.y + player.cell.y*gridSpacing, 
                          gridSpacing -1, 
                          gridSpacing -1,
                          matrice[j][i] ? ON_COLOR : OFF_COLOR); 

         // Draw buttons and left toolbar
        btnClearPressed = GuiButton((Rectangle){ 200, 600, 96, 40 }, "#143#Clear");
        btnCopyPressed  = GuiButton((Rectangle){ 300, 600, 96, 40 }, "#16#CopyHEX");
        btnQuitPressed  = GuiButton((Rectangle){ 400, 600, 96, 40 }, "#152#Quit");

        // left toolbar
        btnShowGridPressed   = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y, 40, 40 }, "#97#");
        btnShiftUpPressed    = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 1, 40, 40 }, "#117#");
        btnShiftRightPressed = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 2, 40, 40 }, "#115#");
        btnShiftLeftPressed  = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 3, 40, 40 }, "#114#");
        btnShiftDownPressed  = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 4, 40, 40 }, "#116#");
        btnRotateLeft        = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 5, 40, 40 }, "#72#");
        btnRotateRight       = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 6, 40, 40 }, "#73#");
        btnInvertPressed     = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 7, 40, 40 }, "#94#");

        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

