/*******************************************************************************************
*
*   Resistor Calculator
*   Small utility to calculate 5 band resistor value
*   A simple app to learn C using raylib library
* 
*  CHANGELOG:
*  v. 1.0: first release.
* 
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
*
*   TODO LIST / IMPROVEMENTS:
*
*   BUGS:
* 
*******************************************************************************************/

#define TOOL_NAME               "5 band resistor calculator"
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

// screen size
    const int screenWidth = 768;
    const int screenHeight = 696;

// Elements positoning
const Vector2 coord_resistor_image = {24, 36};
const Vector2 bin_grid_XY = {140, 270};
const Vector2 coord_Values = {560, 48};

// ARDUINO colors (light)
#define BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
#define FG_COLOR CLITERAL(Color){ 79, 88, 92, 255}
#define GRID_COLOR CLITERAL(Color){ 55, 66, 70, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 218, 227,227, 255} 
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}

#define MAX_COLORS_COUNT    13
#define MAX_BANDS   5

#define gridSpacingX    96
#define gridSpacingY    30

// definizione matrici
int matrice[MAX_BANDS][MAX_COLORS_COUNT];

double valori[MAX_COLORS_COUNT][MAX_BANDS] = 
{
    {-1, 0, 0,1, -1},
    {1, 1, 1, 10, 1},
    {2, 2, 2, 100, 2},
    {3, 3, 3, 1000, 0.05},
    {4, 4, 4, 10000, 0.02},
    {5, 5, 5, 100000, 0.5},
    {6, 6, 6, 1000000, 0.25},
    {7, 7, 7, 10000000, 0.10},
    {8, 8, 8, 100000000, 0.01},
    {9, 9, 9, 1000000000, -1},
    {-1, -1, -1, 0.1, 5},
    {-1, -1, -1, 0.01, 10},
    {-1, -1, -1, 0.001, -1}
};

const char *bande[5]={"1st BAND","2nd BAND","3rd BAND","MULTIPLIER","TOLERANCE"};
double bandVal[MAX_BANDS] ={1,0,0,100,5};
int colorBand[MAX_BANDS] = {1,0,0,2,10};
float resistenza, mintolerance, maxtolerance;

// Colors to choose from
const Color bandColors[MAX_COLORS_COUNT] = {BLACK, BROWN, RED, ORANGE, YELLOW, GREEN, BLUE, VIOLET, GRAY, WHITE, GOLD, LIGHTGRAY, PINK};
const char *colorNames[MAX_COLORS_COUNT] = {"BLACK", "BROWN", "RED", "ORANGE", "YELLOW", "GREEN", "BLUE", "VIOLET", "GRAY", "WHITE", "GOLD", "SILVER", "PINK"};

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

// mouse and clipoard
bool mouseHoverCells = false;

const char* res_int(float n) {
    static char buf[22];

    if (n >= 1000000000)
        sprintf(buf, "%.1fG", n / 1000000000);
    else if (n >= 1000000)
        sprintf(buf, "%.1fM", n / 1000000);
    else if (n >= 1000)
        sprintf(buf, "%.1fK", n / 1000);
    else if (n >= 1)
        sprintf(buf, "%.0f", n / 1);
    else
        sprintf(buf, "%.3f", n);
    return buf;
}

const char* tol_int(float n) {
    static char buf[22];

    if (n >= 1000000000)
        sprintf(buf, "%.5fG", n / 1000000000);
    else if (n >= 1000000)
        sprintf(buf, "%.5fM", n / 1000000);
    else if (n >= 1000)
        sprintf(buf, "%.5fK", n / 1000);
    else if (n >= 1)
        sprintf(buf, "%.5f", n / 1);
    else
        sprintf(buf, "%.5f", n);
    return buf;
}

//  disegna matrice colori
void drawColorTable(void)
{
    bool colore = true; // serve per cambiare fg_color in base ai colori chiari
            // sfondo sotto la tabella per simulare la grigliaa
            DrawRectangle(bin_grid_XY.x-1,bin_grid_XY.y-1, 6+gridSpacingX*5, 14+gridSpacingY*13,FG_COLOR);
            for (int i = 0; i < MAX_COLORS_COUNT; i++)
                {
                    // colore giallo bianco rosa e grigio chiaro , usa colore nero per FG_COLOR
                    if (i==4 || i>=9) colore=false; 
                    // nome dei colori a destra e sinistra
                    DrawText(colorNames[i], bin_grid_XY.x-10-MeasureText(colorNames[i], 10) , (bin_grid_XY.y + 8) +  (gridSpacingY+1 )*i , 10, FG_COLOR);
                    DrawText(colorNames[i], 14+bin_grid_XY.x+gridSpacingX*5, (bin_grid_XY.y + 8) +  (gridSpacingY+1 )*i , 10, FG_COLOR);

                    for (int j = 0; j < MAX_BANDS; j++)
                    {
                        //intestazioni di colonna : nome bande
                        DrawText(bande[j],(bin_grid_XY.x + 20)+(gridSpacingX+1)*j, bin_grid_XY.y - 20,10,FG_COLOR);
                       
                        // disegna sfondo cella  in base al valore 1/0 di matrice
                        DrawRectangle(bin_grid_XY.x + (gridSpacingX*j) + j,
                                     bin_grid_XY.y + (gridSpacingY*i) + i, 
                                    gridSpacingX, 
                                    gridSpacingY, 
                                    matrice[j][i] ? Fade(bandColors[i], 0.4f) : bandColors[i]);
                        // stampa i valori della colonne formattando  il testo in base al tipo di dato.
                        if (valori[i][j]>=0 && j < 3) DrawText(TextFormat("%g",valori[i][j]), (bin_grid_XY.x + 40)+(gridSpacingX+1)*j, (bin_grid_XY.y + 10) +  (gridSpacingY+1 )*i ,10, colore ? WHITE:BLACK);
                        else  if (valori[i][j]>=0 && j ==3) DrawText(TextFormat("%s",res_int(valori[i][j])), (bin_grid_XY.x + 40)+(gridSpacingX+1)*j, (bin_grid_XY.y+10) +  (gridSpacingY+1 )*i ,10, colore ? WHITE:BLACK);
                        else  if (valori[i][j]>=0 && j ==4) DrawText(TextFormat("±%g%%",valori[i][j]), (bin_grid_XY.x + 40)+(gridSpacingX+1)*j, (bin_grid_XY.y+10) +  (gridSpacingY+1 )*i ,10, colore ? WHITE:BLACK);
                    }
                    colore=true;
                }   
}

void reset_matrix_column(int col)
{
    //reset matrice binaria per colonna
    for (int i = 0; i < MAX_COLORS_COUNT; i++) matrice[col][i] = 0; 
}

void calcoloResistenza(void)
{
     // Res = (digit1 × 100 + digit2 × 10 + digit3 ) × multiplier ±tolerance 
    resistenza = (bandVal[0] *100 + bandVal[1] *10 + bandVal[2]) * bandVal[3];
    if (resistenza<=0) resistenza=0;

    mintolerance = resistenza - ((resistenza * bandVal[4])/100);
    maxtolerance = resistenza + ((resistenza * bandVal[4])/100);
}

void SomeDesign(void)
{
               // sfondi scritte e resistenza
                DrawRectangle(0, 0 ,screenWidth,240 , GRID_BG_COLOR);
                DrawRectangle(550,40,261,160,BG_COLOR);
                // griglia valori
                DrawRectangleLines(0, 1 ,screenWidth+1,239 , LIGHTGRAY);
                DrawRectangleLines(550,0,261,240,LIGHTGRAY);
                DrawRectangleLines(550,40,261,160,LIGHTGRAY);
                DrawLine(550,120,screenWidth-1,120,LIGHTGRAY);
                //
                DrawText("Min. tolerance value :",558,4,10,FG_COLOR);
                DrawText("Resistor value :",558,44,10,FG_COLOR);
                DrawText("Tolerance value :",558,124,10,FG_COLOR);

                DrawText("Max. tolerance value :",558,204,10,FG_COLOR);
                DrawText("Standard: IEC 60062 : 2016",208,188,10,FG_COLOR);
}

int main (int argc, char *argv[])
{
    //----------------------------------------------------------------------------------
    // Initialization
    //----------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "5 Band resistor calculator");
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    // loaad TTF font with better antialiasing
    Font font = LoadFontEx("assets/Din.otf", 64, 0, 250);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
 
 // NOTE: Textures MUST be loaded after Window initialization (OpenGL context is required)
    Image resistor = LoadImage("assets/resistor.png");     // Loaded in CPU memory (RAM)
    Texture2D resistor_texture = LoadTextureFromImage(resistor);          // Image converted to texture, GPU memory (VRAM)
    UnloadImage(resistor);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM
    // OHM symbol as image
    Image omega = LoadImage("assets/omega.png");     // Loaded in CPU memory (RAM)
    Texture2D omega_texture = LoadTextureFromImage(omega);          // Image converted to texture, GPU memory (VRAM)
    UnloadImage(omega);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM
    // PERCENTAGE symbol as image
    Image percent = LoadImage("assets/percent.png");     // Loaded in CPU memory (RAM)
    Texture2D percent_texture = LoadTextureFromImage(percent);          // Image converted to texture, GPU memory (VRAM)
    UnloadImage(percent);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM


   // Coordinate grafiche per disegnare le 5 bande della resistenza.
    const Rectangle bands[] = {
        (Rectangle){  coord_resistor_image.x + 90, coord_resistor_image.y, 32, 160 },
        (Rectangle){  coord_resistor_image.x + 170, coord_resistor_image.y +16, 32, 128 },
        (Rectangle){  coord_resistor_image.x + 220, coord_resistor_image.y +16, 32, 128 },
        (Rectangle){  coord_resistor_image.x + 270, coord_resistor_image.y +16, 32, 128 },
        (Rectangle){  coord_resistor_image.x + 372, coord_resistor_image.y, 32, 160 }
    };

    // Init current player state
    PlayerState player = { 0 };
    player.cell = (Point){ 0, 0 };
    
    int currSelBand = 0;

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------

        mouseHoverCells = CheckCollisionPointRec(GetMousePosition(),(Rectangle){bin_grid_XY.x, bin_grid_XY.y, MAX_BANDS*gridSpacingX,MAX_COLORS_COUNT*gridSpacingY });
            
            if (mouseHoverCells)
            {
                 // Icon painting mouse logic
                    player.cell.x = (GetMouseX() - bin_grid_XY.x) / (gridSpacingX+1);
                    player.cell.y = (GetMouseY() - bin_grid_XY.y) / (gridSpacingY+1);

                    // zone off limits

                    if (player.cell.x < 0 ) player.cell.x = 0;
                    else if (player.cell.x >= MAX_BANDS) player.cell.x = MAX_BANDS-1;
                    else if (player.cell.y < 0) player.cell.y = 0 ;
                    else if (player.cell.y >= MAX_COLORS_COUNT) player.cell.y = MAX_COLORS_COUNT-1;

                    // scrive bit 1/0 nella matrice binaria tasto sx /dx del mouse (1 o 0)
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (valori[player.cell.y][player.cell.x]>=0) {
                            currSelBand = player.cell.x;
                            reset_matrix_column(currSelBand);
                            matrice[player.cell.x][player.cell.y]  = 1;
                            // imposta colore relativa banda resistenza
                            colorBand[currSelBand] = player.cell.y;
                            // aassegna valore bande
                            bandVal[currSelBand] = valori[player.cell.y][player.cell.x];
                        }
                    }
            }
      //int px=player.cell.x;
      //int py=player.cell.y;

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------

    BeginTextureMode(target);
        ClearBackground(BG_COLOR);
    EndTextureMode();


    BeginDrawing();
            ClearBackground(BG_COLOR);
            // sfondo sotto la resistenza
            SomeDesign();

            // show resistor PNG images
            DrawTexture(resistor_texture, coord_resistor_image.x, coord_resistor_image.y, WHITE);
            DrawTexture(omega_texture, 728,66, BLACK);
            DrawTexture(percent_texture,728,146,BLACK);

            for (int y = 0; y < MAX_BANDS; ++y)
            {
                // disegna le 5 bande della resistenza...
                DrawRectangleRec(bands[y], bandColors[colorBand[y]]);
            }
                // disegna tabella colori e valori resistenza
                drawColorTable();
                //calcolo della resistenza
                calcoloResistenza();
                // stampa valori
                DrawTextEx(font, TextFormat("%s Ohm",tol_int(mintolerance)),(Vector2){coord_Values.x+32, coord_Values.y -34},26,0,FG_COLOR);
                DrawTextEx(font, TextFormat("%s",res_int(resistenza)),(Vector2){coord_Values.x, coord_Values.y}, 64,0, BLACK);
                DrawTextEx(font, TextFormat("±%g%",tol_int(bandVal[4])),(Vector2){coord_Values.x+20, coord_Values.y+80}, 64,0, BLACK);
                DrawTextEx(font, TextFormat("%s Ohm",tol_int(maxtolerance)),(Vector2){coord_Values.x+32, coord_Values.y +164},26,0,FG_COLOR);


                // DrawText(TextFormat("%d %d %i", px,py,currSelBand),8,screenHeight-28,20,GRID_BG_COLOR);
    
    EndDrawing();
    }
    UnloadRenderTexture(target);
    UnloadFont(font);
CloseWindow();
return 0;
}
