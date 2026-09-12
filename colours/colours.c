#define TOOL_NAME               "simple color table"
#define TOOL_SHORT_NAME         "colours"
#define TOOL_VERSION            "0.0.1"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

const int WIDTH = 864; //multiplo di 24
const int HEIGHT = 576; //multiplo di 24


#define gridSize 24
#define cellsX 36
#define cellsY 24
Color cellColors[cellsX][cellsY];


// functions definition
void genColors(void);


int main (int argc, char *argv[])
{
    // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TOPMOST ); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(WIDTH, HEIGHT, "colours");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);
    // genera matrice colori
    genColors();
    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);


while (!WindowShouldClose())
    {
    //----------------------------------------------------------------------------------
    // Update
    //----------------------------------------------------------------------------------

      if (IsKeyPressed(KEY_SPACE)) genColors();
             
    //----------------------------------------------------------------------------------
    // Draw
    //----------------------------------------------------------------------------------
		BeginTextureMode(target);
		ClearBackground(WHITE);
		EndTextureMode();
		
    BeginDrawing();
            ClearBackground(BLACK);

              for (int i=0; i<cellsX; i++) {
                for (int j=0; j<cellsY; j++) {
                  DrawRectangleRec((Rectangle){i*gridSize,j*gridSize, gridSize-1, gridSize-1}, cellColors[i][j]);
                }
              }
    EndDrawing();
    }
	
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

// functions implementation

void genColors(void) {

for (int i=0; i<cellsX; i++) {
  for (int j=0; j<cellsY; j++) {  
  int R = rand() % 255; 
  int G = rand() % 255;
  int B = rand() % 255;
  //int A = rand() % 255;

  cellColors[i][j]=(Color){R,G,B,255};
  
  }
 }
}
