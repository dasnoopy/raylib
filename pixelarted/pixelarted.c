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

const int screenWidth = 916;
const int screenHeight = 756;

 // initial X,Y coordinates for variuos interface elements
Vector2 sprite_grid_XY = {246, 88};
Vector2 colors_bar = {132, 8};
Vector2 info_bar = {16, 72};
Vector2 panel_bar = {16,160};

#define gridSpacing    20
#define BIN_COLS       32
#define BIN_ROWS       32
#define MAX_COLORS_COUNT    24          // Number of colors available
#define MAX_CUR_SIZES  4       // cursor size : 1, 2, 4, 8.
//
int colorSelected = 0;
int colorMouseHover = 0;
int colorSelectedPrev = 0;
int cursorSize = 1;
bool mouseWasPressed = false;


    // Colors to choose from
const Color colors[MAX_COLORS_COUNT] = {
        BLANK, WHITE,YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
        SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
        LIGHTGRAY, GRAY, DARKGRAY, BLACK };

const char *colorNames[MAX_COLORS_COUNT] = { 
        "Blank","White", "Yellow", "Gold", "Orange", "Pink", "Red", "Maroon", "Green", "Lime", "DarkGreen",
        "SkyBlue", "Blue", "DarkBlue", "Purple", "Violet", "DarkPurple", "Beige", "Brown", "DarkBrown",
        "LightGray", "Gray", "DarkGray", "Black" };

    // Define colorsRecs data (for every rectangle)
Rectangle colorsRecs[MAX_COLORS_COUNT] = { 0 };

// definizione matrici
int matrice[BIN_COLS][BIN_ROWS];

// ARDUINO Matrix tool colors (light)
#define FG_COLOR CLITERAL(Color){ 79, 88, 92, 255}
#define BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
#define GRID_COLOR CLITERAL(Color){ 169, 195, 196, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 226, 231,231, 255} 
#define OFF_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define ON_COLOR CLITERAL(Color){ 76, 86, 106,255}

// Various
bool showGrid = true;
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
            DrawRectangle(sprite_grid_XY.x, sprite_grid_XY.y,gridSpacing*BIN_COLS, gridSpacing*BIN_ROWS, GRID_BG_COLOR);
            DrawRectangleLines(sprite_grid_XY.x-1, sprite_grid_XY.y-1,gridSpacing*BIN_COLS+1, gridSpacing*BIN_ROWS+1, GRID_COLOR);

    if (showGrid)
    {
                for (int y = 0; y <= BIN_ROWS; y++) 
                    DrawLine(sprite_grid_XY.x, sprite_grid_XY.y + (y * gridSpacing),sprite_grid_XY.x + (BIN_COLS* gridSpacing), sprite_grid_XY.y + y*gridSpacing, GRID_COLOR);
                for (int x = 0; x <= BIN_COLS; x++)
                  DrawLine(sprite_grid_XY.x + (x * gridSpacing), sprite_grid_XY.y, sprite_grid_XY.x + (x * gridSpacing), sprite_grid_XY.y + (BIN_ROWS*gridSpacing), GRID_COLOR);
    }
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
                        DrawRectangle((sprite_grid_XY.x + gridSpacing*j) , (sprite_grid_XY.y + gridSpacing*i), 
                          gridSpacing - 1, 
                          gridSpacing - 1, 
                          colors[matrice[j][i]]);

                    }
                }   
}

// azzera matrice colore e di conseguenza anche quella esadecimale
void reset_matrice()
{
    //reset matrice colore
    for (int i = 0; i < BIN_ROWS; i++)
        for (int j = 0; j < BIN_COLS; j++) matrice[j][i] = 0; }    

void copy_matrix_2d(int * src, int * dst, int N, int M)
{
    for(int i=0; i<N; i++)
        for(int j=0; j<M; j++)
            dst[(M*i)+j] = src[(M*i)+j];
}

void show_info (void)
{
        // Draw top panel ( color bar)
        DrawRectangle(0, 0, GetScreenWidth(), 50, GRID_BG_COLOR);

            // intestazioni riga/colonna matrice colore
            for (int i = 0; i < BIN_ROWS; i++)
            {
                for (int j = 0; j < BIN_COLS; ++j)
                {
                    DrawText(TextFormat("%01d",j),sprite_grid_XY.x + 4 + (j * gridSpacing),sprite_grid_XY.y -20 ,10,FG_COLOR);  //sopra
                    DrawText(TextFormat("%01d",i),sprite_grid_XY.x - 18,sprite_grid_XY.y + 4 + (i * gridSpacing),10, FG_COLOR); // sinistra
                    DrawText(TextFormat("%01d",j),sprite_grid_XY.x + 4 + (j * gridSpacing),sprite_grid_XY.y + BIN_ROWS*gridSpacing+8 ,10,FG_COLOR);  //sotto
                    DrawText(TextFormat("%01d",i),sprite_grid_XY.x + BIN_COLS*gridSpacing +8,sprite_grid_XY.y + 4 + (i * gridSpacing),10, FG_COLOR); // destra
                }
            }
            //disegna miniatura
            DrawRectangleLines(info_bar.x, info_bar.y , 72, 72, GRID_COLOR);
            for (int i = 0; i < BIN_ROWS; i++)
                for (int j = 0; j < BIN_COLS; j++) DrawRectangle(4 + info_bar.x + 2*j, 4 + info_bar.y +  2*i ,2 ,2, colors[matrice[j][i]]);
}

int main (int argc, char *argv[])
{
    InitWindow(screenWidth, screenHeight, TOOL_NAME);
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works

    // loaad TTF font with better antialiasing
    Font font = LoadFontEx("assets/JetBrains Mono SemiBold.ttf", 20, 0, 250);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  
    // set FPS
    SetTargetFPS(60);

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

//reset matrice colore
    reset_matrice();

int curW,curH;

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

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            if (!mouseWasPressed)
            {
                colorSelectedPrev = colorSelected;
                colorSelected = 0;
            }
            mouseWasPressed = true;
            matrice[player.cell.x][player.cell.y] = 0;
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && mouseWasPressed)
        {
            colorSelected = colorSelectedPrev;
            mouseWasPressed = false;
        }

        // rileva se la posizione mouse e' dentro la matrice colore...
        mouseHoverCells = CheckCollisionPointRec(GetMousePosition(),(Rectangle){sprite_grid_XY.x, sprite_grid_XY.y,BIN_COLS*gridSpacing,BIN_ROWS*gridSpacing });
            
            if (mouseHoverCells)
            {
                 // Icon painting mouse logic
                    player.cell.x = (GetMouseX() - sprite_grid_XY.x) / gridSpacing ;
                    player.cell.y = (GetMouseY() - sprite_grid_XY.y) / gridSpacing;
                   
        curW=cursorSize;
        curH=cursorSize;

        if ((player.cell.x + curW) >= BIN_COLS) curW = BIN_COLS - player.cell.x;
        if ((player.cell.y + curH) >= BIN_ROWS) curH = BIN_ROWS - player.cell.y;

                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) 
                        for (int i = 0; i < curH; i++)
                            for (int j = 0; j < curW; j++) matrice[player.cell.x + j][player.cell.y + i] = colorSelected;
            }


        // aggiorna posizione "cursore" quando ci si sposta sulla matrice colore con i tasto oppure il mouse
        int px= player.cell.x;
        int py= player.cell.y;


        //----------------------------------------------------------------------------------
		// Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(BG_COLOR);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BG_COLOR);
        // draw accessories information
        show_info();

        // Draw color selection bar
        for (int i = 0; i < MAX_COLORS_COUNT; i++) DrawRectangleRec(colorsRecs[i], colors[i]);
        DrawRectangleLines(colors_bar.x, colors_bar.y, 30, 30, LIGHTGRAY);

        if (colorMouseHover >= 0) DrawRectangleRec(colorsRecs[colorMouseHover], Fade(WHITE, 0.2f));

        DrawRectangleLinesEx((Rectangle){ colorsRecs[colorSelected].x - 2, colorsRecs[colorSelected].y - 2,
                             colorsRecs[colorSelected].width + 4, colorsRecs[colorSelected].height + 4 }, 2, BLACK);

        draw_sprite_grid();
        draw_sprite(); //

            // 5) aggiorna in tempo reale la posizione della cella attuale ("cursore") quando mouse o tastiera si spostano sulle celle...
            DrawRectangle(sprite_grid_XY.x + px*gridSpacing, 
                          sprite_grid_XY.y + py*gridSpacing, 
                          gridSpacing -1, 
                          gridSpacing -1,
                          ON_COLOR);

                //display cursor position and selected color info
                DrawTextEx(font, TextFormat("x: %02i y: %02i",px,py),(Vector2){info_bar.x + 84,info_bar.y},20,0,FG_COLOR);
                DrawTextEx(font, TextFormat("%s",colorNames[matrice[px][py]]),(Vector2){info_bar.x + 84,info_bar.y + 24},20,0,FG_COLOR);

        // some keybinding action to test functionality
        if (IsKeyPressed(KEY_C))
        {
            // Clear render texture to clear color
            reset_matrice();
        }

        if (IsKeyPressed(KEY_R))
        {
            int previousColor=matrice[px][py];
            for (int i = 0; i <= BIN_ROWS; i++) {
                for (int j = 0; j <= BIN_COLS; j++) {
                    //if (matrice[j][i] > 0 && matrice[j][i] == previousColor ) matrice[j][i] = colorSelected;
                    if (matrice[j][i] == previousColor ) matrice[j][i] = colorSelected;
                }
            }
        }
        
        // PabelBar

            GuiCheckBox((Rectangle){ 16, 16 , 20, 20 }, "Show Grid", &showGrid);
            GuiLabel((Rectangle){ panel_bar.x, panel_bar.y, 320, 20 }, "#25#Cursor size:");
            GuiSpinner((Rectangle){ panel_bar.x, panel_bar.y+24, 125, 25 }, "", &cursorSize, 1, 8, false);


        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

