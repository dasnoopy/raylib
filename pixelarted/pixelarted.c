/*******************************************************************************************
*
*   PIXEL ART EDITOR
*   A simple pixel art editor app to learn C using raylib/raygui library
* 
*  CHANGELOG:
* 
*  v. 1.0   : first release: draw a 8x8 dot matrix and show/copy HEX value

*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
*
*   TODO LIST POSSIBLE IMPROVEMENTS:

*******************************************************************************************/

#define TOOL_NAME               "Pixel Art Editor"
#define TOOL_SHORT_NAME         "PixelArtEd"
#define TOOL_VERSION            "0.3.0"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


// raygui integration
#define RAYGUI_IMPLEMENTATION
//#define RAYGUI_CUSTOM_ICONS     // Custom icons set required 
//#include "gui_iconset.h"        // Custom icons set provided, generated with rGuiIcons tool
#include "raygui.h"

const int screenWidth = 770;
const int screenHeight = 760;

 // initial X,Y coordinates for variuos interface elements
Vector2 sprite_grid_XY = {72, 96 }; // 
Vector2 colors_bar = {16,8};

#define gridSpacing    20
#define BIN_COLS       32 
#define BIN_ROWS       32
#define MAX_COLORS_COUNT    23          // Number of colors available

//
int colorSelected = 0;
int colorMouseHover = 0;
int colorSelectedPrev = 0;
bool mouseWasPressed = false;

    // Colors to choose from
const Color colors[MAX_COLORS_COUNT] = {
        RAYWHITE, YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
        SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
        LIGHTGRAY, GRAY, DARKGRAY, BLACK };
    // Define colorsRecs data (for every rectangle)
Rectangle colorsRecs[MAX_COLORS_COUNT] = { 0 };

// definizione matrici
int matrice[BIN_COLS][BIN_ROWS];

// ARDUINO Matrix tool colors (light)
#define BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
#define FG_COLOR CLITERAL(Color){ 79, 88, 92, 255}
#define GRID_COLOR CLITERAL(Color){ 55, 66, 70, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 218, 227,227, 255} 
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}

// mouse and clipoard
bool mouseHoverCells = false;
const char fNAME[] = "image.pix";

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


// Draw binary matrix grid
void draw_sprite_grid(void)
{
            for (int y = 0; y <= BIN_ROWS; y++) {
                DrawLine((int)sprite_grid_XY.x, (int)sprite_grid_XY.y + y * gridSpacing,(int)sprite_grid_XY.x + BIN_COLS* gridSpacing, (int)sprite_grid_XY.y + y*gridSpacing, FG_COLOR);
            }
            for (int x = 0; x <= BIN_COLS; x++)
                DrawLine((int)sprite_grid_XY.x + x * gridSpacing, (int)sprite_grid_XY.y,(int)sprite_grid_XY.x + x * gridSpacing, (int)sprite_grid_XY.y + BIN_ROWS*gridSpacing, FG_COLOR);
}

//  disegna bit delle matrice in base al loro valore
void draw_sprite()
{
            for (int i = 0; i < BIN_ROWS; i++)
                {
                    for (int j = 0; j < BIN_COLS; j++)
                    {
                        // disegna sfondo cella  in base al valore 1/0
                        // se si cambia disegno qui, cambiare anche cursore nella sezione BeginDrawing
                        DrawRectangle(sprite_grid_XY.x + gridSpacing*j, sprite_grid_XY.y + gridSpacing*i, 
                          gridSpacing -1, 
                          gridSpacing -1, 
                          colors[matrice[j][i]]);
                    }
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
    InitWindow(screenWidth, screenHeight, TOOL_NAME);
    
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(160);


// Define colorsRecs data (for every rectangle)
    for (int i = 0; i < MAX_COLORS_COUNT; i++)
    {
        colorsRecs[i].x = colors_bar.x + 30.0f*i + 2*i;
        colorsRecs[i].y = colors_bar.y;
        colorsRecs[i].width = 30;
        colorsRecs[i].height = 30;
    }

// Init player 0 (cursor for bin matrix)
    PlayerState player = { 0 };
    player.cell = (Point){ 0, 0 };

//reset matrice binaria
    reset_matrix();

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        Vector2 mousePos = GetMousePosition();

        if (colorSelected >= MAX_COLORS_COUNT) colorSelected = MAX_COLORS_COUNT - 1;
        else if (colorSelected < 0) colorSelected = 0;

        // Choose color with mouse
        for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            if (CheckCollisionPointRec(mousePos, colorsRecs[i]))
            {
                colorMouseHover = i;
                break;
            }
            else colorMouseHover = -1;
        }

        if ((colorMouseHover >= 0) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            colorSelected = colorMouseHover;
           // colorSelectedPrev = colorSelected;
        }

        if (IsKeyPressed(KEY_C))
        {
            // Clear render texture to clear color
            reset_matrix();
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            if (!mouseWasPressed)
            {
                colorSelectedPrev = colorSelected;
                colorSelected = 0;

            }

            mouseWasPressed = true;

            // Erase circle from render texture
                matrice[player.cell.x][player.cell.y] = 0;
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && mouseWasPressed)
        {
            colorSelected = colorSelectedPrev;
            mouseWasPressed = false;
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
        mouseHoverCells = CheckCollisionPointRec(GetMousePosition(),(Rectangle){sprite_grid_XY.x, sprite_grid_XY.y,BIN_COLS*gridSpacing,BIN_ROWS*gridSpacing });
            
            if (mouseHoverCells)
            {
                 // Icon painting mouse logic
                    player.cell.x = (GetMouseX() - sprite_grid_XY.x) / gridSpacing ;
                    player.cell.y = (GetMouseY() - sprite_grid_XY.y) / gridSpacing;
                    // scrive bit 1/0 nella matrice binaria tasto sx /dx del mouse (1 o 0)
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) matrice[player.cell.x][player.cell.y] = colorSelected;
                    //if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) matrice[player.cell.x][player.cell.y] = 0;
            }

        // aggiorna posizione "cursore" quando ci si sposta sulla matrice binaria con i tasto oppure il mouse
        int j= player.cell.x;
        int i= player.cell.y;

        // scrive bit 1/0 della cella selezionato della matrice binaria,  premendo la BARRA SPAZIO
        if ( (IsKeyPressed(KEY_SPACE)) ) matrice[player.cell.x][player.cell.y] = colorSelected;
        //----------------------------------------------------------------------------------
		// Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(BG_COLOR);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BG_COLOR);
            
        // Draw top panel ( color bar)
        DrawRectangle(0, 0, GetScreenWidth(), 50, WHITE);
        DrawLine(0, 50, GetScreenWidth(), 50, GRID_BG_COLOR );

        // Draw color selection rectangles
        for (int i = 0; i < MAX_COLORS_COUNT; i++) DrawRectangleRec(colorsRecs[i], colors[i]);
        DrawRectangleLines(colors_bar.x, colors_bar.y, 30, 30, LIGHTGRAY);

        if (colorMouseHover >= 0) DrawRectangleRec(colorsRecs[colorMouseHover], Fade(WHITE, 0.2f));

        DrawRectangleLinesEx((Rectangle){ colorsRecs[colorSelected].x - 2, colorsRecs[colorSelected].y - 2,
                             colorsRecs[colorSelected].width + 4, colorsRecs[colorSelected].height + 4 }, 2, BLACK);

            // intestazioni riga/colonna matrice binaria
            for (int z = 0; z < BIN_COLS; z++)
            {
                for (int z = 0; z < BIN_ROWS; ++z)
                {
                    DrawText  (TextFormat("%01d",z+1),sprite_grid_XY.x + 4 + (z * gridSpacing),sprite_grid_XY.y -20 ,10,FG_COLOR); // bit decimal value
                    DrawText  (TextFormat("%01d",z+1),sprite_grid_XY.x - 18,sprite_grid_XY.y + 4 + (z * gridSpacing),10, FG_COLOR);
                }
            }
            draw_sprite_grid();
            draw_sprite(); // 1) disegna il carattere nella matrice dopo che è stato caricato in memoria al click del mouse sulla tabella ASCII

            // 5) aggiorna in tempo reale la posizione della cella attuale ("cursore") quando mouse o tastiera si spostano sulle celle...
            DrawRectangle((int)sprite_grid_XY.x + player.cell.x*gridSpacing, 
                          (int)sprite_grid_XY.y + player.cell.y*gridSpacing, 
                          gridSpacing -1, 
                          gridSpacing -1,
                          matrice[j][i] ? ON_COLOR: OFF_COLOR);

        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

