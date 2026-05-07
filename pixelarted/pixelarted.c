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
#define TOOL_VERSION            "1.4.7"

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
const int screenHeight = 768;

#define numCols       32
#define numRows       32
const int cellSize = 20;

#define MAX_COLORS_COUNT    24          // Number of colors available

//file management
#define MAX_FILES 512
#define MAX_NAME 256

 // initial X,Y coordinates for variuos interface elements
const Vector2 colorsBarPos = {192, 730};
const Vector2 spriteGridPos = {220, 48};
const Vector2 miniaturePos = {28,68};
const Vector2 toolbarPos = {40,204};
const Vector2 keyinfoPos = {24,610};
const Vector2 libraryPos = {906,20};
Rectangle scissorArea = { spriteGridPos.x,spriteGridPos.y, numCols*cellSize,numRows*cellSize };

// definizione matrici
int matrice[numRows][numCols];
int matriceUndo[numRows][numCols];
int matriceMirrorV[numRows][numCols];

// Definizione variabili
int selectedColor = 24;
int currentColor = 0;
int colorMouseHover = 0;
int miniatureSCALE= 4;
bool showGrid = true;
bool mouseHoverCells = false;
char fNAME[] = "library/default.pix";
char extfile[] = { "pix" };
bool fnameEditMode = false;
bool isEditing = false;

int px,py;


// custom Colors

#define myWHITE      CLITERAL(Color){ 255, 255, 255, 255 }   // White
#define myBLACK      CLITERAL(Color){ 14, 35, 46, 255 }         // Black
#define myBLANK      CLITERAL(Color){ 0, 0, 0, 0 }           // Blank (Transparent)
#define myYELLOW     CLITERAL(Color){ 255, 255, 62, 255 }     // Yellow
#define myGOLD       CLITERAL(Color){ 255, 192, 34, 255 }     // Gold
#define myORANGE     CLITERAL(Color){ 255, 112, 17, 255 }     //  Orange
#define myPINK       CLITERAL(Color){ 241, 202, 255, 255 }     //  Pink
#define myRED        CLITERAL(Color){ 220, 0, 0, 255 }     //  Red
#define myMAROON     CLITERAL(Color){ 181, 0, 0, 255 }     //  Maroon
#define myGREEN      CLITERAL(Color){ 204, 255, 66, 255 }      // Green
#define myLIME       CLITERAL(Color){ 154, 222, 0, 255 }      // Lime
#define myDARKGREEN  CLITERAL(Color){ 0, 145, 0, 255 }      // Dark Green
#define mySKYBLUE    CLITERAL(Color){ 25, 174, 255, 255 }   // Sky Blue
#define myBLUE       CLITERAL(Color){ 0, 132, 200, 255 }     // Blue
#define myDARKBLUE   CLITERAL(Color){ 0, 92, 148, 255 }      // Dark Blue
#define myPURPLE     CLITERAL(Color){ 215, 108, 255, 255 }   // Purple
#define myVIOLET     CLITERAL(Color){ 185, 0, 255, 255 }    // Violet
#define myDARKPURPLE CLITERAL(Color){ 112, 31, 126, 255 }    // Dark Purple
#define myBEIGE      CLITERAL(Color){ 205, 171, 143, 255 }   // Beige
#define myBROWN      CLITERAL(Color){ 184, 129, 0, 255 }    // Brown
#define myDARKBROWN  CLITERAL(Color){ 128, 77, 44, 255 }      // Dark Brown
#define myLIGHTGRAY  CLITERAL(Color){ 189, 205, 212,255}   // Light Gray
#define myGRAY       CLITERAL(Color){ 151, 171, 176, 255 }   // Gray
#define myDARKGRAY   CLITERAL(Color){ 54, 78, 89, 255 }      // Dark Gray

// Colors to choose from
const Color colors[MAX_COLORS_COUNT] = {
        myBLANK, myWHITE,myYELLOW, myGOLD, myORANGE, myPINK, myRED, myMAROON, myGREEN, myLIME, myDARKGREEN,
        mySKYBLUE, myBLUE, myDARKBLUE, myPURPLE, myVIOLET, myDARKPURPLE, myBEIGE, myBROWN, myDARKBROWN,
        myLIGHTGRAY, myGRAY, myDARKGRAY, myBLACK };

const char *colorNames[MAX_COLORS_COUNT] = { 
        "Blank","White", "Yellow", "Gold", "Orange", "Pink", "Red", "Maroon", "Green", "Lime", "DarkGreen",
        "SkyBlue", "Blue", "DarkBlue", "Purple", "Violet", "DarkPurple", "Beige", "Brown", "DarkBrown",
        "LightGray", "Gray", "DarkGray", "Black" };

// Define colorsRecs data
Rectangle colorsRecs[MAX_COLORS_COUNT] = { 0 };

// ARDUINO Matrix tool colors (light)
#define FG_COLOR CLITERAL(Color){ 55, 65, 70, 255}
#define BG_COLOR CLITERAL(Color){ 236, 241,241, 255} 
// grid and checkerboard
#define GRID_COLOR CLITERAL(Color){ 116, 126, 146, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 218, 227, 227, 255} 
#define CHECKB_COLOR CLITERAL(Color){ 249, 254, 254, 255} 
// some funs
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}
#define BORDER_COLOR CLITERAL(Color){ 131, 131, 131, 255} 

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
            DrawLineEx((Vector2){spriteGridPos.x, spriteGridPos.y + (row * cellSize)},(Vector2){spriteGridPos.x + (numCols* cellSize), spriteGridPos.y + (row * cellSize)},1, GRID_COLOR);
        for (int col = 0; col <= numCols; col+=1) //vertical lines
            DrawLineEx((Vector2){spriteGridPos.x + (col * cellSize), spriteGridPos.y}, (Vector2){spriteGridPos.x + (col * cellSize), spriteGridPos.y + (numRows*cellSize)},1, GRID_COLOR);
        DrawRectangleLinesEx(scissorArea,1,GRID_COLOR);
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
        DrawRectangle(miniaturePos.x,miniaturePos.y,numCols*miniatureSCALE,numRows*miniatureSCALE, BG_COLOR);
        DrawRectangleLines(miniaturePos.x-1, miniaturePos.y-1 , 2+(numCols*miniatureSCALE), 2+(numRows*miniatureSCALE), BORDER_COLOR);
    // intestazioni riga/colonna matrice colore e miniatura
        for (int row = 0; row < numRows; row++)
        {
            for (int col = 0; col < numCols; ++col)
            {
                DrawRectangle(miniaturePos.x + (miniatureSCALE * col), miniaturePos.y + (miniatureSCALE*row) ,miniatureSCALE ,miniatureSCALE, colors[matrice[row][col]]);
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
            DrawText(TextFormat("%2d",matrice[row][col]), 4 + (spriteGridPos.x + cellSize*col), 4 + (spriteGridPos.y + cellSize*row),10, ON_COLOR);
}

void replaceColor(int old, int new)
{
            for (int row = 0; row < numRows; row++) 
                for (int col = 0; col < numCols; col++) 
                {
                    if (matrice[row][col] == old) matrice[row][col] = new;
                }
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
    return strcmp(fa, fb);
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
                if (written >= 0 && written < MAX_NAME) {
                    count++;
                }
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
    Font font = LoadFontEx("assets/VictorMono-Bold.ttf", 18, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    // Set UI style
    // Custom GUI font isLoading
    //GuiSetFont(font);
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

    const int itemHeight = 25;
    const int listHeight = itemHeight*26; //altezza in pixel lista
    int visibleItems = listHeight / itemHeight;

    // gestione stato load & save file
    bool isLoading=false;
    bool isSaving=false;

// Define colorsRecs data (for every rectangle)
for (int i = 0; i < MAX_COLORS_COUNT; i++)
    {
    colorsRecs[i].x = colorsBarPos.x + 27.0f*i + 2*i;
    colorsRecs[i].y = colorsBarPos.y;
    colorsRecs[i].width = 26;
    colorsRecs[i].height = 26;
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

                // IMPROVE HERE make a better undo action
                //if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) // copia la matrice colori in una matrice copia per un succ. revert completo...
                  //copyMatrix(&matrice[0][0], &matriceUndo[0][0], numRows, numCols);

                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) matrice[player.cell.x][player.cell.y] = selectedColor;
                if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) selectedColor = matrice[player.cell.x][player.cell.y];


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

        // aggiorna posizione "cursore" e relativo colore...
        int px= player.cell.x;
        int py= player.cell.y;

        int currentColor = matrice[px][py];


        // ---------------------------------------------------------------------
        // some keybinding action to test functionality
        //----------------------------------------------------------------------
        if (!fnameEditMode) // se stò digitando il nome file nel riquadro di input, disabilita i keybindings
        {
            if (IsKeyPressed(KEY_Q)) break;
            else if (IsKeyPressed(KEY_Z)) copyMatrix(&matriceUndo[0][0], &matrice[0][0], numRows, numCols);
            else if (IsKeyPressed(KEY_S)) isSaving=true; //save file
            else if (IsKeyPressed(KEY_R)) //replace current color with selectec color
                {
                    // fai sempre una copia di backup dello stato attuale della matrice
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], numRows, numCols);
                    replaceColor(currentColor, selectedColor);
                }
            else if (IsKeyPressed(KEY_F)) // flood fill
            {
                // fai sempre una copia di backup dello stato attuale della matrice
                copyMatrix(&matrice[0][0], &matriceUndo[0][0], numRows, numCols);
                floodFill(px,py,currentColor,selectedColor);
            }
            else if (btnGrid) showGrid = !showGrid;

            else if (btnNew)
            {
                    initGrid();
                    strcpy(fNAME,"library/default.pix");
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
                    copyMatrix(&matrice[0][0], &matriceMirrorV[0][0], numRows, numCols);
                    for (int row = 0; row < numRows; row++)
                        for (int col = 0; col < numCols; col++) {
                            matrice[row][numCols -1 - col] = matriceMirrorV[row][col];
                    }
                }

                else if (btnHmirr) // horizontal mirror
                {
                    copyMatrix(&matrice[0][0], &matriceMirrorV[0][0], numRows, numCols);
                    for (int row = 0; row < numRows; row++)
                        for (int col = 0; col < numCols; col++) {
                            matrice[numRows -1 - row][col] = matriceMirrorV[row][col];
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
        ClearBackground(GRID_BG_COLOR);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(GRID_BG_COLOR);
        DrawRectangle(spriteGridPos.x-34,spriteGridPos.y-36, numCols*cellSize + 68,numRows*cellSize + 68, BG_COLOR);  //sfondo checkerboard compreso intestazioni riga/colonna
        DrawRectangleLines(spriteGridPos.x-34,spriteGridPos.y-36, numCols*cellSize +68,numRows*cellSize + 68, BORDER_COLOR); //bordo attorno al rettangolo qui sopra!
        DrawText(TextFormat("%s", TOOL_SHORT_NAME), 36, 14, 20, FG_COLOR); 
        DrawText(TextFormat("version %s", TOOL_VERSION), 52, 38, 10, GRAY); 

        //----------------------------------------------------------------------
        // Draw color selection bar
        //-----------------------------------------------------------------------
        for (int i = 0; i < MAX_COLORS_COUNT; i++) {
            DrawRectangleRec(colorsRecs[i], colors[i]);
        //  riquadro attorno al primo colore: BLANK (trasparente)
            DrawRectangleLinesEx(colorsRecs[i],1, BORDER_COLOR);
        }
        // passando sopra il colore rendilo piu chiaro
        if (colorMouseHover >= 0) DrawRectangleRec(colorsRecs[colorMouseHover], Fade(WHITE, 0.2f));
        // cliccando sul colore disegna riguadro attorno o sotto per evidenziare selezione
        DrawRectangleLinesEx((Rectangle){ colorsRecs[selectedColor].x-1, colorsRecs[selectedColor].y-1 ,
                            colorsRecs[selectedColor].width+2, colorsRecs[selectedColor].height +2},1, FG_COLOR);

        //----------------------------------------------------------------------
        // draw sprite and grid matrix inside scissor & camera2d area
        //----------------------------------------------------------------------
    BeginScissorMode((int)scissorArea.x, (int)scissorArea.y, (int)scissorArea.width, (int)scissorArea.height);     
        BeginMode2D(camera);
        drawCheckerboard(); // just to emulate a transparent background

        // Draw SPRITE AREA
        drawSprite(); // disegna immagine
        //showArrayVal();  // enable just to debug content of matrix

        //----------------------------------------------------------------------
        // Draw cursor moving when inside the sprite grid
        //----------------------------------------------------------------------
        DrawRectangleRec((Rectangle){ spriteGridPos.x + (py*cellSize), spriteGridPos.y + (px*cellSize), 
                          cellSize, 
                          cellSize},
                          Fade(ON_COLOR, 0.5f));

        //----------------------------------------------------------------------
        // Draw crosshair (and hide grid)
        // ---------------------------------------------------------------------
        if (!showGrid) {
            //vertical
            DrawLineEx((Vector2){ spriteGridPos.x + (py*cellSize) + (cellSize/2), spriteGridPos.y }, 
                       (Vector2){ spriteGridPos.x + (py*cellSize) + (cellSize/2), spriteGridPos.y + (numRows*cellSize)  },
                       1, Fade(ON_COLOR, 0.5f));
            // horizontal
            DrawLineEx((Vector2){ spriteGridPos.x, spriteGridPos.y  + (px*cellSize) + cellSize/2 }, 
                       (Vector2){ spriteGridPos.x + (numCols*cellSize) , spriteGridPos.y  + (px*cellSize) + (cellSize/2) },
                       1, Fade(ON_COLOR, 0.5f));
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
        DrawTextEx(font, TextFormat("%01d",row),(Vector2){spriteGridPos.x - 18,(spriteGridPos.y + 4) + (row * cellSize)}, 12, 0, FG_COLOR); //sinistra
        DrawTextEx(font, TextFormat("%01d",row),(Vector2){spriteGridPos.x + numCols*cellSize +8,spriteGridPos.y + 4 + (row * cellSize)}, 12, 0, FG_COLOR);//destra
        }
        for (int col = 0; col < numCols; col++)
        {
        DrawTextEx(font,TextFormat("%01d",col),(Vector2){spriteGridPos.x + 4 + (col * cellSize),spriteGridPos.y -20},12,0,FG_COLOR);//sopra
        DrawTextEx(font,TextFormat("%01d",col),(Vector2){spriteGridPos.x + 4 + (col * cellSize),spriteGridPos.y + numRows*cellSize+8} ,12,0,FG_COLOR);//sotto
        }

        //----------------------------------------------------------------------
        // draw sprite miniature and colors info
        //----------------------------------------------------------------------
        drawThumbnail();
        //display cursor position and selected color info 
        // Draw x,y for current cell, zoom value
        DrawTextEx(font, TextFormat("x:%02i y:%02i z:%.02f",px,py,camera.zoom),(Vector2){colorsBarPos.x-136, colorsBarPos.y+4},16,0,BLACK);
        // draw current color frame
        DrawRectangleLines(colorsBarPos.x -175  ,colorsBarPos.y , 26,26,BORDER_COLOR);
        DrawRectangle(colorsBarPos.x-174 ,colorsBarPos.y + 1, 24 , 24, colors[currentColor]);
        // Draw selected color frame
        //DrawRectangleLines(toolbarPos.x + 72 ,toolbarPos.y + 108 , 32,32,BORDER_COLOR);
        //DrawRectangle(toolbarPos.x + 73 ,toolbarPos.y + 109 , 30 , 30, colors[selectedColor]);



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
        }

            // -----------------------------------------------------------------
            //  draw file list / Work library 
            //------------------------------------------------------------------
            DrawText("Library",libraryPos.x+40, libraryPos.y-4, 20, FG_COLOR);
            DrawRectangle(libraryPos.x,libraryPos.y + 30,160,listHeight,BG_COLOR);
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
            
            GuiLabel((Rectangle){ libraryPos.x+2, listHeight + 58, 160, 20 }, "Work file: ['S'] to save.");
            if (GuiTextBox((Rectangle){ libraryPos.x, listHeight + 78, 160, 28 }, fNAME, 256, fnameEditMode)) fnameEditMode = !fnameEditMode;


        // toolbar and messages
        int btnWidth = 32;
        int btnHeight = 32;
        GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,0xECF1F1FF);
        GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,0xECF1F1FF);
        GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,0x0CA1A6FF);
        btnGrid = GuiButton((Rectangle){toolbarPos.x, toolbarPos.y, btnWidth, btnHeight }, "#97#");
        btnNew = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y, btnWidth, btnHeight }, "#218#");
        btnClear = GuiButton((Rectangle){toolbarPos.x+72, toolbarPos.y, btnWidth, btnHeight }, "#194#");

        btnHmirr = GuiButton((Rectangle){toolbarPos.x , toolbarPos.y + 36, btnWidth, btnHeight }, "#41#");
        btnShtUp = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 36, btnWidth, btnHeight }, "#121#");
        btnVmirr = GuiButton((Rectangle){toolbarPos.x +72, toolbarPos.y + 36, btnWidth, btnHeight }, "#40#");
        
        btnShtSx = GuiButton((Rectangle){toolbarPos.x, toolbarPos.y + 72, btnWidth, btnHeight }, "#118#");
        btnRotate = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 72, btnWidth, btnHeight }, "#60#");
        btnShtDx = GuiButton((Rectangle){toolbarPos.x +72, toolbarPos.y + 72, btnWidth, btnHeight }, "#119#");

        btnShtDn = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 108, btnWidth, btnHeight }, "#120#");

            GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y, 140, 20 }, "['R' ] to replace color.");
            GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+20, 140, 20 }, "['F'] to fill color area.");
            GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+40, 140, 20 }, "Mousewheel to zoom in/out.");
            GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+60, 140, 20 }, "WinKey + mouse left to move.");
            GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+80, 140, 20 }, "['Q'] to quit program.");

        EndDrawing();
    }
    UnloadRenderTexture(target);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
