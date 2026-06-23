/*******************************************************************************************
*
* 
*   Resistor Calculator
*   Small utility to calculate resistor values
*   A simple app to learn C using raylib library
* 
*  CHANGELOG:
* 
*   v. 1.0: first release.
* 
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
*
*   TODO LIST POSSIBLE IMPROVEMENTS:
*
* 
* 
*
*   BUGS:
* 
* 
* 
*******************************************************************************************/

#define TOOL_NAME               "Resistor Calculator"
#define TOOL_SHORT_NAME         "ResCalc"
#define TOOL_VERSION            "1.0"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// raygui integration
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// mouse and clipoard
bool mouseHoverBands = false;

// window initial size
const int screenWidth = 768;
const int screenHeight = 696;

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

// ARDUINO Matrix tool colors (light)
#define BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
#define FG_COLOR CLITERAL(Color){ 79, 88, 92, 255}
#define GRID_COLOR CLITERAL(Color){ 55, 66, 70, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 218, 227,227, 255} 
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}


// 'fake' background
void drawRectangleRounded (int x, int y, int w, int h, Color color)  
{
  Rectangle  rect = { x, y, w, h};   // toplx, toply, width, height
  float radius = 0.04; // no radius
  int   segs   = 12; // non segments
  DrawRectangleRounded ( rect, radius, segs, color );
}


int main (int argc, char *argv[])
{
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(screenWidth, screenHeight, "Resistor Calculator");
    
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowState(FLAG_WINDOW_TOPMOST);
    //SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);

    // Set UI style
    // Custom GUI font loading
    Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    GuiLoadStyle("assets/dce.rgs");
    GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetIconScale(1);

    // Init player 0 (cursor for bin matrix)
    //PlayerState player = { 0 };
    //player.cell = (Point){ 0, 0 };


    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
 

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
          

        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
