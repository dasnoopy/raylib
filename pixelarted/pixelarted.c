/*******************************************************************************************
*
*   PIXEL ART EDITOR
*   A simple pixel perfect editor  to learn C using raylib/raygui library
*
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************/

#define TOOL_NAME               "Pixel Art Editor"
#define TOOL_SHORT_NAME         "PixelArtEd"
#define TOOL_VERSION            "1.7.3"

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <raylib.h>

// barra path su Linux oppure su WIN
#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif


// raygui integration
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

const int screenWidth = 1080;
const int screenHeight = 732;

#define numCols       32
#define numRows       32
const int cellSize = 20;

#define MAX_COLORS_COUNT    24          // Number of colors available

//file management
#define MAX_FILES 512
#define MAX_NAME 256

 // initial X,Y coordinates for variuos interface elements
const Vector2 colorsBarPos = {40, 356};
const Vector2 spriteGridPos = {220, 48};
const Vector2 miniaturePos = {44,72};
const Vector2 toolbarPos = {40,188};
const Vector2 keyinfoPos = {912,620};
const Vector2 libraryPos = {906,20};
Rectangle scissorArea = { spriteGridPos.x,spriteGridPos.y, numCols*cellSize,numRows*cellSize };

// definizione matrici
int matrice[numRows][numCols];
int matriceUndo[numRows][numCols];
int matriceMirror[numRows][numCols];

// Definizione variabili
int selectedColor = 24;
int currentColor = 0;
int colorMouseHover = 0;
int miniatureSCALE= 3;
bool showGrid = true;
bool mouseHoverCells = false;
char fNAME[] = "default.pix";
char extfile[] = { "pix" };
bool fnameEditMode = false;
bool isEditing = false;
bool isDrawHmirr = false;
bool isDrawVmirr = false;
bool isFloodFill = false;
bool isColorRepl = false;
bool isDrawing = false;
// gestione stato load & save file
bool isLoading=false;
bool isSaving=false;
bool debug = false;
int px,py;

// custom Colors
#define myWHITE      CLITERAL(Color){ 255, 255, 255, 255 }   // White
#define myBLACK      CLITERAL(Color){ 14, 35, 46, 255 }         // Black
#define myBLANK      CLITERAL(Color){ 0, 0, 0, 0 }           // Blank (Transparent)
#define myYELLOW     CLITERAL(Color){ 255, 233, 3, 255 }     // Yellow / Giallo Modena Ferrari
#define myGOLD       CLITERAL(Color){ 239,191,4, 255 }     // Gold
#define myORANGE     CLITERAL(Color){ 255, 128, 0, 255 }     //  Orange Mclaren Papaya
#define myPINK       CLITERAL(Color){ 255, 192, 203, 255 }     //  Pink Panther
#define myRED        CLITERAL(Color){ 205, 33, 42, 255 }     //  Red /Rosso bandiera
#define myMAROON     CLITERAL(Color){ 148,34,34, 255 }     //  Maroon / Granata
#define myGREEN      CLITERAL(Color){ 141, 198, 84, 255 }      // Green
#define myLIME       CLITERAL(Color){ 70, 163, 41, 255 }      // Lime
#define myDARKGREEN  CLITERAL(Color){ 32, 104, 17, 255 }      // Dark Green /verde bandiera
#define mySKYBLUE    CLITERAL(Color){ 25, 174, 255, 255 }   // Sky Blue
#define myBLUE       CLITERAL(Color){ 0, 132, 200, 255 }     // Blue
#define myDARKBLUE   CLITERAL(Color){ 0, 92, 148, 255 }      // Dark Blue
#define myPURPLE     CLITERAL(Color){ 147,112,219, 255 }   // Purple
#define myVIOLET     CLITERAL(Color){ 112,74,191, 255 }    // Violet
#define myDARKPURPLE CLITERAL(Color){ 66,49,137, 255 }    // Dark Purple
#define myBEIGE      CLITERAL(Color){ 217,182,154, 255 }   // Beige
#define myBROWN      CLITERAL(Color){ 121,85,61, 255 }    // Brown
#define myDARKBROWN  CLITERAL(Color){ 73,55,43, 255 }      // Dark Brown
#define myLIGHTGRAY  CLITERAL(Color){ 189, 205, 212,255}   // Light Gray
#define myGRAY       CLITERAL(Color){ 111, 131, 136, 255 }   // Gray
#define myDARKGRAY   CLITERAL(Color){ 54, 78, 89, 255 }      // Dark Gray

// Colors to choose from
const Color colors[MAX_COLORS_COUNT] = {
        myBLANK, myWHITE,myYELLOW, myGOLD, myORANGE, myPINK, myRED, myMAROON, myGREEN, myLIME, myDARKGREEN,
        mySKYBLUE, myBLUE, myDARKBLUE, myPURPLE, myVIOLET, myDARKPURPLE, myBEIGE, myBROWN, myDARKBROWN,
        myLIGHTGRAY, myGRAY, myDARKGRAY, myBLACK };

// // Colors to choose from
// const Color colors[MAX_COLORS_COUNT] = {
//          BLANK, WHITE,YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
//          SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
//          LIGHTGRAY, GRAY, DARKGRAY, BLACK };

const char *colorNames[MAX_COLORS_COUNT] = { 
        "Blank","White", "Yellow", "Gold", "Orange", "Pink", "Red", "Maroon", "Green", "Lime", "DarkGreen",
        "SkyBlue", "Blue", "DarkBlue", "Purple", "Violet", "DarkPurple", "Beige", "Brown", "DarkBrown",
        "LightGray", "Gray", "DarkGray", "Black" };

// Define colorsRecs data
Rectangle colorsRecs[MAX_COLORS_COUNT] = { 0 };

// ARDUINO Matrix tool colors (light)
#define FG_COLOR CLITERAL(Color){ 55, 65, 70, 255}
#define BG_COLOR CLITERAL(Color){ 218, 227, 227, 255} 
// grid and checkerboard
#define GRID_COLOR CLITERAL(Color){ 155, 160, 163, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
#define CHECKB_COLOR CLITERAL(Color){ 244, 249, 249, 255} 
// some funs
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}
#define BORDER_COLOR CLITERAL(Color){ 20, 25, 25, 202} 

// bottoni toolbar
    // UI required variables
bool btnGrid = false;
bool btnNew = false;
bool btnClear = false;
bool btnHmirr = false;
bool btnVmirr = false;
bool btnShtUp = false;
bool btnShtDn = false;
bool btnShtSx = false;
bool btnShtDx = false;
bool btnRotate = false;
bool btnBruHor = false;
bool btnBruVer = false;
bool btnQuit = false;

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

//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------

// Draw checkerboard (default)
void drawCheckerboard(void)
{
        for (int row = 0; row < numRows-1; row+=2)
            for (int col = 0; col < numCols-1; col+=2)
                {
                DrawRectangleRec((Rectangle){spriteGridPos.x + ( col * cellSize), spriteGridPos.y + ( row * cellSize), cellSize, cellSize},CHECKB_COLOR);
                DrawRectangleRec((Rectangle){spriteGridPos.x + cellSize+ (col * cellSize), spriteGridPos.y + cellSize + (row * cellSize), cellSize, cellSize},CHECKB_COLOR);
                }
        DrawRectangleLinesEx(scissorArea,1,CHECKB_COLOR);
    }

// Draw grid lines 
void drawGridLines(void)
{
        for (int row = 0; row <= numRows; row+=1) // horizontal lines
            DrawLineEx((Vector2){spriteGridPos.x, spriteGridPos.y + (row * cellSize)},(Vector2){spriteGridPos.x + (numCols* cellSize), spriteGridPos.y + (row * cellSize)},1, BORDER_COLOR);
        for (int col = 0; col <= numCols; col+=1) //vertical lines
            DrawLineEx((Vector2){spriteGridPos.x + (col * cellSize), spriteGridPos.y}, (Vector2){spriteGridPos.x + (col * cellSize), spriteGridPos.y + (numRows*cellSize)},1, BORDER_COLOR);
        DrawRectangleLinesEx(scissorArea,1,BORDER_COLOR);
}

void drawSprite()
{    
            for (int row = 0; row < numRows; row++)
                {
                    for (int col = 0; col < numCols; col++)
                    {
                        // disegna sfondo cella  in base al valore 1/0
                        // se si cambia disegno qui, cambiare anche cursore nella sezione BeginDrawing
                        DrawRectangleRec((Rectangle){(spriteGridPos.x + (cellSize * col)) , (spriteGridPos.y + (cellSize * row)), 
                          cellSize, 
                          cellSize }, 
                          colors[matrice[row][col]]);

                    }
                }   
}

void drawThumbnail (void)
{
    // cornice e sfondo miniatura
        DrawRectangleLines(miniaturePos.x-2, miniaturePos.y-2 , 4+(numCols*miniatureSCALE), 4+(numRows*miniatureSCALE), BORDER_COLOR);
        DrawRectangle(miniaturePos.x,miniaturePos.y,numCols*miniatureSCALE,numRows*miniatureSCALE, GRID_BG_COLOR);
        //miniatura
        for (int row = 0; row < numRows; row++)
        {
            for (int col = 0; col < numCols; ++col)
            {
            DrawRectangle(miniaturePos.x + (miniatureSCALE * col), miniaturePos.y + (miniatureSCALE*row) ,miniatureSCALE,miniatureSCALE, colors[matrice[row][col]]);
            }
        }
}

// azzera matrice colore
void initGrid(void)
{
    for (int row = 0; row < numRows; row++)
        for (int col = 0; col < numCols; col++)
            matrice[row][col] = 0;
}
void showArrayVal(void)
{
    for (int row = 0; row < numRows; row++)
        for (int col = 0; col < numCols; col++) 
            DrawText(TextFormat("%2d",matrice[row][col]), 4 + (spriteGridPos.x + cellSize*col), 5 + (spriteGridPos.y + cellSize*row),10, FG_COLOR);
}

void replaceColor(int old, int new)
{
            for (int row = 0; row < numRows; row++) 
                for (int col = 0; col < numCols; col++) 
                    if (matrice[row][col] == old) matrice[row][col] = new;
}

void copyMatrix(int * src, int * dst, int N, int M)
{
    for(int i=0; i<N; i++)
        for(int j=0; j<M; j++) dst[(M*i)+j] = src[(M*i)+j];
}

void floodFill(int row, int col, int oldColor, int newColor)
{
    int checkRow; /* The positions we're checking currently */
    int checkCol;
    struct /* vectors for checking adjacent cells */
    {
        int dx;
        int dy;
    }   adjacent[] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};

    if(oldColor != newColor)
    {
        matrice[row][col] = newColor;
        for(int i = 0; i < 4; i++)
        {
            checkRow = row + adjacent[i].dx;
            checkCol = col + adjacent[i].dy;
            if((checkRow < numRows) && (checkCol < numCols)) /* within window boundaries */
            {
                if((checkRow >= 0) && (checkCol >= 0))
                {
                    if(matrice[checkRow][checkCol] == oldColor)
                    {
                        matrice[checkRow][checkCol] = newColor;
                        floodFill(checkRow, checkCol, oldColor, newColor);
                    }
                }
            }
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

            if (ext && strcmp(ext, ".pix") == 0) {
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
    SetConfigFlags (FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT); // occhio che sfalsa visualizzazione linee spessori colori...!!!
    InitWindow(screenWidth, screenHeight, "Pixel Art Editor");
        // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works

    // load TTF font with better antialiasing
    Font font = LoadFontEx("assets/OSD-Mono.ttf", 16, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    GuiSetIconScale(1);

    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);

 // camera2d settings only effect objects between BeginMode2D and EndMode2D
   Camera2D camera = { 0 };
    camera.target = (Vector2) { spriteGridPos.x, spriteGridPos.y};  // centro della zoom area
    camera.offset = (Vector2) { spriteGridPos.x, spriteGridPos.y};   // coordinate x origin zoom area
    camera.rotation = 0;        // rotation in deg
    camera.zoom     = 1.0f;      // magnification, i.e fov or zoom

// file open/save variables
    char files[MAX_FILES][MAX_NAME];
    int fileCount = load_files_recursive(".", files,0);
    int selected = 0;
    int scrollOffset = 0;
    const int itemHeight = 23;
    const int listHeight = itemHeight*18; // "numero" di file vizualizzati
    int visibleItems = listHeight / itemHeight;

// Define colorsRecs data (for every rectangle)
int offX=0;
int offY=0;
for (int i = 0; i < MAX_COLORS_COUNT; i++)
    {
    if (i % 3 == 0) {
        offX=0;
        offY++;
    }
    colorsRecs[i].x = colorsBarPos.x + 34.0f*offX + 2*offX;
    colorsRecs[i].y = colorsBarPos.y + 35.0f*offY;
    colorsRecs[i].width = 31;
    colorsRecs[i].height = 31;
    offX++;
    }

// Init player 0 (cursor for bin matrix)
    PlayerState player = { 0 };
    player.cell = (Point){ 0, 0 };

//reset matrice sprite e copia di backup
    initGrid();

// set FPS
SetTargetFPS(60);

while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        Vector2 mousePos = GetMousePosition();
        float speed = 150 * GetFrameTime();
        
        //------------------------------------------------------------------------------
        // Choose color with mouse from top color bar
        //------------------------------------------------------------------------------
        if (selectedColor >= MAX_COLORS_COUNT) selectedColor = MAX_COLORS_COUNT - 1;
        else if (selectedColor < 0) selectedColor = 0;

        for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            if (CheckCollisionPointRec(mousePos, colorsRecs[i]))
            {
                colorMouseHover = i;
                break;
            }
            else colorMouseHover = -1;
        }
        //LEFT MOUSE :select color
        if ((colorMouseHover >= 0) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selectedColor = colorMouseHover;

        //------------------------------------------------------------------------------
        // rileva se la posizione mouse e' dentro la matrice sprite...
        //------------------------------------------------------------------------------
        mouseHoverCells = CheckCollisionPointRec(mousePos,scissorArea);
        if (mouseHoverCells)
            {
                //HideCursor(); //hide os cursor inside matrix
                isEditing = true;

            //---------------------------------------------------------------------
            // ZOOM sprite con rotella mouse
            //----------------------------------------------------------------------

                float wheel = GetMouseWheelMove();
                if (wheel != 0) {
                    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
                    camera.offset = GetMousePosition();
                    camera.target = mouseWorld;
                    camera.zoom  += wheel * 0.2f * camera.zoom;   // proportional zoom
                    if (camera.zoom < 1.0f)
                        { 
                        camera.target = mousePos;
                        camera.zoom = 1.0f; // min. zoom x1
                        } 
                    if (camera.zoom > 5.0f) camera.zoom = 5.0f; // max. zoom 5x
            }

                 // Icon painting mouse logic
                Vector2 mouseWorldPos = GetScreenToWorld2D(mousePos, camera);

                player.cell.x = (mouseWorldPos.y - spriteGridPos.y) / cellSize ;
                player.cell.y = (mouseWorldPos.x - spriteGridPos.x) / cellSize;

                // Make sure player does not go out of bounds
                if (player.cell.x < 0) player.cell.x = 0;
                else if (player.cell.x >= numRows) player.cell.x = numRows - 1;
                if (player.cell.y < 0) player.cell.y = 0;
                else if (player.cell.y >= numCols) player.cell.y = numCols - 1;

                // aggiorna posizione "cursore" e relativo colore...
                px= player.cell.x;
                py= player.cell.y;
                currentColor = matrice[px][py];

                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) {
                        if (isDrawing) matrice[player.cell.x][player.cell.y] = selectedColor;
                        if (isDrawHmirr) {
                            matrice[player.cell.x][player.cell.y] = selectedColor;
                            matrice[numRows-player.cell.x-1][player.cell.y] = selectedColor;  
                        }
                        if (isDrawVmirr) {
                            matrice[player.cell.x][player.cell.y] = selectedColor;
                            matrice[player.cell.x][numCols-player.cell.y-1] = selectedColor;  
                        }
                        if (isColorRepl) replaceColor(currentColor, selectedColor);
                        if (isFloodFill) floodFill(px,py,currentColor,selectedColor);
                    }

                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) selectedColor = matrice[player.cell.x][player.cell.y];

            //-------------------------------------------------------------------
            // PAN SPRITE / ZOOM AREA with cursor KEY only if camera.zoom > 1.0f
            //-------------------------------------------------------------------
                if (camera.zoom > 1.0f)
                {
                    if (IsKeyDown(KEY_UP))
                    {
                        camera.target.y -= speed;
                         if (camera.target.y < spriteGridPos.y) camera.target.y = spriteGridPos.y;
                    }
                    if (IsKeyDown(KEY_DOWN))
                    {
                        camera.target.y += speed;
                         if (camera.target.y > spriteGridPos.y + (numRows*cellSize)) camera.target.y = spriteGridPos.y + (numRows*cellSize);
                    }
                    if (IsKeyDown(KEY_LEFT))
                    {
                        camera.target.x -= speed;
                         if (camera.target.x < spriteGridPos.x) camera.target.x = spriteGridPos.x;
                    }
                    if (IsKeyDown(KEY_RIGHT))
                    {
                        camera.target.x += speed;
                         if (camera.target.x > spriteGridPos.x + (numCols*cellSize)) camera.target.x = spriteGridPos.x + (numCols*cellSize);
                    }
                }
        }
        else { 
        ShowCursor(); //restore os cursor visibility outside matrix
        isEditing = false; } // fuori dalla matrice faccio quello che voglio!

        // ---------------------------------------------------------------------
        // some keybinding action to test functionality
        //----------------------------------------------------------------------
        if (!fnameEditMode) // se stò digitando il nome file nel riquadro di input, disabilita i keybindings
        {
            if (btnQuit || IsKeyPressed(KEY_Q)) break;
            else if (IsKeyPressed(KEY_D)) debug = !debug;
            else if (IsKeyPressed(KEY_Z)) copyMatrix(&matriceUndo[0][0], &matrice[0][0], numRows, numCols);
            else if (IsKeyPressed(KEY_S)) isSaving=true; //save file
            else if (btnGrid) showGrid = !showGrid;
            else if (btnNew)
            {
                    initGrid();
                    strcpy(fNAME,"default.pix");
                    //azzera matrice undo
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], numRows, numCols);
            }
            else if (btnClear)  //clear matrix 
                {
                    // fai sempre una copia di backup dello stato attuale della matrice
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], numRows, numCols);
                    initGrid();
                }

            else if (btnShtDx) // shift bin array right by 1
            {
                for (int row = 0; row < numRows; row++) // righe
                    {
                        // memorizza ultimo bit della riga
                        const int temp = matrice[row][numCols - 1];
                        for (int col = numCols-1; col > 0; col--) // colonne
                            matrice[row][col] = matrice[row][col-1];
                        // alla fine il "primo" bit prende il valore dell'ultimo
                        matrice[row][0] = temp;
                    }
            }
            else if (btnShtSx) // shift bin array left by 1
            {
                for (int row = 0; row < numRows; row++) // righe
                    {
                        // memorizza primo bit della riga
                        const int temp = matrice[row][0];
                        for (int col = 0; col < numCols-1; col++) // colonne
                            matrice[row][col] = matrice[row][col+1];
                        // alla fine ultimo bit prende il valore del prim
                        matrice[row][numCols-1] = temp;
                    }
            }
            else if (btnShtUp) // shift bin array up by 1
            {
                for (int col = 0; col < numCols; col++) // colonne
                    {
                        // memorizza prima riga
                        const int temp = matrice[0][col];
                            for (int row=0; row < numRows-1; row++) // righe
                                matrice[row][col] = matrice[row+1][col];
                        // ultima riga prende valori della prima
                        matrice[numRows - 1][col] = temp;
                    }
            }
            else if (btnShtDn) // shift bin array down by 1
             {
                    for (int col = 0; col < numCols; col++) // colonne
                        {
                            // memorizza stato ultima riga
                            const int temp = matrice[numRows - 1][col];
                            for (int row = numRows -1;  row > 0; row--) // righe
                                matrice[row][col] = matrice[row-1][col];
                            // prima riga prende valore ultima riga
                            matrice[0][col] = temp;
                        }
                }

            else if (btnRotate) // rotate matrix clockwise : works only for square matrix e.g 8x8, 16x16 and so on...
             {
                    // trasposizione  matrice binaria
                    for (int row = 0; row < numRows; row++) {
                        for (int col = row + 1 ; col < numCols ; col++) { 
                           int temp = matrice[col][row];
                           matrice[col][row] =  matrice[row][col];
                           matrice[row][col] = temp;
                        }
                    }
                    // poi ruota di 90° antiorario
                    for (int row = 0; row < numRows; row++) {
                        for (int col = 0,k = numCols -1; col<k; col++, k--) { 
                           int temp = matrice[row][col];
                           matrice[row][col] =  matrice[row][k];
                           matrice[row][k] = temp;
                        }
                    }
            }

                else if (btnVmirr) // vertical mirror
                {
                    copyMatrix(&matrice[0][0], &matriceMirror[0][0], numRows, numCols);
                    for (int row = 0; row < numRows; row++)
                        for (int col = 0; col < numCols; col++) {
                            matrice[row][numCols -1 - col] = matriceMirror[row][col];
                    }
                }

                else if (btnHmirr) // horizontal mirror
                {
                    copyMatrix(&matrice[0][0], &matriceMirror[0][0], numRows, numCols);
                    for (int row = 0; row < numRows; row++)
                        for (int col = 0; col < numCols; col++) {
                            matrice[numRows -1 - row][col] = matriceMirror[row][col];
                    }
                }

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
                            camera.zoom=1.0f;
                            isLoading=true;
                        }
                        // Clamp selezione
                        if (selected < 0) selected = 0;
                        if (selected >= fileCount) selected = fileCount - 1;
                        // Mantieni selezione visibile
                        if (selected < scrollOffset) scrollOffset = selected;
                        if (selected >= scrollOffset + visibleItems) scrollOffset = selected - visibleItems + 1;
                    }
        }       
//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------
    BeginTextureMode(target);
        ClearBackground(BG_COLOR);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawRectangle(spriteGridPos.x-34,spriteGridPos.y-36, numCols*cellSize + 68,numRows*cellSize + 68, GRID_BG_COLOR);  //sfondo checkerboard compreso intestazioni riga/colonna
        DrawRectangleLines(spriteGridPos.x-34,spriteGridPos.y-36, numCols*cellSize +68,numRows*cellSize + 68, BORDER_COLOR); //bordo attorno al rettangolo qui sopra!
        DrawText(TextFormat("%s", TOOL_SHORT_NAME), 38, 14, 20, FG_COLOR); 
        DrawText(TextFormat("version %s", TOOL_VERSION), 58, 38, 10, GRAY); 

        //----------------------------------------------------------------------
        // Draw color selection bar
        //-----------------------------------------------------------------------
        for (int i = 0; i < MAX_COLORS_COUNT; i++) {
            DrawRectangleRec(colorsRecs[i], colors[i]);
        //  riquadro attorno ai colori
            DrawRectangleLinesEx(colorsRecs[i],1,GRID_COLOR);
        // debug info
            if (debug)  DrawText(TextFormat("%2d",i), colorsRecs[i].x + 2 , colorsRecs[i].y + 4,10, FG_COLOR);
        }
        // passando sopra il colore rendilo piu chiaro
        if (colorMouseHover >= 0) DrawRectangleRec(colorsRecs[colorMouseHover], Fade(WHITE, 0.2f));
        // cliccando sul colore disegna riguadro attorno o sotto per evidenziare selezione
        DrawRectangleLinesEx((Rectangle){ colorsRecs[selectedColor].x-1, colorsRecs[selectedColor].y-1 ,
                            colorsRecs[selectedColor].width+2, colorsRecs[selectedColor].height+2},2, BORDER_COLOR);

        //----------------------------------------------------------------------
        // draw sprite and grid matrix inside scissor & camera2d area
        //----------------------------------------------------------------------
    BeginScissorMode((int)scissorArea.x, (int)scissorArea.y, (int)scissorArea.width, (int)scissorArea.height);     
        BeginMode2D(camera);
        drawCheckerboard(); // just to emulate a transparent background
        
        // Draw SPRITE AREA
        drawSprite(); // disegna immagine
        if (debug) showArrayVal();  // enable just to debug content of matrix

        //----------------------------------------------------------------------
        // Draw cursor moving when inside the sprite grid
        //----------------------------------------------------------------------
        DrawRectangleRec((Rectangle){ spriteGridPos.x + (py*cellSize), spriteGridPos.y + (px*cellSize), 
                          cellSize, 
                          cellSize},
                          Fade(BLACK, 0.5f));
        //----------------------------------------------------------------------
        // Draw crosshair (and hide grid)
        // ---------------------------------------------------------------------
        if (!showGrid) {
            //vertical
            DrawLineEx((Vector2){ spriteGridPos.x + (py*cellSize) + (cellSize/2), spriteGridPos.y }, 
                       (Vector2){ spriteGridPos.x + (py*cellSize) + (cellSize/2), spriteGridPos.y + (numRows*cellSize)  },
                       1, Fade(BLACK, 0.5f));
            // horizontal
            DrawLineEx((Vector2){ spriteGridPos.x, spriteGridPos.y  + (px*cellSize) + cellSize/2 }, 
                       (Vector2){ spriteGridPos.x + (numCols*cellSize) , spriteGridPos.y  + (px*cellSize) + (cellSize/2) },
                       1, Fade(BLACK, 0.5f));
            }
        // grid below sprite (if want grid above sprite move line just before EndMode2D)
        if (showGrid) drawGridLines();
        EndMode2D();
    EndScissorMode();

        //----------------------------------------------------------------------
        // draw rows and columns headers.
        //----------------------------------------------------------------------
        for (int row = 0; row < numRows; row++)
        {
        DrawTextEx(font, TextFormat("%02d",row),(Vector2){spriteGridPos.x - 18,(spriteGridPos.y + 4) + (row * cellSize)}, 9, 0, FG_COLOR); //sinistra
        DrawTextEx(font, TextFormat("%02d",row),(Vector2){spriteGridPos.x + numCols*cellSize +8,spriteGridPos.y + 4 + (row * cellSize)}, 9, 0, FG_COLOR);//destra
        }
        for (int col = 0; col < numCols; col++)
        {
        DrawTextEx(font,TextFormat("%02d",col),(Vector2){spriteGridPos.x + 4 + (col * cellSize),spriteGridPos.y -20},9,0,FG_COLOR);//sopra
        DrawTextEx(font,TextFormat("%02d",col),(Vector2){spriteGridPos.x + 4 + (col * cellSize),spriteGridPos.y + numRows*cellSize+8} ,9,0,FG_COLOR);//sotto
        }

        //----------------------------------------------------------------------
        // draw sprite miniature and colors info
        //----------------------------------------------------------------------
        drawThumbnail();
        //display cursor position and selected color info 
        // Draw x,y for current cell, zoom value
        DrawTextEx(font, TextFormat("x:%02i y:%02i z:%.02f",px,py,camera.zoom),(Vector2){colorsBarPos.x+ 10, colorsBarPos.y+346},12,0,FG_COLOR);
        // draw current color frame
        DrawRectangleLines(colorsBarPos.x-20  ,colorsBarPos.y + 342 , 22,22,BORDER_COLOR);
        DrawRectangle(colorsBarPos.x-18 ,colorsBarPos.y + 344, 18 , 18, colors[currentColor]);

        //----------------------------------------------------------------------
        // SAVE and LOAD in binary mode : to improve!
        //----------------------------------------------------------------------
        if (isSaving)
        {
            FILE *fSave = fopen(fNAME, "wb");
                if (fSave == NULL) {
                    printf("Impossibile scrivere il file [%s]!\n",fNAME);
                    return 1; }
            fwrite(matrice, sizeof(char), sizeof(matrice), fSave);
            fclose(fSave);
            // aggiorna files list
            fileCount = load_files_recursive(".", files,0);
            isSaving=false;
        }

        if (isLoading)
        {
            FILE *fLoad = fopen(fNAME, "rb"); 
            fread(matrice, sizeof(char), sizeof(matrice), fLoad);
            fclose(fLoad);
            isLoading=false;
            // fai subito una copia della matrice
            copyMatrix(&matrice[0][0], &matriceUndo[0][0], numRows, numCols);
        }

            // -----------------------------------------------------------------
            //  print file list / library 
            //------------------------------------------------------------------
            DrawText("Library",libraryPos.x, 14, 20, FG_COLOR);
            DrawRectangle(libraryPos.x,libraryPos.y + 30,160,listHeight,GRID_BG_COLOR);
            DrawRectangleLines(libraryPos.x,libraryPos.y + 30,160,listHeight,BORDER_COLOR);
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
            GuiLabel((Rectangle){ libraryPos.x+2, listHeight + 58, 160, 20 }, "Work file: ['S'] to save.");
            if (GuiTextBox((Rectangle){ libraryPos.x, listHeight + 78, 160, 28 }, fNAME, 256, fnameEditMode)) fnameEditMode = !fnameEditMode;


        // toolbar and messages
        int btnWidth = 32;
        int btnHeight = 32;
        GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
        // define button style
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,0xECF1F1FF);
        GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,0xECF1F1FF);
        GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,0x0CA1A6FF);
        // define TOGGLE styel
        GuiSetStyle(TOGGLE, TEXT_COLOR_NORMAL,0x000000FF);
        GuiSetStyle(TOGGLE, BASE_COLOR_NORMAL,0xECF1F1FF);
        GuiSetStyle(TOGGLE, TEXT_COLOR_PRESSED,0x000000FF);
        GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED,0x0CA1A6FF);

        //btnGrid = GuiButton((Rectangle){toolbarPos.x, toolbarPos.y, btnWidth, btnHeight }, "#50#");
        GuiToggle((Rectangle){ toolbarPos.x, toolbarPos.y , 32, 32 }, "#38#", &showGrid);
        btnNew = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y, btnWidth, btnHeight }, "#218#");
        btnClear = GuiButton((Rectangle){toolbarPos.x+72, toolbarPos.y, btnWidth, btnHeight }, "#63#");

        GuiToggle((Rectangle){ toolbarPos.x, toolbarPos.y + 36 , 32, 32 }, "#23#", &isDrawing);
            if (isDrawing) {
                isColorRepl=false;
                isFloodFill=false;
                isDrawHmirr=false;
                isDrawVmirr=false;
            }
        GuiToggle((Rectangle){ toolbarPos.x, toolbarPos.y + 72 , 32, 32 }, "#29#", &isFloodFill);
            if (isFloodFill) {
                isDrawing=false;
                isColorRepl=false;
                isDrawHmirr=false;
                isDrawVmirr=false;
            }
        GuiToggle((Rectangle){ toolbarPos.x+72, toolbarPos.y + 72 , 32, 32 }, "#94#", &isColorRepl);
            if (isColorRepl) {
                isDrawing=false;
                isFloodFill=false;
                isDrawHmirr=false;
                isDrawVmirr=false;
            }


        GuiToggle((Rectangle){ toolbarPos.x + 36, toolbarPos.y + 36 , 32, 32 }, "#85#", &isDrawHmirr);
            if (isDrawHmirr) {
                isDrawing=false;
                isDrawVmirr=false;
                isFloodFill=false;
                isColorRepl=false;
            }

        btnShtUp = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 72, btnWidth, btnHeight }, "#121#");

        GuiToggle((Rectangle){ toolbarPos.x+72, toolbarPos.y + 36 , 32, 32 }, "#83#", &isDrawVmirr);
            if (isDrawVmirr) {
                isDrawing=false;
                isDrawHmirr=false;
                isFloodFill=false;
                isColorRepl=false;
            }

        btnShtSx = GuiButton((Rectangle){toolbarPos.x, toolbarPos.y + 108, btnWidth, btnHeight }, "#118#");
        btnRotate = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 108, btnWidth, btnHeight }, "#77#");
        btnShtDx = GuiButton((Rectangle){toolbarPos.x +72, toolbarPos.y + 108, btnWidth, btnHeight }, "#119#");

        btnHmirr = GuiButton((Rectangle){toolbarPos.x , toolbarPos.y + 144, btnWidth, btnHeight }, "#41#");
        btnShtDn = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 144, btnWidth, btnHeight }, "#120#");
        btnVmirr = GuiButton((Rectangle){toolbarPos.x +72, toolbarPos.y + 144, btnWidth, btnHeight }, "#40#");

        GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+40, 140, 20 }, "Mousewheel to zoom in/out.");
        GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+60, 140, 20 }, "WinKey + mouse left to move.");
        GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+80, 140, 20 }, "['Q'] to quit program.");

        btnQuit = GuiButton((Rectangle){screenWidth-34,14,20,20}, "#128#");

        EndDrawing();
    }
    UnloadRenderTexture(target);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
