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
*       - consentire edit solo dei caratteri validi da 33 a 127
*       - funzione specchio orrizzontale e verticale
*       - gestione font.data : 
*           - file missing x esempio 
*           - warning overwrite file font.data
*           - warning load che sovrascrive mappa caratteri attuale
*           - fare toolbar con pulsante e testo perche con le icone non si capisce molto la loro funzione
*
*******************************************************************************************/

#define TOOL_NAME               "DotChar Editor"
#define TOOL_SHORT_NAME         "DotEdit"
#define TOOL_VERSION            "2.2.0"

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

const int screenWidth = 640;
const int screenHeight = 780;

 // initial X,Y coordinates for variuos interface elements
Vector2 bin_grid_XY = { 96, 96 }; // x, y devono essere uguale o multiplo di gridSpacing ....
Vector2 hex_grid_XY = {496, 96 }; // posizione tabella esadecimale
Vector2 toolbar_XY = { 58, 420 }; // posizione toolbar
// ASCII TABLE
Vector2 ascii_grid_XY  = { 36, 472 };
int curr_ascii_char = 32; //carattere corrente selezionato nella tabella ASCII : default iniziale "!"

#define gridSpacing       36
#define BIN_COLS        8 // larghezza matrice binario (nr.colonne)del disegno  (1-8)
#define BIN_ROWS       8 // altezza matrice binaria( nr. righe) del singolo carattere (1-8)
#define HEX_VAL_X         2 // larghezza matrice esadecimale (nr. colonne) per nibble 1 e 2 (nibble = mezzo byte MSB e LSB)
#define HEX_VAL_Y         8 // deve corrispondere a CHAR_ROW : altezza matrice esadecimale( nr. righe) (1-8)

bool showGrid = true;

// definizione matrici
int matrice[BIN_COLS][BIN_ROWS];
int revert_matrix[BIN_COLS][BIN_ROWS];
int copypaste_matrix[BIN_COLS][BIN_ROWS];
char hex[HEX_VAL_X][HEX_VAL_Y];

// // NORD colors (Dark)
// #define BG_COLOR CLITERAL(Color){ 59, 66, 82, 255} 
// #define FG_COLOR CLITERAL(Color){ 236, 241, 241, 255}
// #define GRID_COLOR CLITERAL(Color){ 201, 210, 210, 255} 
// #define GRID_BG_COLOR CLITERAL(Color){ 76, 86, 106, 255} 
// #define ON_COLOR CLITERAL(Color){ 208, 135, 112,255}
// #define OFF_COLOR CLITERAL(Color){ 191, 97, 106,255}

// ARDUINO Matrix tool colors (light)
#define BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
#define FG_COLOR CLITERAL(Color){ 79, 88, 92, 255}
#define GRID_COLOR CLITERAL(Color){ 55, 66, 70, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 218, 227,227, 255} 
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 32, 181, 186,255}

// mouse and clipoard
bool mouseHoverCells = false;
bool mouseHoverASCII = false;
const char *clipboardText = NULL;
char inputBuffer[256] = ""; // Random initial string


// bottoni toolbar
    // UI required variables
    bool btnCopyPressed = false;
    bool btnLoadPressed = false;
    bool btnSavePressed = false;
    bool btnPastePressed = false;
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
    bool btnRevertPressed = false;

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
          esp--;
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

// disegna le miniature dei caratteri nella tabella ASCII
void drawLetter(int x,int y,int ASCII_CODE)
{ 
    int pos = ASCII_CODE ;
    char byte[8]={0,0,0,0,0,0,0,0};
    int posY = y;
            for (int y=0; y<BIN_ROWS; y++) // scansiona le 7 linee HEX che formano altezza carattere
            {
            HexToBin(TableFont[pos][y],byte);
            int posX = x;
            for(int i=BIN_COLS -1; i>-1 ; i--)
                {
                DrawRectangle(posX,posY,3,3, byte[i] ? FG_COLOR : GRID_BG_COLOR);
                posX += 4;
                }
            posY += 4;
            }   

        // quadrato bianco attorno alla lettera ASCII selezionata
         DrawRectangleLines(ascii_grid_XY.x + curr_ascii_char % 16 * gridSpacing -2 , ascii_grid_XY.y + curr_ascii_char/16 * gridSpacing -2 , gridSpacing, gridSpacing, GRID_COLOR);
}

// carica nella matrice il carattere selezionato dalla tabella ASCII
void LoadLetter(int ASCII_CODE)
{ 
    int pos = ASCII_CODE ;
    char byte[8]={0,0,0,0,0,0,0,0};
            for (int y=0; y<BIN_ROWS; y++) // scansiona le 7 linee HEX che formano altezza carattere
            {
            HexToBin(TableFont[pos][y],byte);
            for(int i=BIN_COLS; i>-1; i--)
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
                x= ascii_grid_XY.x + (i*gridSpacing);
                y= ascii_grid_XY.y + (j*gridSpacing);
                drawLetter(x,y, count);
                DrawText(TextFormat("%d",count), x, y, 10 ,RED);
                count++;
            }
        }
        DrawRectangleLines(ascii_grid_XY.x -6, ascii_grid_XY.y -6 , 8 + gridSpacing*16, 8 + gridSpacing*8, GRID_BG_COLOR);
}

// Draw binary matrix grid
void draw_bin_grid(void)
{
            
            for (int y = 0; y <= BIN_ROWS; y++) {
                DrawLine((int)bin_grid_XY.x, (int)bin_grid_XY.y + y * gridSpacing,(int)bin_grid_XY.x + BIN_COLS* gridSpacing, (int)bin_grid_XY.y + y*gridSpacing, GRID_COLOR);
                DrawLine((int)hex_grid_XY.x, (int)hex_grid_XY.y + y * gridSpacing,(int)hex_grid_XY.x + HEX_VAL_X* gridSpacing, (int)hex_grid_XY.y + y*gridSpacing, GRID_COLOR);
            }

            for (int x = 0; x <= BIN_COLS; x++)
                DrawLine((int)bin_grid_XY.x + x * gridSpacing, (int)bin_grid_XY.y,(int)bin_grid_XY.x + x * gridSpacing, (int)bin_grid_XY.y + BIN_ROWS*gridSpacing, GRID_COLOR);

            for (int x = 0; x <= HEX_VAL_X; x++)
                DrawLine((int)hex_grid_XY.x + x * gridSpacing, (int)hex_grid_XY.y,(int)hex_grid_XY.x + x * gridSpacing, (int)hex_grid_XY.y + HEX_VAL_Y*gridSpacing, GRID_COLOR);
}

// Draw hex matrix grid
void draw_hex_grid(void)
{
            // background
            for (int y = 0; y < HEX_VAL_Y; y++)
            {
                for (int x = 0; x < HEX_VAL_X; x++) { 
                    DrawRectangle((int)hex_grid_XY.x + x*gridSpacing, (int)hex_grid_XY.y + y * gridSpacing , gridSpacing-1, gridSpacing-1, GRID_BG_COLOR);  }
            }
}

// legge le righe della matrice binaria e memorizza valori HEX nella matrice esadecimale
void BinToHex (void)
{
    // dividi il byte in due parti MSB e LSB (nibble)
     for (int i = 0; i < HEX_VAL_Y; i++) 
     {
        int msb = 0;
        int lsb = 0;
        for (int j = 0; j < 4; j++)
            msb = (msb << 1) | matrice[j][i];
        for (int j = 4; j < 8; j++)
            lsb = (lsb << 1) | matrice[j][i];
        hex[0][i] = charToHex(msb);
        hex[1][i] = charToHex(lsb);

        //converti in valore esadecimale in un unico valore (byte)
        for (int i = 0; i < HEX_VAL_Y; i++) 
            {
                uint8_t byte = 0;
                for (int j = 0; j < 8; j++)
                {
                    byte = (byte << 1) | matrice[j][i];
                    TableFont[curr_ascii_char][i] = byte;
                }
            }
    }
}

//  disegna bit delle matrice in base al loro valore
void drawBinCells()
{
            for (int i = 0; i < BIN_ROWS; i++)
                {
                    for (int j = 0; j < BIN_COLS; j++)
                    {
                        // disegna sfondo cella  in base al valore 1/0
                        // se si cambia disegno qui, cambiare anche cursore nella sezione BeginDrawing
                        DrawRectangle(bin_grid_XY.x + gridSpacing*j, bin_grid_XY.y + gridSpacing*i, 
                          gridSpacing -1, 
                          gridSpacing -1, 
                          matrice[j][i] ? FG_COLOR : GRID_BG_COLOR);
                          // // mostra miniatura matrice per debug
                          //       DrawRectangleLines(bin_grid_XY.x + gridSpacing*10 -4, bin_grid_XY.y + gridSpacing*3 -4 , 72, 72, GRID_BG_COLOR);  // NOTE: Uses QUADS internally, not lines
                          //       DrawRectangle((bin_grid_XY.x + gridSpacing*10) + 8*j, (bin_grid_XY.y + gridSpacing*3) + 8*i,7,7, matrice[j][i] ? FG_COLOR : GRID_BG_COLOR);
                    }
                }   
}

// stampa valori esadecimali nella relativa griglia
void printHexValues (void)
{
      for (int i = 0; i < HEX_VAL_Y; i++)
        {
             DrawText(TextFormat("%c", hex[0][i]), hex_grid_XY.x + 12, 4 + hex_grid_XY.y + gridSpacing*i, 30, FG_COLOR);
             DrawText(TextFormat("%c", hex[1][i]), hex_grid_XY.x + gridSpacing + 12, 4 + hex_grid_XY.y + gridSpacing*i, 30, FG_COLOR);
        }
}

// azzera matrice binaria e di conseguenza anche quella esadecimale
void reset_matrix()
{
    //reset matrice binaria
    for (int i = 0; i < BIN_ROWS; i++)
        {  for (int j = 0; j < BIN_COLS; j++)
                    { matrice[j][i] = 0; }
        }    
}

void copy_matrix_2d(int * src, int * dst, int N, int M){
    for(int i=0; i<N; i++){
        for(int j=0; j<M; j++){
            dst[(M*i)+j] = src[(M*i)+j];
        }
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

    // Set UI style
    // Custom GUI font loading
    Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    GuiLoadStyle("assets/style_amber.rgs");
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
    reset_matrix();

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------

        if (btnRevertPressed) copy_matrix_2d(&revert_matrix[0][0], &matrice[0][0], 8, 8);
        if (btnClearPressed)
        {
            reset_matrix();
            player.cell.x = 0;
            player.cell.y = 0;
        }
        if (btnCopyPressed) copy_matrix_2d(&matrice[0][0], &copypaste_matrix[0][0], 8, 8);
        if (btnPastePressed) copy_matrix_2d(&copypaste_matrix[0][0], &matrice[0][0], 8, 8);
        if (btnQuitPressed) break;
        if (btnShowGridPressed) 
            {
                showGrid = !showGrid;
            }
        if (btnInvertPressed)
        {
            // inverte matrice binaria
            for (int i = 0; i < BIN_ROWS; i++) {
                for (int j = 0; j < BIN_COLS; j++) { 
                    matrice[j][i] = !matrice[j][i]; } }
        }
        
        if (btnShiftRightPressed) // shift bin array right by 1
        {
     
            for (int i = 0; i < BIN_ROWS; i++) // righe
                {
                    // memorizza ultimo bit della riga
                    const int tmp = matrice[BIN_COLS - 1][i];
                    for (int j = BIN_COLS-1; j>0; j--) // colonne
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
     
            for (int i = 0; i < BIN_ROWS; i++) // righe
                {
                    // memorizza primo bit della riga
                    const int tmp = matrice[0][i];
                    for (int j = 0; j< BIN_COLS-1; j++) // colonne
                    {                        
                        // sposta verso destra bit righe
                        matrice[j][i] = matrice[j+1][i];
                    }
                    // alla fine ultimo bit prende il valore del prim
                    matrice[BIN_COLS-1][i] = tmp;
                }
        }

        if (btnShiftUpPressed) // shift bin array down by 1
        {
     
            for (int i = 0; i < BIN_COLS; i++) // colonne
                {
                    // memorizza prima riga
                    const int tmp = matrice[i][0];
                        for (int j=0; j < BIN_ROWS-1; ++j) // righe
                        {
                            matrice[i][j] = matrice[i][j+1];
                        }
                    //
                    // ultima riga prende valori della prima
                    matrice[i][BIN_ROWS- 1] = tmp;
                }
        }
 
         if (btnShiftDownPressed) // shift bin array up by 1
         {
                for (int i = 0; i < BIN_COLS; i++) // colonne
                    {
                        // memorizza stato ultima riga
                        const int tmp = matrice[i][BIN_ROWS - 1];
                        for (int j = BIN_ROWS -1; j>0; --j) // righe
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
            for (int i = 0; i < BIN_ROWS; i++) {
                for (int j = i +1 ; j < BIN_COLS ; j++) { 
                   int temp = matrice[j][i];
                   matrice[j][i] =  matrice[i][j];
                   matrice[i][j] = temp;
                }
            }
            // poi ruota di 90° antiorario
            for (int i = 0; i < BIN_ROWS; i++) {
                for (int j = 0,k = BIN_COLS -1; j<k; j++, k--) { 
                   int temp = matrice[i][j];
                   matrice[i][j] =  matrice[i][k];
                   matrice[i][k] = temp;
                }
            }
        }

        if (btnRotateRight) {
                    // trasposizione  matrice binaria
            for (int i = 0; i < BIN_ROWS; i++) {
                for (int j = i +1 ; j < BIN_COLS ; j++) { 
                   int temp = matrice[j][i];
                   matrice[j][i] =  matrice[i][j];
                   matrice[i][j] = temp;
                }
            }
            // poi ruota di 90° in senso orario
            for (int i = 0; i < BIN_ROWS; i++) {
                for (int j = 0,k = BIN_COLS -1; j<k; j++, k--) { 
                   int temp = matrice[j][i];
                   matrice[j][i] =  matrice[k][i];
                   matrice[k][i] = temp;
                }
            }
        }

        if (btnSavePressed)
        {
                // Source - https://stackoverflow.com/a/18597747
                // Posted by Sergey Kalinichenko
                // Retrieved 2026-04-16, License - CC BY-SA 3.0

                FILE *fSave = fopen("font.data", "wb");
                fwrite(TableFont, sizeof(char), sizeof(TableFont), fSave);
                fclose(fSave);
        }

        if (btnLoadPressed)
        {
            FILE *fLoad = fopen("font.data", "rb"); 
            fread(TableFont, sizeof(char), sizeof(TableFont), fLoad);
            fclose(fLoad);
        }
        
        //----------------------------------------------------------------------------------
        // Player movement logic using arrow keys
        if (IsKeyPressed(KEY_RIGHT)) player.cell.x++;
        else if (IsKeyPressed(KEY_LEFT)) player.cell.x--;
        else if (IsKeyPressed(KEY_UP)) player.cell.y--;
        else if (IsKeyPressed(KEY_DOWN)) player.cell.y++;

        // Make sure player does not go out of bounds
        if (player.cell.x < 0) player.cell.x = 0;
        else if (player.cell.x >= BIN_COLS) player.cell.x = BIN_COLS-1;
        else if (player.cell.y < 0) player.cell.y = 0 ;
        else if (player.cell.y >= BIN_ROWS) player.cell.y = BIN_ROWS-1;

        // rileva se la posizione mouse e' dentro la matrice binaria...
        mouseHoverCells = CheckCollisionPointRec(GetMousePosition(),(Rectangle){bin_grid_XY.x, bin_grid_XY.y,BIN_COLS*gridSpacing,BIN_ROWS*gridSpacing });
        mouseHoverASCII = CheckCollisionPointRec(GetMousePosition(),(Rectangle){ascii_grid_XY.x, ascii_grid_XY.y,16*gridSpacing,8*gridSpacing});
            
            if (mouseHoverCells)
            {
                 // Icon painting mouse logic
                    player.cell.x = (GetMouseX() - bin_grid_XY.x) / gridSpacing ;
                    player.cell.y = (GetMouseY() - bin_grid_XY.y) / gridSpacing;
                    // scrive bit 1/0 nella matrice binaria tasto sx /dx del mouse (1 o 0)
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) matrice[player.cell.x][player.cell.y] = 1;
                    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) matrice[player.cell.x][player.cell.y] = 0;
            }
                    
            // rileva se la posizione mouse e' dentro la tabella ASCIIa...
            if (mouseHoverASCII)
            {
                 // Mouse logic over ASCII TABLE

                    player1.cell.x = (GetMouseX() - ascii_grid_XY.x) / gridSpacing;
                    player1.cell.y = (GetMouseY() - ascii_grid_XY.y) / gridSpacing;
                    copy_matrix_2d(&matrice[0][0], &revert_matrix[0][0], 8, 8);
                    
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
                    {
                        curr_ascii_char = player1.cell.x +(player1.cell.y *16);
                        LoadLetter(curr_ascii_char);
                    }
            }
   
        // aggiorna posizione "cursore" quando ci si sposta sulla matrice binaria con i tasto oppure il mouse
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
            // stampa la tabella ASCII
            drawASCII_Table();

            // print titles and some headers
            DrawText(TextFormat("%s v%s", TOOL_NAME, TOOL_VERSION), bin_grid_XY.x+ gridSpacing*4, 24, 20, FG_COLOR); 
            //DrawText("When mouse cursor is inside matrix use mouse buttons to set/unset bit.", 140, 52, 10, GRID_COLOR);
            DrawText(TextFormat("HEX"), hex_grid_XY.x + 16, bin_grid_XY.y - 32, 20, FG_COLOR);
            
            // intestazioni riga/colonna matrice binaria
            for (int z = 0; z < BIN_COLS; z++)
            {
                DrawText  (TextFormat("%01d",z+1),bin_grid_XY.x + 14 + (z * gridSpacing),bin_grid_XY.y -28 ,20,FG_COLOR); // bit decimal value
                DrawText  (TextFormat("%01d",z+1),bin_grid_XY.x + 14 + (z * gridSpacing),bin_grid_XY.y + gridSpacing*8 +8  ,20,FG_COLOR); // bit decimal value
                //DrawText  (TextFormat("%02d",potenza(2,7-z)),bin_grid_XY.x + 12 + (z * gridSpacing),bin_grid_XY.y + 12 +  (gridSpacing*BIN_ROWS),10,FG_COLOR); // potenza del due in basso
            }

            for (int z = 0; z < BIN_ROWS; ++z)
            {
                DrawText  (TextFormat("%01d",z+1),bin_grid_XY.x - 28,bin_grid_XY.y + 12 + (z * gridSpacing),20, FG_COLOR);
                DrawText  (TextFormat("%01d",z+1),bin_grid_XY.x + gridSpacing*8 + 20,bin_grid_XY.y + 12 + (z * gridSpacing),20, FG_COLOR);
                DrawText  ("0x",hex_grid_XY.x - 32 , hex_grid_XY.y + 12 + (z * gridSpacing),20, FG_COLOR);
            }
          
          if (showGrid) draw_bin_grid (); // disegna o meno la griglia della matrice binaria
            
            //LoadLetter(curr_ascii_char); // 0) carica il current CHAR nella matrice binaria,
            drawBinCells(); // 1) disegna il carattere nella matrice dopo che è stato caricato in memoria al click del mouse sulla tabella ASCII
            draw_hex_grid(); // 2) disegna la matrice esadecimale

            BinToHex(); // 3) converti il valore binario di ogni riga nel corrispondente valore esadecimal (8 bit -> 1 byte 0x hex)
            printHexValues();  // 4) stampa nella matrice il valore esadecimale

            // 5) aggiorna in tempo reale la posizione della cella attuale ("cursore") quando mouse o tastiera si spostano sulle celle...
            DrawRectangle((int)bin_grid_XY.x + player.cell.x*gridSpacing, 
                          (int)bin_grid_XY.y + player.cell.y*gridSpacing, 
                          gridSpacing -1, 
                          gridSpacing -1,
                          matrice[j][i] ? ON_COLOR : OFF_COLOR); 
            
       
         // Close button
        btnQuitPressed  = GuiButton((Rectangle){ 600, 20, 24, 24}, "#113#");
        //  toolbar
        btnShowGridPressed   = GuiButton((Rectangle){ toolbar_XY.x, toolbar_XY.y, gridSpacing, gridSpacing }, "#97#");
        btnShiftUpPressed    = GuiButton((Rectangle){ toolbar_XY.x + 2 + gridSpacing * 1, toolbar_XY.y, gridSpacing, gridSpacing }, "#117#");
        btnShiftRightPressed = GuiButton((Rectangle){ toolbar_XY.x + 4 + gridSpacing * 2, toolbar_XY.y, gridSpacing, gridSpacing }, "#115#");
        btnShiftLeftPressed  = GuiButton((Rectangle){ toolbar_XY.x + 6 + gridSpacing * 3, toolbar_XY.y, gridSpacing, gridSpacing }, "#114#");
        btnShiftDownPressed  = GuiButton((Rectangle){ toolbar_XY.x + 8+ gridSpacing * 4, toolbar_XY.y, gridSpacing, gridSpacing }, "#116#");
        btnRotateLeft        = GuiButton((Rectangle){ toolbar_XY.x + 10 + gridSpacing * 5, toolbar_XY.y, gridSpacing, gridSpacing }, "#72#");
        btnRotateRight       = GuiButton((Rectangle){ toolbar_XY.x + 12 + gridSpacing * 6, toolbar_XY.y, gridSpacing, gridSpacing }, "#73#");
        btnInvertPressed     = GuiButton((Rectangle){ toolbar_XY.x + 14 + gridSpacing * 7, toolbar_XY.y, gridSpacing, gridSpacing }, "#94#");
        btnCopyPressed       = GuiButton((Rectangle){ toolbar_XY.x + 16 + gridSpacing * 8, toolbar_XY.y, gridSpacing, gridSpacing }, "#16#");
        btnPastePressed      = GuiButton((Rectangle){ toolbar_XY.x + 18 + gridSpacing * 9, toolbar_XY.y, gridSpacing, gridSpacing }, "#18#");
        btnRevertPressed     = GuiButton((Rectangle){ toolbar_XY.x + 20 + gridSpacing *10, toolbar_XY.y, gridSpacing, gridSpacing }, "#211#");
        btnClearPressed      = GuiButton((Rectangle){ toolbar_XY.x + 22 + gridSpacing *11, toolbar_XY.y, gridSpacing, gridSpacing }, "#143#");
        btnLoadPressed       = GuiButton((Rectangle){ toolbar_XY.x + 24 + gridSpacing *12, toolbar_XY.y, gridSpacing, gridSpacing }, "#1#");
        btnSavePressed       = GuiButton((Rectangle){ toolbar_XY.x + 26 + gridSpacing *13, toolbar_XY.y, gridSpacing, gridSpacing }, "#2#");


        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

