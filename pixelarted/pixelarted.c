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
*   BUGS:
*   replace color broken default TEXT 
*******************************************************************************************/

#define TOOL_NAME               "Pixel Art Editor"
#define TOOL_SHORT_NAME         "PixelArtEd"
#define TOOL_VERSION            "0.8.4"

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
Vector2 colorsBarPos = {112, 8};
Vector2 spriteGridPos = {212, 86};
Vector2 miniaturePos = {28,104};
Vector2 infoBarPos = {28, 240};
Vector2 panelBarPos = {24,300};

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
// Various
bool showGrid = true;
bool mouseHoverCells = false;
char fNAME[256] = "image.pix";
bool fnameEditMode = false;
bool keyBinding = true;

int miniatureSCALE=4;

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
#define FG_COLOR CLITERAL(Color){ 55, 65, 70, 255}
#define BG_COLOR CLITERAL(Color){ 218, 227, 227, 255} 
#define GRID_COLOR CLITERAL(Color){ 181, 190, 190, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 236, 241,241, 255} 
#define OFF_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define ON_COLOR CLITERAL(Color){ 242, 103, 39,255}


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
            
    DrawRectangle(spriteGridPos.x, spriteGridPos.y,gridSpacing*BIN_COLS, gridSpacing*BIN_ROWS, GRID_BG_COLOR);
    DrawRectangleLines(spriteGridPos.x-1, spriteGridPos.y-1,gridSpacing*BIN_COLS+1, gridSpacing*BIN_ROWS+1, GRID_COLOR);


    if (showGrid)
    {
                for (int y = 0; y <= BIN_ROWS; y++) 
                    DrawLine(spriteGridPos.x, spriteGridPos.y + (y * gridSpacing),spriteGridPos.x + (BIN_COLS* gridSpacing), spriteGridPos.y + y*gridSpacing, GRID_COLOR);
                for (int x = 0; x <= BIN_COLS; x++)
                  DrawLine(spriteGridPos.x + (x * gridSpacing), spriteGridPos.y, spriteGridPos.x + (x * gridSpacing), spriteGridPos.y + (BIN_ROWS*gridSpacing), GRID_COLOR);
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
                        DrawRectangle((spriteGridPos.x + gridSpacing*j) , (spriteGridPos.y + gridSpacing*i), 
                          gridSpacing, 
                          gridSpacing, 
                          colors[matrice[j][i]]);
                         //DrawText(TextFormat("%02i",matrice[j][i]),2+spriteGridPos.x + gridSpacing*j,2+spriteGridPos.y + gridSpacing*i,10,BG_COLOR);

                    }
                }   
}

// azzera matrice colore e di conseguenza anche quella esadecimale
void reset_matrix()
{
    //reset matrice colore
    for (int i = 0; i < BIN_ROWS; i++)
        for (int j = 0; j < BIN_COLS; j++) matrice[j][i] = 0;
}

void replace_color(int old, int new)
{
            for (int i = 0; i < BIN_ROWS; i++) 
                for (int j = 0; j < BIN_COLS; j++) {
                    if (matrice[j][i] == old) matrice[j][i] = new;
                }
}

void copy_matrix_2d(int * src, int * dst, int N, int M)
{
    for(int i=0; i<N; i++)
        for(int j=0; j<M; j++) dst[(M*i)+j] = src[(M*i)+j];
}

void show_info (void)
{
            // intestazioni riga/colonna matrice colore
            for (int i = 0; i < BIN_ROWS; i++)
            {
                for (int j = 0; j < BIN_COLS; ++j)
                {
                    DrawText(TextFormat("%01d",j),spriteGridPos.x + 4 + (j * gridSpacing),spriteGridPos.y -20 ,10,FG_COLOR);  //sopra
                    DrawText(TextFormat("%01d",i),spriteGridPos.x - 18,spriteGridPos.y + 4 + (i * gridSpacing),10, FG_COLOR); // sinistra
                    DrawText(TextFormat("%01d",j),spriteGridPos.x + 4 + (j * gridSpacing),spriteGridPos.y + BIN_ROWS*gridSpacing+8 ,10,FG_COLOR);  //sotto
                    DrawText(TextFormat("%01d",i),spriteGridPos.x + BIN_COLS*gridSpacing +8,spriteGridPos.y + 4 + (i * gridSpacing),10, FG_COLOR); // destra
                }
            }
            //disegna miniatura
            DrawRectangle(miniaturePos.x,miniaturePos.y,BIN_COLS*miniatureSCALE,BIN_ROWS*miniatureSCALE, GRID_BG_COLOR);
            DrawRectangleLines(miniaturePos.x-3, miniaturePos.y-3 , (BIN_COLS*miniatureSCALE)+6, (BIN_ROWS*miniatureSCALE)+6, GRID_COLOR);

            for (int i = 0; i < BIN_ROWS; i++)
                for (int j = 0; j < BIN_COLS; j++) DrawRectangle(miniaturePos.x + (miniatureSCALE*j), miniaturePos.y + (miniatureSCALE*i) ,miniatureSCALE ,miniatureSCALE, colors[matrice[j][i]]);
}

int main (int argc, char *argv[])
{
    SetConfigFlags (FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Pixel Art Editor");
        // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works


    // loaad TTF font with better antialiasing
    Font font = LoadFontEx("assets/JetBrains Mono SemiBold.ttf", 20, 0, 0);
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
        colorsRecs[i].x = colorsBarPos.x + 30.0f*i + 2*i;
        colorsRecs[i].y = colorsBarPos.y;
        colorsRecs[i].width = 30;
        colorsRecs[i].height = 30;
    }

// Init player 0 (cursor for bin matrix)
    PlayerState player = { 0 };
    player.cell = (Point){ 0, 0 };

//reset matrice colore
    reset_matrix();

int curW,curH;
curW=cursorSize;
curH=cursorSize;

int a,b=0;

    while (!WindowShouldClose())
    {
        GuiEnable();
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

                    if (IsKeyPressed(KEY_RIGHT)) player.cell.x++;
                    else if (IsKeyPressed(KEY_LEFT)) player.cell.x--;
                    else if (IsKeyPressed(KEY_UP)) player.cell.y--;
                    else if (IsKeyPressed(KEY_DOWN)) player.cell.y++;
                                        // Make sure player does not go out of bounds
                    if (player.cell.x < 0) player.cell.x = 0;
                    else if (player.cell.x >= BIN_COLS) player.cell.x = BIN_COLS-1;
                    else if (player.cell.y < 0) player.cell.y = 0 ;
                    else if (player.cell.y >= BIN_ROWS) player.cell.y = BIN_ROWS-1;


        // rileva se la posizione mouse e' dentro la matrice colore...
        mouseHoverCells = CheckCollisionPointRec(mousePos,(Rectangle){spriteGridPos.x, spriteGridPos.y,BIN_COLS*gridSpacing,BIN_ROWS*gridSpacing });
        

        if (mouseHoverCells)
            {


                 // Icon painting mouse logic
                player.cell.x = (GetMouseX() - spriteGridPos.x) / gridSpacing ;
                player.cell.y = (GetMouseY() - spriteGridPos.y) / gridSpacing;
               
                curW=cursorSize;
                curH=cursorSize;
                
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_SPACE)) 
                {
                if ((player.cell.x + curW) >= BIN_COLS) curW = BIN_COLS - player.cell.x;
                if ((player.cell.y + curH) >= BIN_ROWS) curH = BIN_ROWS - player.cell.y;

                    for (int i = 0; i < curH; i++)
                        for (int j = 0; j < curW; j++) matrice[player.cell.x + j][player.cell.y + i] = colorSelected;
                }
            }


        // aggiorna posizione "cursore" quando ci si sposta sulla matrice colore con i tasto oppure il mouse
        int px= player.cell.x;
        int py= player.cell.y;
        int currentColor = matrice[px][py];

        bool ShowLoading=false;
        bool ShowSaving=false;

        // scrive bit 1/0 della cella selezionato della matrice binaria,  premendo la BARRA SPAZIO
        //if (IsKeyPressed(KEY_SPACE)) matrice[player.cell.x][player.cell.y] = !matrice[player.cell.x][player.cell.y];

        // ---------------------------------------------------------------------
        // some keybinding action to test functionality
        //----------------------------------------------------------------------

        if (!fnameEditMode) // se sto digitando il nome file nel riquadro di inpunt, disabilita i keybindings
        {
            if (IsKeyPressed(KEY_Q)) break;
            if (IsKeyPressed(KEY_C)) reset_matrix();
            if (IsKeyPressed(KEY_R)) replace_color(currentColor, colorSelected);
            if (IsKeyPressed(KEY_S)) ShowSaving=true;
            if (IsKeyPressed(KEY_L)) ShowLoading=true;

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
            ClearBackground(BG_COLOR);
        EndTextureMode();

        BeginDrawing();
        
        ClearBackground(BG_COLOR);

    
        // Draw top panel ( color bar)
        DrawRectangle(0, 0, GetScreenWidth(), 50, GRID_BG_COLOR);


        // Draw color selection bar
        for (int i = 0; i < MAX_COLORS_COUNT; i++) DrawRectangleRec(colorsRecs[i], colors[i]);
        DrawRectangleLines(colorsBarPos.x, colorsBarPos.y, 30, 30, LIGHTGRAY);
        if (colorMouseHover >= 0) DrawRectangleRec(colorsRecs[colorMouseHover], Fade(WHITE, 0.2f));
        DrawRectangleLinesEx((Rectangle){ colorsRecs[colorSelected].x - 2, colorsRecs[colorSelected].y - 2,
                             colorsRecs[colorSelected].width + 4, colorsRecs[colorSelected].height + 4 }, 2, BLACK);

        // draw sprite and grid matrix
        draw_sprite_grid();
        draw_sprite(); //

        // aggiorna in tempo reale la dimensione del "cursore"  quando mouse o tastiera si spostano sulle celle...
        // adattando anche la stessa  in prossimità del bordo destro e in basso.
        if ((px + curW) >= BIN_COLS)
        { a = BIN_COLS - px;}
        else {a=cursorSize;}
        
        if ((py + curH) >= BIN_ROWS)
        { b = BIN_ROWS - py;}
        else {b=cursorSize;}
        
        DrawRectangleLines(spriteGridPos.x + px*gridSpacing, 
                          spriteGridPos.y + py*gridSpacing, 
                          gridSpacing * a, 
                          gridSpacing * b,
                          ON_COLOR);

        // draw accessories information
        show_info();

        //display cursor position and selected color info
            DrawTextEx(font, TextFormat("Pixel: %i x %i",BIN_COLS,BIN_ROWS),(Vector2){infoBarPos.x,infoBarPos.y-164},20,0,FG_COLOR);
            DrawTextEx(font, TextFormat("x:%02i y:%02i",px,py),(Vector2){infoBarPos.x,infoBarPos.y},20,0,FG_COLOR);
            DrawTextEx(font, TextFormat("Color: %s",colorNames[currentColor]),(Vector2){infoBarPos.x,infoBarPos.y + 24},20,0,FG_COLOR);

        if (ShowSaving)
            {

            // int ret = GuiTextInputBox ((Rectangle){ (float) GetScreenWidth() / 2 -
            //               120, (float)GetScreenHeight() / 2 - 60, 240,
            //               140 }, GuiIconText ( ICON_FILE_SAVE, "Save file as..." ),
            //               "Type output file name:", "Ok;Cancel", fNAME,
            //               255, NULL );

            //         if (ret!= -1){
            //             if (ret == 1) {
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

            if (ShowLoading)
            {
                // int ret = GuiTextInputBox((Rectangle){ 1000,300, 340, 20 },"Load file","Choose a file to load...","Ok;Cancel",fNAME,255,NULL);
                //     if (ret!= -1){
                //         if (ret ==1) {
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
            GuiCheckBox((Rectangle){ 24, 16 , 20, 20 }, "Show Grid", &showGrid);
            
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y, 150, 20 }, "#25#Cursor size:");
            GuiSpinner((Rectangle){ panelBarPos.x, panelBarPos.y+24, 128, 24 }, "", &cursorSize, 1, 8, false);
            //Keybinding info
            GuiGroupBox((Rectangle){ panelBarPos.x-8, panelBarPos.y + 72,150,140}, "Sprite options:");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+80, 140, 20 }, "Press 'C' to clear matrix.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+100, 140, 20 }, "Press 'R' to replace color.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+120, 140, 20 }, "Press 'A' to shift matrix left.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+140, 140, 20 }, "Press 'D' to shit matrix right.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+160, 140, 20 }, "Press 'W' to shift matrix up.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+180, 140, 20 }, "Press 'X' to shift matrix down.");
            
            GuiGroupBox((Rectangle){ panelBarPos.x-8, panelBarPos.y + 310,156,90}, "filename for save/load:");
            
            if(GuiTextBox((Rectangle){ panelBarPos.x, panelBarPos.y +320, 140, 28 }, fNAME, 256, fnameEditMode)) fnameEditMode = !fnameEditMode;
            
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+356, 140, 20 }, "Press 'S' to save matrix.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+376, 140, 20 }, "Press 'L' to load matrix.");
            
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+400, 140, 20 }, "WinKey + mouse left to move.");
            GuiLabel((Rectangle){ panelBarPos.x, panelBarPos.y+420, 140, 20 }, "Press 'Q' to quit program.");


        EndDrawing();
    }
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

