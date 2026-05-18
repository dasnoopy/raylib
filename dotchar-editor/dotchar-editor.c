/*******************************************************************************************
*
*   DOT CHAR EDITOR
*   A simple app to learn C using raylib library

*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
*
*   TODO LIST POSSIBLE IMPROVEMENTS:
*       - consentire edit solo dei caratteri validi da 33 a 127
*       - gestione default.fnt e font.h  : 
*           - file missing x esempio 
*           - warning overwrite file font.data
*           - warning load che sovrascrive mappa caratteri attuale
*
*******************************************************************************************/

#define TOOL_NAME               "Dot Character Editor"
#define TOOL_SHORT_NAME         "DotCharEd"
#define TOOL_VERSION            "2.8.2"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
// raygui integration
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// load custom font to a 128x8 matrix
#include "custom_font.h"

// barra path su Linux oppure su WIN
#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

const int screenWidth = 1024;
const int screenHeight = 720;

 // initial X,Y coordinates for variuos interface elements
const Vector2 bin_grid_XY = {244, 60}; // x, y devono essere uguale o multiplo di gridSpacing ....
const Vector2 hex_grid_XY = {724, 60}; // posizione tabella esadecimale
const Vector2 toolbar_XY = { 40, 72}; // posizione toolbar
const Vector2 libraryPos = {844,28}; // posizione libreria file
// ASCII TABLE
const Vector2 ascii_grid_XY  = { 224, 408};

int curr_ascii_char = 32; //carattere corrente selezionato nella tabella ASCII : default iniziale "!"

#define gridSpacing       36
#define BIN_COLS        8// larghezza matrice binario (nr.colonne)del disegno  (1-8)
#define BIN_ROWS       8 // altezza matrice binaria( nr. righe) del singolo carattere (1-8)
#define HEX_VAL_X         2 // larghezza matrice esadecimale (nr. colonne) per nibble 1 e 2 (nibble = mezzo byte MSB e LSB)
#define HEX_VAL_Y         8 // deve corrispondere a CHAR_ROW : altezza matrice esadecimale( nr. righe) (1-8)

bool showGrid = true;

// definizione matrici
int matrice[BIN_COLS][BIN_ROWS];
char hex[HEX_VAL_X][HEX_VAL_Y];

// matrici copia per varie utilita
int revert_matrix[BIN_ROWS][BIN_COLS];
int revert_font[128][8];
int copypaste_matrix[BIN_ROWS][BIN_COLS];
int matrix_Mirror[BIN_ROWS][BIN_COLS];

// ARDUINO Matrix tool colors (light)
#define BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
#define FG_COLOR CLITERAL(Color){ 79, 88, 92, 255}
#define GRID_COLOR CLITERAL(Color){ 55, 66, 70, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 218, 227,227, 255} 
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}

// mouse and clipoard
bool mouseHoverCells = false;
bool mouseHoverASCII = false;

//file management
#define MAX_FILES 512
#define MAX_NAME 256
char fNAME[] = "default.fnt";
const char extfName[] = ".fnt";

// bottoni toolbar
    // UI required variables
    bool btnCopy = false;
    bool btnLoad = false;
    bool btnSave = false;
    bool btnPaste = false;
    bool btnClear = false;
    // toolbar
    bool btnShiftRight = false;
    bool btnShiftLeft = false;
    bool btnShiftDown = false;
    bool btnShiftUp = false;
    bool btnInvert = false;
    bool btnRotateLeft = false;
    bool btnRotateRight = false;
    bool btnRevertChar = false;
    bool btnMirrorH = false;
    bool btnMirrorV = false;
    bool btnRevertFont = false;
    bool btnQuit = false;

    // quando scrivo nome file non fare niente altro
    bool fnameEditMode = false;
    bool isEditing = false;
    // gestione stato load & save file
    bool isLoading=false;
    bool isSaving=false;
    bool saveDotH = false;

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
  float radius = 0.03f; // no radius
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
    // draw nice ASCII table background
    DrawRectangle(ascii_grid_XY.x -6, ascii_grid_XY.y -6 , 8 + gridSpacing*16, 8 + gridSpacing*8, RAYWHITE);
    DrawRectangleLines(ascii_grid_XY.x -6, ascii_grid_XY.y -6 , 8 + gridSpacing*16, 8 + gridSpacing*8, GRID_BG_COLOR);

    // sfondo grigio per evidenziare la lettera ASCII selezionata
    DrawRectangle(ascii_grid_XY.x + curr_ascii_char % 16 * gridSpacing -2 , ascii_grid_XY.y + curr_ascii_char/16 * gridSpacing -2 , gridSpacing-1, gridSpacing-1, (Color){ 188, 197, 197,255}); 
        
        for (int j = 0; j < 8; ++j)
        {
            for (int i = 0; i < 16; ++i)
            {
                x= ascii_grid_XY.x + (i*gridSpacing);
                y= ascii_grid_XY.y + (j*gridSpacing);
                drawLetter(x,y, count);
                DrawText(TextFormat("%d",count), x, y-1, 10 ,RED);
                count++;
            }
        }

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
// sfondo e cornice miniatura carattere
    DrawRectangle(bin_grid_XY.x + gridSpacing*10 -12, bin_grid_XY.y + gridSpacing*2 -4 , 71, 71, RAYWHITE);
    DrawRectangleLines(bin_grid_XY.x + gridSpacing*10 -12, bin_grid_XY.y + gridSpacing*2 -4 , 71, 71, GRID_BG_COLOR);
            
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

                          // mostra miniatura matrice per debug
                        DrawRectangle((bin_grid_XY.x + gridSpacing*10-8) + 8*j, (bin_grid_XY.y + gridSpacing*2) + 8*i,7,7, matrice[j][i] ? FG_COLOR : GRID_BG_COLOR);
                    }
                } 
    DrawText(TextFormat("Symbol: '%c'",curr_ascii_char),bin_grid_XY.x + gridSpacing*10 -8, bin_grid_XY.y + (gridSpacing*4) + 8, 10,FG_COLOR);
    DrawText(TextFormat("Dec: %i",curr_ascii_char),bin_grid_XY.x + gridSpacing*10 -8, bin_grid_XY.y + gridSpacing*5, 10,FG_COLOR);
    DrawText(TextFormat("Hex: %x",curr_ascii_char),bin_grid_XY.x + gridSpacing*10 -8, bin_grid_XY.y + gridSpacing*6-8, 10,FG_COLOR);
    DrawText("Size: 8 x 8",bin_grid_XY.x + gridSpacing*10 -8, bin_grid_XY.y + gridSpacing*7-16, 10,FG_COLOR);  
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

int compare_files(const void *a, const void *b) {
    const char *fa = (const char *)a;
    const char *fb = (const char *)b;
    return strcmp(fa, fb); // case sensitive
    // return strcasecmp((const char *)a, (const char *)b); // no case sensitive
}

int load_files_recursive(const char *path, char files[MAX_FILES][MAX_NAME], int count)
{
    DIR *dir = opendir(path);
    if (!dir) return count;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < MAX_FILES) {
        const char *name = entry->d_name;

        // evita "." e ".."
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, SEPARATOR, name);

        struct stat st;
        if (stat(fullpath, &st) == -1) continue;

        // se directory procedi con ricorsione
        if (S_ISDIR(st.st_mode)) count = load_files_recursive(fullpath, files, count);
        // se è file → controlla estensione
        else if (S_ISREG(st.st_mode)) {
            const char *ext = strrchr(name, '.');

            if (ext && strcmp(ext, extfName) == 0) {
                 int written = snprintf(files[count], MAX_NAME, "%s", fullpath);
                if (written >= 0 && written < MAX_NAME) count++;
            }
        }
    }
    closedir(dir);
    qsort(files, count, MAX_NAME, compare_files);  // ordinamento file
    return count;
}

int main (int argc, char *argv[])
{

    //SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(screenWidth, screenHeight, "DotChar Editor");
        // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);

    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    
    // General variables
    Vector2 mousePosition = { 0 };

    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);

    // Set UI style
    // Custom GUI font loading
    //Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    //GuiLoadStyle("assets/dce.rgs");
    //GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    //GuiSetIconScale(1);

    // Init player 0 (cursor for bin matrix)
    PlayerState player = { 0 };
    player.cell = (Point){ 0, 0 };

    // Init  player1 (cursor for ASCII table matrix)
    PlayerState player1 = { 0 };
    player1.cell = (Point){ 0, 0 };

// file open/save variables
    char files[MAX_FILES][MAX_NAME];
    int fileCount = load_files_recursive(".", files,0);
    int selected = 0;
    int scrollOffset = 0;
    const int itemHeight = 23;
    const int listHeight = itemHeight*18; // "numero" di file vizualizzati
    int visibleItems = listHeight / itemHeight;



    //reset matrice binaria
    reset_matrix();

    // copia la tabella caratteri ASCII un una matrice copia per un succ. revert completo...
     copy_matrix_2d(&TableFont[0][0], &revert_font[0][0], 128, 8);

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------

        if (btnRevertFont) 
        {   
            //ripristina copia orginale matrice TableFont ...
            strcpy(fNAME,"default.fnt");
            copy_matrix_2d(&revert_font[0][0], &TableFont[0][0], 128, 8);
            LoadLetter(curr_ascii_char);
            // copia di backup del carattere corrente
            copy_matrix_2d(&matrice[0][0], &revert_matrix[0][0], 8, 8);
        }
        if (btnRevertChar) copy_matrix_2d(&revert_matrix[0][0], &matrice[0][0], 8, 8);
        if (btnClear)
        {
            reset_matrix();
            player.cell.x = 0;
            player.cell.y = 0;
        }
        if (btnCopy) copy_matrix_2d(&matrice[0][0], &copypaste_matrix[0][0], 8, 8);
        if (btnPaste) copy_matrix_2d(&copypaste_matrix[0][0], &matrice[0][0], 8, 8);

        if (btnInvert)
        {
            // inverte matrice binaria
            for (int i = 0; i < BIN_ROWS; i++) {
                for (int j = 0; j < BIN_COLS; j++) { 
                    matrice[j][i] = !matrice[j][i]; } }
        }
        
        if (btnShiftRight) // shift bin array right by 1
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

        if (btnShiftLeft) // shift bin array left by 1
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

        if (btnShiftUp) // shift bin array down by 1
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
 
         if (btnShiftDown) // shift bin array up by 1
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

        if (btnMirrorH)
        {
            copy_matrix_2d(&matrice[0][0], &matrix_Mirror[0][0], 8, 8);
            for (int i = 0; i < BIN_ROWS; ++i)
            {
                for (int j = 0; j < BIN_COLS; j++) {
                    matrice[BIN_COLS -1- j][i] = matrix_Mirror[j][i];
                }
            }
        }

        if (btnMirrorV)
        {
            copy_matrix_2d(&matrice[0][0], &matrix_Mirror[0][0], 8, 8);
            for (int i = 0; i < BIN_ROWS; ++i)
            {
                for (int j = 0; j < BIN_COLS; j++) {
                    matrice[j][BIN_ROWS -1 -i] = matrix_Mirror[j][i];
                }
            }
        }

        
        //----------------------------------------------------------------------------------
        // Player movement logic using arrow keys
        mousePosition = GetMousePosition();
        
        // rileva se la posizione mouse e' dentro la matrice binaria...
        mouseHoverCells = CheckCollisionPointRec(mousePosition,(Rectangle){bin_grid_XY.x, bin_grid_XY.y,BIN_COLS*gridSpacing,BIN_ROWS*gridSpacing });
        mouseHoverASCII = CheckCollisionPointRec(mousePosition,(Rectangle){ascii_grid_XY.x, ascii_grid_XY.y,16*gridSpacing,8*gridSpacing});
            
            if (mouseHoverCells)
            {
                    isEditing = true;
                    if (IsKeyPressed(KEY_RIGHT)) player.cell.x++;
                    else if (IsKeyPressed(KEY_LEFT)) player.cell.x--;
                    else if (IsKeyPressed(KEY_UP)) player.cell.y--;
                    else if (IsKeyPressed(KEY_DOWN)) player.cell.y++;


                    // Make sure player does not go out of bounds
                    if (player.cell.x < 0) player.cell.x = 0;
                    else if (player.cell.x >= BIN_COLS) player.cell.x = BIN_COLS-1;
                    else if (player.cell.y < 0) player.cell.y = 0 ;
                    else if (player.cell.y >= BIN_ROWS) player.cell.y = BIN_ROWS-1;
                    // scrive bit 1/0 nella matrice binaria tasto sx /dx del mouse (1 o 0)
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) 
                 // Icon painting mouse logic
                    {
                    player.cell.x = (GetMouseX() - bin_grid_XY.x) / gridSpacing;
                    player.cell.y = (GetMouseY() - bin_grid_XY.y) / gridSpacing;
                    matrice[player.cell.x][player.cell.y] =!matrice[player.cell.x][player.cell.y];
                    }
            }
            else { isEditing = false; } // fuori dalla matrice faccio quello che voglio!
                    
            // rileva se la posizione mouse e' dentro la tabella ASCIIa...
            if (mouseHoverASCII)
            {
                 // Mouse logic over ASCII TABLE

                    player1.cell.x = (GetMouseX() - ascii_grid_XY.x) / gridSpacing;
                    player1.cell.y = (GetMouseY() - ascii_grid_XY.y) / gridSpacing;

                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
                    {
                        //fai sempre una copia di sicurezza del carattere selezionato per un successivo revert...
                        copy_matrix_2d(&matrice[0][0], &revert_matrix[0][0], 8, 8);
                        curr_ascii_char = player1.cell.x +(player1.cell.y *16);
                        LoadLetter(curr_ascii_char);
                    }
            }

        // aggiorna posizione "cursore" quando ci si sposta sulla matrice binaria con i tasto oppure il mouse
        int j= player.cell.x;
        int i= player.cell.y;

        // scrive bit 1/0 della cella selezionato della matrice binaria,  premendo la BARRA SPAZIO
        if (IsKeyPressed(KEY_SPACE)) matrice[player.cell.x][player.cell.y] = !matrice[player.cell.x][player.cell.y];
        if (btnQuit || IsKeyPressed(KEY_Q)) break;

        if (!fnameEditMode) // se stò digitando il nome file nel riquadro di input, disabilita i keybindings
        {
                //---------------------------------------------------------------------
                //  file LIbrary management
                //----------------------------------------------------------------------
                // Scroll con tastiera per sopstarsi tra i files (solo se si e' fuori dalla zona di editing)
                 if (!isEditing)
                    {
                        if (IsKeyPressed(KEY_DOWN)) selected++;
                        if (IsKeyPressed(KEY_UP)) selected--;
                        if (IsKeyPressed(KEY_ENTER) && fileCount > 0) {
                            strcpy(fNAME,files[selected]);
                            isLoading=true;
                        }
                        // Clamp selezione
                        if (selected < 0) selected = 0;
                        if (selected >= fileCount) selected = fileCount - 1;
                        // Mantieni selezione visibile
                        if (selected < scrollOffset) scrollOffset = selected;
                        if (selected >= scrollOffset + visibleItems) scrollOffset = selected - visibleItems + 1;
                    }
           if (IsKeyPressed(KEY_S)) isSaving=true; //save file
        }
        //----------------------------------------------------------------------------------
		// Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(BG_COLOR);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BG_COLOR);

            // draw round rectangle as "fake" background with some opacity
            DrawRectangle(0,0,200, screenHeight,GRID_BG_COLOR);
            DrawLine(200,0,200,screenHeight,LIGHTGRAY);
            DrawRectangle(screenWidth-200, 0,200, screenHeight,GRID_BG_COLOR);
            DrawLine(screenWidth-200,0,screenWidth-200,screenHeight,LIGHTGRAY);
            // stampa la tabella ASCII aggiornata
            drawASCII_Table();

            // some windows info e tricks
            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 42, 16, 20, FG_COLOR); 
            DrawText(TextFormat("version %s", TOOL_VERSION), 60, 40, 10, GRAY); 
            
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
        

        if (isSaving)
        {
                //salva binario
                FILE *fSave = fopen(fNAME, "wb");
                if (fSave == NULL) {
                    printf("Errore non definito durante salvataggio file!\n");
                    return 1; }
                fwrite(TableFont, sizeof(char), sizeof(TableFont), fSave);
                fclose(fSave);

                // // salva myFont.h
                // FILE *fp = fopen("font.h", "w");
                // if (fp == NULL) {
                //     printf("File [default.fnt] non trovato!\n");
                //     return 1;}
                
                // fprintf(fp, "// custom matrix font definition\n");
                // fprintf(fp, "int TableFont[128][8] = {\n");
                // for (int i = 0; i < 128; ++i) {
                //     fprintf(fp,"\t{");
                //     // scrivi i primi 7 byte nel formato 0x00,
                //     for (int j = 0; j < 7; ++j) fprintf(fp,"0x%02x, ",TableFont [i][j]);
                //     // scrivi ultimo byte riga "0x00}," e ultimissimo byte "0x00}" 
                //     if (i<127) fprintf(fp,"0x%02x}, // char: %i\n",TableFont[i][7],i);
                //     else fprintf(fp,"0x%02x} // char: 127\n",TableFont[127][7]);
                // }
                // fprintf(fp, "};\n");
                // fclose(fp);

                // aggiorna files list
                fileCount = load_files_recursive(".", files,0);
                isSaving=false;
        }

        if (isLoading)
        {

            FILE *fLoad = fopen(fNAME, "rb"); 
                if (fLoad == NULL) {
                    printf("File [default.fnt] non trovato!\n");
                    return 1;
                    // gestire file not found con finestra
                    }
            fread(TableFont, sizeof(char), sizeof(TableFont), fLoad);
            fclose(fLoad);

            // aggiorna carattere selezionato dopo load font table
            LoadLetter(curr_ascii_char);

            // copia di backup del carattere corrente
            copy_matrix_2d(&matrice[0][0], &revert_matrix[0][0], 8, 8);

            isLoading=false;
        }
    
            // -----------------------------------------------------------------
            //  print file list / library 
            //------------------------------------------------------------------
            DrawText("Library",libraryPos.x, 16, 20, FG_COLOR);
            DrawRectangle(libraryPos.x,libraryPos.y + 30,160,listHeight,BG_COLOR);
            DrawRectangleLines(libraryPos.x,libraryPos.y + 30,160,listHeight,GRID_COLOR);
            for (int i = 0; i < visibleItems; i++) {
                int index = i + scrollOffset;
                if (index >= fileCount) break;

                int y = libraryPos.y + 30 + i * itemHeight;
                Rectangle rect = {libraryPos.x, y, 160, itemHeight};
                    // Evidenzia file selezionato
                if (index == selected) {
                    DrawRectangleRec(rect, ON_COLOR);
                }   
                DrawText(files[index],libraryPos.x + 4 , y + 8, 10, BLACK);
            }
            //------------------------------------------------------------------            
        GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
            GuiLabel((Rectangle){ libraryPos.x+2, listHeight + 58, 160, 20 }, "Font file: ['S'] to save.");
            if (GuiTextBox((Rectangle){ libraryPos.x, listHeight + 78, 160, 28 }, fNAME, 256, fnameEditMode)) fnameEditMode = !fnameEditMode;
            
            GuiCheckBox((Rectangle){libraryPos.x, listHeight+112, 20, 20  }, "Create also [font].h", &saveDotH);
            btnRevertFont = GuiButton((Rectangle){ libraryPos.x,listHeight+160 , 160, 28 }, "Load default font"); 
       
        //  toolbar
        int btnWidth = 112;
        int btnHeight = 30;

        //GuiCheckBox((Rectangle){toolbar_XY.x +2 , toolbar_XY.y+8, 20, 20 }, "Show grid", &showGrid);

        btnShiftUp    = GuiButton((Rectangle){ toolbar_XY.x, toolbar_XY.y + gridSpacing*1, btnWidth, btnHeight}, "Shift up");
        btnShiftRight = GuiButton((Rectangle){ toolbar_XY.x, 4 + toolbar_XY.y + gridSpacing*2, btnWidth, btnHeight }, "Shift right");
        btnShiftLeft  = GuiButton((Rectangle){ toolbar_XY.x, 8 + toolbar_XY.y + gridSpacing*3, btnWidth, btnHeight }, "Shift left");
        btnShiftDown  = GuiButton((Rectangle){ toolbar_XY.x,12 + toolbar_XY.y + gridSpacing*4, btnWidth, btnHeight }, "Shift down");
        btnRotateLeft = GuiButton((Rectangle){ toolbar_XY.x,16 + toolbar_XY.y + gridSpacing*5, btnWidth, btnHeight }, "Rotate left");
        btnRotateRight= GuiButton((Rectangle){ toolbar_XY.x,20 + toolbar_XY.y + gridSpacing*6, btnWidth, btnHeight }, "Rotate right");
        btnMirrorH    = GuiButton((Rectangle){ toolbar_XY.x,24 + toolbar_XY.y + gridSpacing*7, btnWidth, btnHeight }, "Horiz. mirror");
        btnMirrorV    = GuiButton((Rectangle){ toolbar_XY.x,28 + toolbar_XY.y + gridSpacing*8, btnWidth, btnHeight }, "Vert. mirror");
        btnInvert     = GuiButton((Rectangle){ toolbar_XY.x,32 + toolbar_XY.y + gridSpacing*9, btnWidth, btnHeight }, "Invert dots");
        btnCopy       = GuiButton((Rectangle){ toolbar_XY.x,36 + toolbar_XY.y + gridSpacing*10, btnWidth, btnHeight }, "Copy char");
        btnPaste      = GuiButton((Rectangle){ toolbar_XY.x,40 + toolbar_XY.y + gridSpacing*11, btnWidth, btnHeight }, "Paste char");
        btnRevertChar = GuiButton((Rectangle){ toolbar_XY.x,44 + toolbar_XY.y + gridSpacing*12, btnWidth, btnHeight }, "Revert char");
        btnClear      = GuiButton((Rectangle){ toolbar_XY.x,48 + toolbar_XY.y + gridSpacing*13, btnWidth, btnHeight }, "Delete char");

         btnQuit = GuiButton((Rectangle){screenWidth-34,14,20,20}, "#128#");
        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

