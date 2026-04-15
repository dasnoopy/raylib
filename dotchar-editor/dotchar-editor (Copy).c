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
*       - una volta che ho selezionato un ratatter ASCII po il mouse ove non deve selzionare altri caratteri fino al 
*         clik sinitro
*       - modiffica il realtime del carattere selezionato
*       - copy e paste
*       - save e load
*
*******************************************************************************************/

#define TOOL_NAME               "DotChar Editor"
#define TOOL_SHORT_NAME         "DotEdit"
#define TOOL_VERSION            "2.0.2"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
// load custom font to a 128x8 matrix
#include "custom_font.h"

// raygui integration
#define RAYGUI_IMPLEMENTATION
//#define RAYGUI_CUSTOM_ICONS     // Custom icons set required 
//#include "gui_iconset.h"        // Custom icons set provided, generated with rGuiIcons tool
#include "raygui.h"

const int screenWidth = 700;
const int screenHeight = 780;

 // initial X,Y coordinates for variuos interface elements
Vector2 grid_bin_XY = { 144, 144 }; // x, y devono essere uguale o multiplo di gridSpacing ....
Vector2 grid_hex_XY = { 572, 144 };


#define gridSpacing               36
#define MAX_GRID_BIN_X            8 // larghezza singolo carattere: max 1 byte (0-7)
#define MAX_GRID_BIN_Y            8// altezza singolo caratte fissa a 7 pixel (1-7)
#define MAX_GRID_HEX_X            2
#define MAX_GRID_HEX_Y MAX_GRID_BIN_Y 

bool showGrid = true;
// matrici
int matrice[MAX_GRID_BIN_X][MAX_GRID_BIN_Y];
char hex[MAX_GRID_HEX_X][MAX_GRID_BIN_Y];


// NORD colors
#define BG_COLOR CLITERAL(Color){ 59, 66, 82, 255} 
#define FG_COLOR CLITERAL(Color){ 236, 241, 241, 255}
#define GRID_COLOR CLITERAL(Color){ 201, 210, 210, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 76, 86, 106, 255} 
#define ON_COLOR CLITERAL(Color){ 208, 135, 112,255}
#define OFF_COLOR CLITERAL(Color){ 191, 97, 106,255}

// mouse and clipoard
bool mouseHoverCells = false;
bool mouseHoverASCII = false;
const char *clipboardText = NULL;
char inputBuffer[256] = ""; // Random initial string

// ASCII TABLE
Vector2 grid_ASCII  = { 64, 472 };
int currCHAR = 0;

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

// converti numero da decimale 0-15 a esadecimale 0-A
char charToHex(int val) {
    if (val < 10)
        return '0' + val;
    else
        return 'A' + (val - 10);
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

void drawLetter(int x,int y,int ASCII_CODE)
{ 
    int pos = ASCII_CODE ;
    char byte[8]={0,0,0,0,0,0,0,0};
    int posY = y;
            for (int y=0; y<MAX_GRID_BIN_Y; y++) // scansiona le 7 linee HEX che formano altezza carattere
            {
            HexToBin(TableFont[pos][y],byte);
            int posX = x;
            for(int i=MAX_GRID_BIN_X -1; i>-1 ; i--)
                {
                DrawRectangle(posX,posY,3,3, byte[i] ? FG_COLOR : GRID_BG_COLOR);
                posX += 4;
                }
            posY += 4;
            }   
}

void LoadLetter(int ASCII_CODE)
{ 
    int pos = ASCII_CODE ;
    char byte[8]={0,0,0,0,0,0,0,0};
            for (int y=0; y<MAX_GRID_BIN_Y; y++) // scansiona le 7 linee HEX che formano altezza carattere
            {
            HexToBin(TableFont[pos][y],byte);
            for(int i=MAX_GRID_BIN_X; i>-1; i--)
                {
                matrice[7-i][y]=byte[i];
                }
            }   
}



void drawASCII_Table (void)
{
int x;
int y;
int count=0;
        for (int j = 0; j < 8; ++j)
        {
            for (int i = 0; i < 16; ++i)
            {
                x= grid_ASCII.x + (i*gridSpacing);
                y= grid_ASCII.y + (j*gridSpacing);
                drawLetter(x,y, count);
                DrawText(TextFormat("%d",count), x, y, 10 ,RED);
                count++;
            }
        }
        // quaadrato bianco attorno alla lettera ASCII selezionata
         DrawRectangleLines(grid_ASCII.x + currCHAR % 16 * gridSpacing -2 , grid_ASCII.y + currCHAR/16 * gridSpacing -2 , gridSpacing, gridSpacing, GRID_COLOR);
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

// legge le righe della matrice binaria e memorizza valori HEX nella matrice esadecimale
void BinToHex (void)
{
    // dividi il byte in due valori 
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

//unico valore esadecimale del byte
for (int i = 0; i < MAX_GRID_HEX_Y; i++) 
    {
        uint8_t byte = 0;
            for (int j = 0; j < 8; j++)
            byte = (byte << 1) | matrice[j][i];

            TableFont[currCHAR][i] = byte;
    }

        // copia sempre il  contenuto dell'array HEX nella clipoboard per un successivo utilizzo (aggiorna carattere nella tabella ASCII e COPY in clipboards) 
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
             DrawText(TextFormat("%c", hex[0][i]), grid_hex_XY.x + 12, 4 + grid_hex_XY.y + gridSpacing*i, 30, GREEN);
             DrawText(TextFormat("%c", hex[1][i]), grid_hex_XY.x + gridSpacing + 12, 4 + grid_hex_XY.y + gridSpacing*i, 30, LIME);
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

    // Init player 0 (cursor for bin matrix)
    PlayerState player = { 0 };
    player.cell = (Point){ 0, 0 };

    // Init  player1 (cursor for ASCII table matrix)
    PlayerState player1 = { 0 };
    player1.cell = (Point){ 0, 0 };

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
                    // ultima riga prende valori della prima
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
                        // prima riga prende valore ultima riga
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
        mouseHoverCells = CheckCollisionPointRec(GetMousePosition(),(Rectangle){grid_bin_XY.x, grid_bin_XY.y,MAX_GRID_BIN_X*gridSpacing,MAX_GRID_BIN_Y*gridSpacing });
        mouseHoverASCII = CheckCollisionPointRec(GetMousePosition(),(Rectangle){grid_ASCII.x, grid_ASCII.y,16*gridSpacing,8*gridSpacing});
            
            if (mouseHoverCells)
            {
                 // Icon painting mouse logic
                if ((player.cell.x >= 0) && (player.cell.y >= 0) && (player.cell.x < MAX_GRID_BIN_X*gridSpacing) && (player.cell.y < MAX_GRID_BIN_Y* gridSpacing))
                {
                    player.cell.x = (GetMouseX() - grid_bin_XY.x) / gridSpacing ;
                    player.cell.y = (GetMouseY() - grid_bin_XY.y) / gridSpacing;
                    // scrive bit 1/0 nella matrice binaria tasto sx /dx del mouse (1 o 0)
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
                    {
                        matrice[player.cell.x][player.cell.y] = 1; 
                        //TableFont[currCHAR][player.cell.y] = '0x' & hex[0][1] & hex[1][2] ;
                    }
                    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) 
                    { 
                        matrice[player.cell.x][player.cell.y] = 0; 
                        TableFont[currCHAR][player.cell.y] = 0x00;
                    }

                }
            }
                    // rileva se la posizione mouse e' dentro la tabella ASCIIa...
            if (mouseHoverASCII)
            {
                 // Mouse logic over ASCII TABLE
                if ((player1.cell.x >= 0) && (player1.cell.y >= 0) && (player1.cell.x < 16*gridSpacing) && (player1.cell.y < 8*8))
                {
                    player1.cell.x = (GetMouseX() - grid_ASCII.x) / gridSpacing;
                    player1.cell.y = (GetMouseY() - grid_ASCII.y) / gridSpacing;

                    
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
                    {
                        currCHAR = player1.cell.x +(player1.cell.y *16);
                        LoadLetter(currCHAR);

                    }
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

            // print titles and some headers
            DrawText(TextFormat("%s v%s", TOOL_NAME, TOOL_VERSION), grid_bin_XY.x+ gridSpacing*2, 32, 20, FG_COLOR); 
            //DrawText("When mouse cursor is inside matrix use mouse buttons to set/unset bit.", 140, 52, 10, GRID_COLOR);
            DrawText(TextFormat("HEX"), grid_hex_XY.x + 16, grid_bin_XY.y - 32, 20, SKYBLUE);
            
            // intestazioni riga/colonna matrice binaria
            for (int z = 0; z < MAX_GRID_BIN_X; z++)
            {
                DrawText  (TextFormat("%01d",z+1),grid_bin_XY.x + 14 + (z * gridSpacing),grid_bin_XY.y -32 ,20,SKYBLUE); // bit decimal value
                DrawText  (TextFormat("%02d",potenza(2,7-z)),grid_bin_XY.x + 12 + (z * gridSpacing),grid_bin_XY.y + 12 +  (gridSpacing*MAX_GRID_BIN_Y),10,SKYBLUE); // potenza del due in basso
            }

            for (int z = 0; z < MAX_GRID_BIN_Y; ++z)
            {
                DrawText  (TextFormat("%01d",z+1),grid_bin_XY.x - 28,grid_bin_XY.y + 16 + (z * gridSpacing),20, SKYBLUE);
                DrawText  (TextFormat("%01d",z+1),grid_bin_XY.x + gridSpacing*8 + 20,grid_bin_XY.y + 16 + (z * gridSpacing),20, SKYBLUE);
                DrawText  ("0x",grid_hex_XY.x - 32 , grid_hex_XY.y + 12 + (z * gridSpacing),20, SKYBLUE);
            }
          
          

            if (showGrid) draw_bin_grid (); // disegna o meno la griglia della matrice binaria
            
            drawBinCells(); // 1) disegna la matrice binaria disegnando lo sfondo della cella cambiando il colore di sfondo in base al valore 1/0
            draw_hex_grid(); // 2) disegna la matrice esadecimale

            BinToHex(); // 3) converti il valore binario di ogni riga nel corrispondente valore esadecimal (8 bit -> 1 byte 0x hex)
            printHexValues();  // 4) stampa nella matrice il valore esadecimale



            // 5) aggiorna in tempo reale la posizione della cella attuale ("cursore") quando mouse o tastiera si spostano sulle celle...
            DrawRectangle((int)grid_bin_XY.x + player.cell.x*gridSpacing, 
                          (int)grid_bin_XY.y + player.cell.y*gridSpacing, 
                          gridSpacing -1, 
                          gridSpacing -1,
                          matrice[j][i] ? ON_COLOR : OFF_COLOR); 
            
            // stampa la tabella ASCII
            drawASCII_Table();

         // Draw buttons and left toolbar
        btnQuitPressed  = GuiButton((Rectangle){ 644, 20, gridSpacing, gridSpacing}, "#113#");

        // left toolbar
        btnShowGridPressed   = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y, gridSpacing, gridSpacing }, "#97#");
        btnShiftUpPressed    = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 1, gridSpacing, gridSpacing }, "#117#");
        btnShiftRightPressed = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 2, gridSpacing, gridSpacing }, "#115#");
        btnShiftLeftPressed  = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 3, gridSpacing, gridSpacing }, "#114#");
        btnShiftDownPressed  = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 4, gridSpacing, gridSpacing }, "#116#");
        btnRotateLeft        = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 5, gridSpacing, gridSpacing }, "#72#");
        btnRotateRight       = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 6, gridSpacing, gridSpacing }, "#73#");
        btnInvertPressed     = GuiButton((Rectangle){ grid_bin_XY.x - 96, grid_bin_XY.y + gridSpacing * 7, gridSpacing, gridSpacing }, "#94#");

        btnClearPressed      = GuiButton((Rectangle){ grid_bin_XY.x + gridSpacing*9 + 12, grid_bin_XY.y, gridSpacing, gridSpacing }, "#143#");
        btnCopyPressed       = GuiButton((Rectangle){ grid_bin_XY.x + gridSpacing*9 + 12, grid_bin_XY.y + gridSpacing*1, gridSpacing, gridSpacing }, "#16#");


        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

