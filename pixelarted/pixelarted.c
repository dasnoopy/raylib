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
#define TOOL_VERSION            "1.2.1"

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

#define BIN_COLS       32
#define BIN_ROWS       32
#define MAX_COLORS_COUNT    24          // Number of colors available
const int gridSpacing = 20;

//file management
#define MAX_FILES 512
#define MAX_NAME 256


 // initial X,Y coordinates for variuos interface elements
Vector2 colorsBarPos = {192, 12};
Vector2 spriteGridPos = {220, 86};
Vector2 miniaturePos = {28,64};
Vector2 panelBarPos = {24,300};
Vector2 panel2BarPos = {24,690};
Vector2 libraryPos = {906,20};
Rectangle scissorArea = { 220,86, BIN_COLS*gridSpacing,BIN_ROWS*gridSpacing };

// definizione matrici
int matrice[BIN_COLS][BIN_ROWS];
int matriceUndo[BIN_ROWS][BIN_COLS];
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
bool keyBinding = true;
bool isEditing = false;

// Some Basic Colors
// NOTE: Custom raylib color palette for amazing visuals on WHITE background
#define LIGHTGRAY  CLITERAL(Color){ 200, 200, 200, 255 }   // Light Gray
#define GRAY       CLITERAL(Color){ 130, 130, 130, 255 }   // Gray
#define DARKGRAY   CLITERAL(Color){ 80, 80, 80, 255 }      // Dark Gray

#define GOLD       CLITERAL(Color){ 255, 203, 0, 255 }     // Gold
#define ORANGE     CLITERAL(Color){ 255, 161, 0, 255 }     // Orange
#define PINK       CLITERAL(Color){ 255, 109, 194, 255 }   // Pink
#define RED        CLITERAL(Color){ 230, 41, 55, 255 }     // Red
#define MAROON     CLITERAL(Color){ 190, 33, 55, 255 }     // Maroon
#define GREEN      CLITERAL(Color){ 0, 228, 48, 255 }      // Green
#define LIME       CLITERAL(Color){ 0, 158, 47, 255 }      // Lime
#define DARKGREEN  CLITERAL(Color){ 0, 117, 44, 255 }      // Dark Green
#define SKYBLUE    CLITERAL(Color){ 102, 191, 255, 255 }   // Sky Blue
#define BLUE       CLITERAL(Color){ 0, 121, 241, 255 }     // Blue
#define DARKBLUE   CLITERAL(Color){ 0, 82, 172, 255 }      // Dark Blue
#define PURPLE     CLITERAL(Color){ 200, 122, 255, 255 }   // Purple
#define VIOLET     CLITERAL(Color){ 135, 60, 190, 255 }    // Violet
#define DARKPURPLE CLITERAL(Color){ 112, 31, 126, 255 }    // Dark Purple
#define BEIGE      CLITERAL(Color){ 211, 176, 131, 255 }   // Beige
#define BROWN      CLITERAL(Color){ 127, 106, 79, 255 }    // Brown
#define DARKBROWN  CLITERAL(Color){ 76, 63, 47, 255 }      // Dark Brown

// Colors to choose from
const Color colors[MAX_COLORS_COUNT] = {
        BLANK, WHITE,YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
        SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
        LIGHTGRAY, GRAY, DARKGRAY, BLACK };


// Custom color Palette
// #define MYWHITE      CLITERAL(Color){ 255, 255, 255, 255 }   // White
// #define MYBLACK      CLITERAL(Color){ 26, 21, 39, 255 }         // Black
// #define MYBLANK      CLITERAL(Color){ 0, 0, 0, 0 }           // Blank (Transparent)
// #define MYYELLOW     CLITERAL(Color){ 249, 240, 107, 255 }     // Yellow
// #define MYGOLD       CLITERAL(Color){ 245, 194, 17, 255 }     // Gold
// #define MYORANGE     CLITERAL(Color){ 255, 120, 0, 255 }     //  Orange
// #define MYPINK       CLITERAL(Color){ 255, 127, 157, 255 }     //  Pink
// #define MYRED        CLITERAL(Color){ 237, 51, 59, 255 }     //  Red
// #define MYMAROON     CLITERAL(Color){ 165, 29, 45, 255 }     //  Maroon
// #define MYGREEN      CLITERAL(Color){ 143, 240, 164, 255 }      // Green
// #define MYLIME       CLITERAL(Color){ 51, 209, 122, 255 }      // Lime
// #define MYDARKGREEN  CLITERAL(Color){ 0, 162, 105, 255 }      // Dark Green
// #define MYSKYBLUE    CLITERAL(Color){ 153, 193, 241, 255 }   // Sky Blue
// #define MYBLUE       CLITERAL(Color){ 53, 132, 228, 255 }     // Blue
// #define MYDARKBLUE   CLITERAL(Color){ 26, 95, 180, 255 }      // Dark Blue
// #define MYPURPLE     CLITERAL(Color){ 200, 122, 255, 255 }   // Purple
// #define MYVIOLET     CLITERAL(Color){ 135, 60, 190, 255 }    // Violet
// #define MYDARKPURPLE CLITERAL(Color){ 112, 31, 126, 255 }    // Dark Purple
// #define MYBEIGE      CLITERAL(Color){ 205, 171, 143, 255 }   // Beige
// #define MYBROWN      CLITERAL(Color){ 152, 106, 68, 255 }    // Brown
// #define MYDARKBROWN  CLITERAL(Color){ 99, 69, 44, 255 }      // Dark Brown
// #define MYLIGHTGRAY  CLITERAL(Color){ 192, 191, 188,255}   // Light Gray
// #define MYGRAY       CLITERAL(Color){ 154, 153, 150, 255 }   // Gray
// #define MYDARKGRAY   CLITERAL(Color){ 94, 92, 100, 255 }      // Dark Gray

// // Colors to choose from
// const Color colors[MAX_COLORS_COUNT] = {
//         MYBLANK, MYWHITE,MYYELLOW, MYGOLD, MYORANGE, MYPINK, MYRED, MYMAROON, MYGREEN, MYLIME, MYDARKGREEN,
//         MYSKYBLUE, MYBLUE, MYDARKBLUE, MYPURPLE, MYVIOLET, MYDARKPURPLE, MYBEIGE, MYBROWN, MYDARKBROWN,
//         MYLIGHTGRAY, MYGRAY, MYDARKGRAY, MYBLACK };

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
        for (int y = 0; y < BIN_ROWS-1; y+=2)
            for (int x = 0; x < BIN_COLS-1; x+=2)
                {
                DrawRectangleRec((Rectangle){spriteGridPos.x + (x*gridSpacing), spriteGridPos.y + (y*gridSpacing), gridSpacing, gridSpacing},CHECKB_COLOR);
                DrawRectangleRec((Rectangle){spriteGridPos.x + gridSpacing+ (x*gridSpacing), spriteGridPos.y + gridSpacing + (y*gridSpacing), gridSpacing, gridSpacing},CHECKB_COLOR);
                }
        DrawRectangleLinesEx(scissorArea,1,CHECKB_COLOR);
    }

// Draw grid lines 
void drawGridLines(void)
{
        for (int i = 0; i <= BIN_ROWS; i+=1)
            DrawLineEx((Vector2){spriteGridPos.x, spriteGridPos.y + (i * gridSpacing)},(Vector2){spriteGridPos.x + (BIN_COLS* gridSpacing), spriteGridPos.y + i*gridSpacing},1, GRID_COLOR);
        for (int j = 0; j <= BIN_COLS; j+=1)
            DrawLineEx((Vector2){spriteGridPos.x + (j * gridSpacing), spriteGridPos.y}, (Vector2){spriteGridPos.x + (j * gridSpacing), spriteGridPos.y + (BIN_ROWS*gridSpacing)},1, GRID_COLOR);
        DrawRectangleLinesEx(scissorArea,1,GRID_COLOR);
}

void drawSprite()
{    
            for (int i = 0; i < BIN_ROWS; i++)
                {
                    for (int j = 0; j < BIN_COLS; j++)
                    {
                        // disegna sfondo cella  in base al valore 1/0
                        // se si cambia disegno qui, cambiare anche cursore nella sezione BeginDrawing
                        DrawRectangleRec((Rectangle){(spriteGridPos.x + gridSpacing*j) , (spriteGridPos.y + gridSpacing*i), 
                          gridSpacing, 
                          gridSpacing }, 
                          colors[matrice[j][i]]);
                         //DrawText(TextFormat("%02i",matrice[j][i]),2+spriteGridPos.x + gridSpacing*j,2+spriteGridPos.y + gridSpacing*i,10,WHITE);
                    }
                }   
}

void drawThumbnail (void)
{
    // cornice e sfondo miniatura
        DrawRectangle(miniaturePos.x,miniaturePos.y,BIN_COLS*miniatureSCALE,BIN_ROWS*miniatureSCALE, BG_COLOR);
        DrawRectangleLines(miniaturePos.x, miniaturePos.y , (BIN_COLS*miniatureSCALE), (BIN_ROWS*miniatureSCALE), BORDER_COLOR);
    // intestazioni riga/colonna matrice colore e miniatura
        for (int i = 0; i < BIN_ROWS; i++)
        {
            for (int j = 0; j < BIN_COLS; ++j)
            {
                DrawRectangle(miniaturePos.x + (miniatureSCALE*j), miniaturePos.y + (miniatureSCALE*i) ,miniatureSCALE ,miniatureSCALE, colors[matrice[j][i]]);
            }
        }
}

// azzera matrice colore
void resetSprite(void)
{
    for (int i = 0; i < BIN_ROWS; i++)
        for (int j = 0; j < BIN_COLS; j++) matrice[j][i] = 0;
}

void replaceColor(int old, int new)
{
            for (int i = 0; i < BIN_ROWS; i++) 
                for (int j = 0; j < BIN_COLS; j++) 
                {
                    if (matrice[j][i] == old) matrice[j][i] = new;
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
            if((checkRow < BIN_ROWS) && (checkCol < BIN_COLS)) /* within window boundaries */
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

    // loaad TTF font with better antialiasing
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

    const int itemHeight = 24;
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
    resetSprite();

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

                player.cell.x = (mouseWorldPos.x - spriteGridPos.x) / gridSpacing ;
                player.cell.y = (mouseWorldPos.y - spriteGridPos.y) / gridSpacing;


                // Make sure player does not go out of bounds
                if (player.cell.x < 0) player.cell.x = 0;
                if (player.cell.x >= BIN_COLS) player.cell.x = BIN_COLS - 1;
                if (player.cell.y < 0) player.cell.y = 0;
                if (player.cell.y >= BIN_ROWS) player.cell.y = BIN_ROWS - 1;

                // IMPROVE HERE make a better undo action
                //if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) // copia la matrice colori in una matrice copia per un succ. revert completo...
                  //copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);

                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) matrice[player.cell.x][player.cell.y] = selectedColor;
                if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) matrice[player.cell.x][player.cell.y] = 0;

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
                         if (camera.target.y > spriteGridPos.y + (BIN_ROWS*gridSpacing)) camera.target.y = spriteGridPos.y + (BIN_ROWS*gridSpacing);
                    }
                    if (IsKeyDown(KEY_LEFT))
                    {
                        camera.target.x -= speed;
                         if (camera.target.x < spriteGridPos.x) camera.target.x = spriteGridPos.x;
                    }
                    if (IsKeyDown(KEY_RIGHT))
                    {
                        camera.target.x += speed;
                         if (camera.target.x > spriteGridPos.x + (BIN_COLS*gridSpacing)) camera.target.x = spriteGridPos.x + (BIN_COLS*gridSpacing);
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
            if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
            if (IsKeyPressed(KEY_Q)) break;
            else if (IsKeyPressed(KEY_N))
            {
                     // fai sempre una copia di backup dello stato attuale della matrice
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);
                    resetSprite();
                    strcpy(fNAME,"library/default.pix");
            }
            else if (IsKeyPressed(KEY_C)) 
                {
                    // fai sempre una copia di backup dello stato attuale della matrice
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);
                    resetSprite();
                }
            else if (IsKeyPressed(KEY_R)) 
                {
                    // fai sempre una copia di backup dello stato attuale della matrice
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);
                    replaceColor(currentColor, selectedColor);
                }
            else if (IsKeyPressed(KEY_Z)) copyMatrix(&matriceUndo[0][0], &matrice[0][0], BIN_ROWS, BIN_COLS);
            else if (IsKeyPressed(KEY_S)) isSaving=true;
            else if (IsKeyPressed(KEY_F))
            {
                // fai sempre una copia di backup dello stato attuale della matrice
                copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);
                floodFill(px,py,currentColor,selectedColor);
            }
            else if (IsKeyPressed(KEY_D)) // shift bin array right by 1
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
            else if (IsKeyPressed(KEY_A))// shift bin array left by 1
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
            else if (IsKeyPressed(KEY_W)) // shift bin array down by 1
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
            else if (IsKeyPressed(KEY_X))// shift bin array up by 1
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
        }       
//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------
    BeginTextureMode(target);
        ClearBackground(GRID_BG_COLOR);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(GRID_BG_COLOR);
        DrawRectangle(186,50, BIN_ROWS*gridSpacing + 68,screenHeight, BG_COLOR);  //sfondo checkerboard compreso intestazioni riga/colonnaa
        //DrawRectangleLines(186,50, BIN_ROWS*gridSpacing +68,screenHeight, BORDER_COLOR); //bordo attorno al rettangolo qui sopra!
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

        // //Draw cursor moving when inside the sprite grid        
        // DrawRectangleRec((Rectangle){ spriteGridPos.x + (px*gridSpacing), spriteGridPos.y + (py*gridSpacing), 
        //                   gridSpacing, 
        //                   gridSpacing},
        //                   Fade(ON_COLOR, 0.5f));

        //----------------------------------------------------------------------
        // Draw crosshair (and hide grid)
        // ---------------------------------------------------------------------
        if (!showGrid) {
            //vertical
            DrawLineEx((Vector2){ spriteGridPos.x + (px*gridSpacing) + (gridSpacing/2), spriteGridPos.y }, 
                       (Vector2){ spriteGridPos.x + (px*gridSpacing) + (gridSpacing/2), spriteGridPos.y + (BIN_ROWS*gridSpacing)  },
                       1, Fade(ON_COLOR, 0.5f));
            // horizontal
            DrawLineEx((Vector2){ spriteGridPos.x, spriteGridPos.y  + (py*gridSpacing) + gridSpacing/2 }, 
                       (Vector2){ spriteGridPos.x + (BIN_COLS*gridSpacing) , spriteGridPos.y  + (py*gridSpacing) + (gridSpacing/2) },
                       1, Fade(ON_COLOR, 0.5f));
            }

        // grid below sprite (if want grid above sprite move line just before EndMode2D)
        if (showGrid) drawGridLines();
        
        EndMode2D();
    EndScissorMode();

        //----------------------------------------------------------------------
        // draw rows and columns headers.
        //----------------------------------------------------------------------
        for (int i = 0; i < BIN_ROWS; i+=1)
        {
        DrawTextEx(font, TextFormat("%01d",i),(Vector2){spriteGridPos.x - 18,(spriteGridPos.y + 4) + (i * gridSpacing)}, 12, 0, FG_COLOR); //sinistra
        DrawTextEx(font, TextFormat("%01d",i),(Vector2){spriteGridPos.x + BIN_COLS*gridSpacing +8,spriteGridPos.y + 4 + (i * gridSpacing)}, 12, 0, FG_COLOR);//destra
        }
        for (int j = 0; j < BIN_COLS; j+=1)
        {
        DrawTextEx(font,TextFormat("%01d",j),(Vector2){spriteGridPos.x + 4 + (j * gridSpacing),spriteGridPos.y -20},12,0,FG_COLOR);//sopra
        DrawTextEx(font,TextFormat("%01d",j),(Vector2){spriteGridPos.x + 4 + (j * gridSpacing),spriteGridPos.y + BIN_ROWS*gridSpacing+8} ,12,0,FG_COLOR);//sotto
        }
        //----------------------------------------------------------------------
        // draw sprite miniature and colors info
        //----------------------------------------------------------------------
        drawThumbnail();
        //display cursor position and selected color info 
        // Draw x,y info
        DrawTextEx(font, TextFormat("x:%02i y:%02i [x%.02f]",px,py,camera.zoom),(Vector2){miniaturePos.x-4,miniaturePos.y+136},18,0,FG_COLOR);
        // draw current color frame
        DrawRectangleLines(miniaturePos.x  ,miniaturePos.y+168 , 36,36,BORDER_COLOR);
        DrawRectangle(miniaturePos.x + 2 ,miniaturePos.y+170 , 32 , 32, colors[currentColor]);
        DrawTextEx(font,colorNames[currentColor],(Vector2){miniaturePos.x + 48, miniaturePos.y + 170},18,0, BLACK);
        DrawTextEx(font,"Current color",(Vector2){miniaturePos.x + 48, miniaturePos.y + 190},12,0,FG_COLOR);
        // Draw selected color frame
        DrawRectangleLines(miniaturePos.x  ,miniaturePos.y + 210 , 36,36,BORDER_COLOR);
        DrawRectangle(miniaturePos.x +2,miniaturePos.y + 212 , 32 , 32, colors[selectedColor]);
        DrawTextEx(font,colorNames[selectedColor],(Vector2){miniaturePos.x + 48, miniaturePos.y + 212},18,0,BLACK);
        DrawTextEx(font,"Selected color",(Vector2){miniaturePos.x + 48, miniaturePos.y + 232},12,0,FG_COLOR);


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
            DrawText("Art Library",libraryPos.x+16, libraryPos.y-4, 20, FG_COLOR);
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
                GuiLabel((Rectangle){ libraryPos.x+2, listHeight + 56, 130, 20 }, "Current file:");
                if(GuiTextBox((Rectangle){ libraryPos.x, listHeight + 76, 160, 28 }, fNAME, 256, fnameEditMode)) fnameEditMode = !fnameEditMode;
                GuiLabel((Rectangle){ libraryPos.x+2, listHeight+106, 160, 20 }, "Press 'S' to save matrix.");
            //------------------------------------------------------------------
            // draw panel bar
            GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
            //Keybinding label
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+40, 140, 20 }, "Press 'G' to show/hide grid.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+60, 140, 20 }, "Press 'C' to clear all.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+80, 140, 20 }, "Press 'R' to replace color.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+100, 140, 20 }, "Press 'A' to shift matrix left.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+120, 140, 20 }, "Press 'D' to shit matrix right.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+140, 140, 20 }, "Press 'W' to shift matrix up.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+160, 140, 20 }, "Press 'X' to shift matrix down.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+180, 140, 20 }, "Press 'Z' to undo last action.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+200, 140, 20 }, "Press 'F' to fill color area.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+220, 140, 20 }, "Press 'N' to create new default file.");

            GuiLabel((Rectangle){ panel2BarPos.x, panel2BarPos.y, 140, 20 }, "Mousewheel to zoom in/out.");
            GuiLabel((Rectangle){ panel2BarPos.x, panel2BarPos.y+20, 140, 20 }, "WinKey + mouse left to move.");
            GuiLabel((Rectangle){ panel2BarPos.x, panel2BarPos.y+40, 140, 20 }, "Press 'Q' to quit program.");
            
        EndDrawing();
    }
    UnloadRenderTexture(target);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
