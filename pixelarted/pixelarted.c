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
#define TOOL_VERSION            "0.9.3"

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

const int screenWidth = 900;
const int screenHeight = 756;

 // initial X,Y coordinates for variuos interface elements
Vector2 colorsBarPos = {188, 12};
Vector2 spriteGridPos = {220, 86};
Vector2 miniaturePos = {28,120};
Vector2 panelBarPos = {24,340};

int gridSpacing = 20;
#define BIN_COLS       32
#define BIN_ROWS       32
#define MAX_COLORS_COUNT    24          // Number of colors available
#define MAX_CUR_SIZES  4       // cursor size : 1, 2, 4, 8.

// definizione matrici
int matrice[BIN_COLS][BIN_ROWS];
int matriceUndo[BIN_ROWS][BIN_COLS];
// Definizione variabili
int selectedColor = 24;
int currentColor = 0;
int colorMouseHover = 0;
int cursorSize = 1;
int miniatureSCALE= 4;
bool showGrid = false;
bool showChessboard = true;
bool mouseHoverCells = false;
char fNAME[256] = "default.pix";
bool fnameEditMode = false;
bool keyBinding = true;

// Colors to choose from
const Color colors[MAX_COLORS_COUNT] = {
        BLANK, WHITE,YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
        SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
        LIGHTGRAY, GRAY, DARKGRAY, BLACK };

// Define colorsRecs data
Rectangle colorsRecs[MAX_COLORS_COUNT] = { 0 };

// ARDUINO Matrix tool colors (light)
#define FG_COLOR CLITERAL(Color){ 55, 65, 70, 255}
#define BG_COLOR CLITERAL(Color){ 236, 241,241, 255} 
// grid and checkerboard
#define GRID_COLOR CLITERAL(Color){ 76, 86, 106, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 218, 227, 227, 255} 
#define CHECKB_COLOR CLITERAL(Color){ 246, 251, 251, 255} 
// some funs
#define OFF_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define ON_COLOR CLITERAL(Color){ 242, 103, 39,255}

#define BORDER_COLOR CLITERAL(Color){ 181, 190, 190, 255} 

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
        DrawRectangleLinesEx((Rectangle){spriteGridPos.x-2, spriteGridPos.y-2, 4+(BIN_COLS*gridSpacing),4+(BIN_ROWS*gridSpacing)},2,BG_COLOR);
    }

// Draw grid lines 
void drawGridLines(void)
{
    if (showGrid)
    {
        for (int y = 0; y <= BIN_ROWS; y+=1) 
            DrawLineEx((Vector2){spriteGridPos.x, spriteGridPos.y + (y * gridSpacing)},(Vector2){spriteGridPos.x + (BIN_COLS* gridSpacing), spriteGridPos.y + y*gridSpacing},1, GRID_COLOR);
        for (int x = 0; x <= BIN_COLS; x+=1)
            DrawLineEx((Vector2){spriteGridPos.x + (x * gridSpacing), spriteGridPos.y}, (Vector2){spriteGridPos.x + (x * gridSpacing), spriteGridPos.y + (BIN_ROWS*gridSpacing)},1, GRID_COLOR);

        DrawRectangleLinesEx((Rectangle){spriteGridPos.x-1, spriteGridPos.y-1, 1+(BIN_COLS*gridSpacing),1+(BIN_ROWS*gridSpacing)},1,GRID_COLOR);
    }
}
//  disegna bit delle matrice in base al loro valore
void drawSprite()
{    
            for (int i = 0; i < BIN_ROWS; i++)
                {
                    for (int j = 0; j < BIN_COLS; j++)
                    {
                        // disegna sfondo cella  in base al valore 1/0
                        // se si cambia disegno qui, cambiare anche cursore nella sezione BeginDrawing
                        DrawRectangle(((spriteGridPos.x) + gridSpacing*j) , ((spriteGridPos.y) + gridSpacing*i), 
                          gridSpacing, 
                          gridSpacing, 
                          colors[matrice[j][i]]);
                         //DrawText(TextFormat("%02i",matrice[j][i]),2+spriteGridPos.x + gridSpacing*j,2+spriteGridPos.y + gridSpacing*i,10,WHITE);
                    }
                }   
}

void showInfo (void)
{
    DrawText(TextFormat("%s", TOOL_SHORT_NAME), 36, 14, 20, FG_COLOR); 
    DrawText(TextFormat("version %s", TOOL_VERSION), 52, 38, 10, GRAY); 

    // cornice e sfondo miniatura
        DrawRectangle(miniaturePos.x,miniaturePos.y,BIN_COLS*miniatureSCALE,BIN_ROWS*miniatureSCALE, BG_COLOR);
        DrawRectangleLines(miniaturePos.x-3, miniaturePos.y-3 , (BIN_COLS*miniatureSCALE)+6, (BIN_ROWS*miniatureSCALE)+6, BORDER_COLOR);
    // intestazioni riga/colonna matrice colore e miniatura
        for (int i = 0; i < BIN_ROWS; i++)
        {
            for (int j = 0; j < BIN_COLS; ++j)
            {
                DrawText(TextFormat("%01d",j+1),spriteGridPos.x + 4 + (j * gridSpacing),spriteGridPos.y -20 ,10,FG_COLOR);  //sopra
                DrawText(TextFormat("%01d",i+1),spriteGridPos.x - 18,spriteGridPos.y + 4 + (i * gridSpacing),10, FG_COLOR); // sinistra
                DrawText(TextFormat("%01d",j+1),spriteGridPos.x + 4 + (j * gridSpacing),spriteGridPos.y + BIN_ROWS*gridSpacing+8 ,10,FG_COLOR);  //sotto
                DrawText(TextFormat("%01d",i+1),spriteGridPos.x + BIN_COLS*gridSpacing +8,spriteGridPos.y + 4 + (i * gridSpacing),10, FG_COLOR); // destra
                //miniatura
                DrawRectangle(miniaturePos.x + (miniatureSCALE*j), miniaturePos.y + (miniatureSCALE*i) ,miniatureSCALE ,miniatureSCALE, colors[matrice[j][i]]);
            }
        }
}

// azzera matrice colore
void resetSprite()
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

int main (int argc, char *argv[])
{
    //SetConfigFlags (FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT); // occhio che sfalsa visualizzazione linee spessori colori...!!!
    InitWindow(screenWidth, screenHeight, "Pixel Art Editor");
        // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works

    // loaad TTF font with better antialiasing
    Font font = LoadFontEx("assets/VictorMono-Bold.ttf", 18, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    // Set UI style
    // Custom GUI font ShowLoading
    //GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    GuiSetIconScale(1);

    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);

    // set FPS
    SetTargetFPS(60);

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

// variabile usata per adattamento cursore in prossimità lato destro e in basso
// della matrice colore
int curW=cursorSize;
int curH=cursorSize;
int curSW=0;
int curSH=0;

while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        Vector2 mousePos = GetMousePosition();

        if (selectedColor >= MAX_COLORS_COUNT) selectedColor = MAX_COLORS_COUNT - 1;
        else if (selectedColor < 0) selectedColor = 0;
        //------------------------------------------------------------------------------
        // Choose color with mouse
        //------------------------------------------------------------------------------
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
        // rileva se la posizione mouse e' dentro la matrice colore...
        //------------------------------------------------------------------------------
        mouseHoverCells = CheckCollisionPointRec(mousePos,(Rectangle){spriteGridPos.x, spriteGridPos.y,BIN_COLS*gridSpacing,BIN_ROWS*gridSpacing });
        if (mouseHoverCells)
            {
                 // Icon painting mouse logic
                player.cell.x = (GetMouseX() - spriteGridPos.x) / gridSpacing ;
                player.cell.y = (GetMouseY() - spriteGridPos.y) / gridSpacing;

                curW=cursorSize;
                curH=cursorSize;
                
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) // copia la matrice colori in una matrice copia per un succ. revert completo...
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);

                if ((player.cell.x + curW) >= BIN_COLS) curW = BIN_COLS - player.cell.x;
                if ((player.cell.y + curH) >= BIN_ROWS) curH = BIN_ROWS - player.cell.y;

                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    for (int i = 0; i < curH; i++)
                        for (int j = 0; j < curW; j++) matrice[player.cell.x + j][player.cell.y + i] = selectedColor;
                }
                if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
                {
                    for (int i = 0; i < curH; i++)
                        for (int j = 0; j < curW; j++) matrice[player.cell.x + j][player.cell.y + i] = 0;
                }
        }

                if (IsKeyPressed(KEY_SPACE))
                {
                    for (int i = 0; i < curH; i++)
                        for (int j = 0; j < curW; j++) matrice[player.cell.x + j][player.cell.y + i] = selectedColor;
                }

        // aggiorna posizione "cursore" quando ci si sposta sulla matrice colore con i tasto oppure il mouse
        int px= player.cell.x;
        int py= player.cell.y;
        int currentColor = matrice[px][py];
        // gestione stato load & save file
        bool ShowLoading=false;
        bool ShowSaving=false;

            


        // ---------------------------------------------------------------------
        // some keybinding action to test functionality
        //----------------------------------------------------------------------

        if (!fnameEditMode) // se stò digitando il nome file nel riquadro di input, disabilita i keybindings
        {
            
            if (IsKeyPressed(KEY_Q)) break;
            if (IsKeyPressed(KEY_N)) 
                {
                    // fai sempre una copia di backup dello stato attuale della matrice
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);
                    strcpy(fNAME,"default.pix");
                    resetSprite();
                }
            if (IsKeyPressed(KEY_R)) 
                {
                    // fai sempre una copia di backup dello stato attuale della matrice
                    copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);
                    replaceColor(currentColor, selectedColor);
                }
            if (IsKeyPressed(KEY_Z)) copyMatrix(&matriceUndo[0][0], &matrice[0][0], BIN_ROWS, BIN_COLS);
            if (IsKeyPressed(KEY_S)) ShowSaving=true;
            if (IsKeyPressed(KEY_L)) ShowLoading=true;

            if (IsKeyPressed(KEY_F))
            {
                // fai sempre una copia di backup dello stato attuale della matrice
                copyMatrix(&matrice[0][0], &matriceUndo[0][0], BIN_ROWS, BIN_COLS);
                floodFill(px,py,currentColor,selectedColor);
            }

            if (IsKeyPressed(KEY_D)) // shift bin array right by 1
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

            if (IsKeyPressed(KEY_A))// shift bin array left by 1
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

            if (IsKeyPressed(KEY_W)) // shift bin array down by 1
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
     
             if (IsKeyPressed(KEY_X))// shift bin array up by 1
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
         }       
        //----------------------------------------------------------------------------------
		// Draw
        //----------------------------------------------------------------------------------
    BeginTextureMode(target);
        ClearBackground(GRID_BG_COLOR);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(GRID_BG_COLOR);
        DrawRectangle(186,50, screenWidth,screenHeight, BG_COLOR);
        DrawRectangleLines(186,50, screenWidth,screenHeight, BORDER_COLOR);

        // Draw color selection bar
        for (int i = 0; i < MAX_COLORS_COUNT; i++) DrawRectangleRec(colorsRecs[i], colors[i]);
        //  riquadro attorno al primo colore: BLANK (trasparente)
        DrawRectangleLinesEx((Rectangle){colorsBarPos.x, colorsBarPos.y, 26, 26},1, BORDER_COLOR);
        // passando sopra il colore rendilo piu chiaro
        if (colorMouseHover >= 0) DrawRectangleRec(colorsRecs[colorMouseHover], Fade(WHITE, 0.2f));
        // cliccando sul colore disegna riguadro attorno per evidenziare selezione
        DrawRectangleLinesEx((Rectangle){ colorsRecs[selectedColor].x - 2, colorsRecs[selectedColor].y + 29,
                             colorsRecs[selectedColor].width + 4, colorsRecs[selectedColor].height - 22 }, 2, ON_COLOR);

        // draw sprite and grid matrix
        drawCheckerboard();
        drawSprite(); //

        // aggiorna in tempo reale la dimensione del "cursore"  quando mouse o tastiera si spostano sulle celle...
        // adattando anche la stessa  in prossimità del bordo destro e in basso.
       
        // destra
        if ((px + curW) >= BIN_COLS)
        { curSW = BIN_COLS - px;}
        else { curSW = cursorSize;}
        // basso
        if ((py + curH) >= BIN_ROWS)
        { curSH = BIN_ROWS - py;}
        else { curSH = cursorSize;}
        
        DrawRectangleRec((Rectangle){(spriteGridPos.x) + px*gridSpacing, 
                          (spriteGridPos.y) + py*gridSpacing, 
                          gridSpacing * curSW, 
                          gridSpacing * curSH},
                          OFF_COLOR);

        // draw accessories information
        showInfo();

        //display cursor position and selected color info 
            // draw current color frame
            DrawRectangleLines(miniaturePos.x -3 ,miniaturePos.y+136 , 24,24,BORDER_COLOR);
            DrawRectangle(miniaturePos.x-1,miniaturePos.y+138 , 20 , 20, colors[matrice[px][py]]);
            // Draw selected color frame
            DrawRectangleLines(miniaturePos.x +24 ,miniaturePos.y+136 , 24,24,BORDER_COLOR);
            DrawRectangle(miniaturePos.x+26,miniaturePos.y+138 , 20 , 20, colors[selectedColor]);
            // Draw x,y info
            DrawTextEx(font, TextFormat("x:%02i y:%02i",px+1,py+1),(Vector2){miniaturePos.x + 56,miniaturePos.y+140},18,0,FG_COLOR);


        if (ShowSaving)
        { // manage overwriting file
                            FILE *fSave = fopen(fNAME, "wb");
                                if (fSave == NULL) {
                                    printf("File [%s] not found!\n",fNAME);
                                    return 1;
                                }
                            fwrite(matrice, sizeof(char), sizeof(matrice), fSave);
                            fclose(fSave);
                    //         }
                    ShowSaving=false;
                    // }
            }

            if (ShowLoading &&  FileExists(fNAME))
            {
                    FILE *fLoad = fopen(fNAME, "rb"); 
                                if (fLoad == NULL) {
                                    printf("File [%s] not found!\n",fNAME);
                                    return 1;
                                    // gestire file not found con finestra
                                    }
                            fread(matrice, sizeof(char), sizeof(matrice), fLoad);
                            fclose(fLoad);
                    //     }
                    ShowLoading=false;
                    // }
            }

            // panelBarPos
            
            //GuiGroupBox((Rectangle){ panelBarPos.x-8, panelBarPos.y,150,290}, "Sprite options:");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y +10, 150, 24 }, "Cursor size:");
            GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
            GuiSpinner((Rectangle){ panelBarPos.x, panelBarPos.y+30, 132, 24 }, "", &cursorSize, 1, 8, false);
            //Keybinding label
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+60, 140, 20 }, "Press 'N' to new default.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+80, 140, 20 }, "Press 'R' to replace color.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+100, 140, 20 }, "Press 'A' to shift matrix left.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+120, 140, 20 }, "Press 'D' to shit matrix right.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+140, 140, 20 }, "Press 'W' to shift matrix up.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+160, 140, 20 }, "Press 'X' to shift matrix down.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+180, 140, 20 }, "Press 'Z' to undo last action.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+200, 140, 20 }, "Press 'F' to fill color area.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+220, 140, 20 }, "Press 'S' to save matrix.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+240, 140, 20 }, "Press 'L' to load matrix.");
            GuiCheckBox((Rectangle){ panelBarPos.x, panelBarPos.y +264 , 20, 20 }, "Show Grid lines", &showGrid);
            //GuiToggle((Rectangle){ panelBarPos.x, panelBarPos.y +264 , 140, 20 }, "Grid / Checkerboaard", &showGrid); 
            GuiLabel((Rectangle){ miniaturePos.x, miniaturePos.y - 60, 140, 20 }, "Filename [ Load / Save ]");
            if(GuiTextBox((Rectangle){ miniaturePos.x-3, miniaturePos.y -40, 134, 28 }, fNAME, 256, fnameEditMode)) fnameEditMode = !fnameEditMode;

            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+340, 140, 20 }, "WinKey + mouse left to move.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+360, 140, 20 }, "Press 'Q' to quit program.");
            
            //zoom
            // float gridSpacingF = (float) gridSpacing;
            // GuiSliderBar((Rectangle){ panelBarPos.x + 30, panelBarPos.y + 260, 90, 16 }, "ZOOM:", TextFormat("x%i", gridSpacing), &gridSpacingF, 10.0f, 20.0f);
            // gridSpacing = (int) gridSpacingF;
            // if (gridSpacing < 10) gridSpacing = 10;
            // else if (gridSpacing > 20) gridSpacing = 20;
        drawGridLines();

        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}