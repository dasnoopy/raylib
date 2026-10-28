/*******************************************************************************
*
*   SUNCLOCK reborn : a raylib simple sunlight worldmap aand gmt/locale time info
*   Small utility to play mp3 files based on raylib version 6.0
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************/
		
#define TOOL_NAME               "Sunlight World Mapr"
#define TOOL_SHORT_NAME         "sunclock"
#define TOOL_COMMENT            "Simple but modern sunlight world map"
#define TOOL_VERSION            "0.0.1"

#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <id3tag.h>

// gcc -Wall -Werror simplayer.c  -o simplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

// window initial size
#define screenWidth   1080
#define screenHeight  572

// some custom colors
const Color accentColor = SKYBLUE;//CLITERAL(Color){136, 192, 208, 255}; 
const Color primaryColor = CLITERAL(Color){253, 255, 255, 255}; 
const Color secondaryColor = CLITERAL(Color){156, 158, 158, 255};
const Color borderColor = CLITERAL(Color){50, 70, 70, 255};
const Color bgColor = CLITERAL(Color){10, 20, 30, 255};


const int bottomOffset = 32;
Rectangle mapCoord = {0,0,1080,540};
Rectangle infoCoord = {0,540,1080,32};

int latitude;
int longitude;
char latCardinal[6] = "";
char longCardinal[6] = "";

char tmpBuffer[64] = { '\0' };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

void drawRectangleRounded (int X, int Y, int W, int H, float radius, Color color)  {
// raylib vuole il "roundness" espresso come frazione rispetto al lato più corto (300)
  Rectangle  rect = { X, Y, W, H};   // toplx, toply, width, height
  int segments = 36;
  DrawRectangleRounded ( rect, radius, segments, color );
}


Color darkenColor(Color color, float factor)
{
	if (factor < 0.0f) factor = 0.0f;
	if (factor > 1.0f) factor = 1.0f;

	return (Color){
		(unsigned char)(color.r * factor),
		(unsigned char)(color.g * factor),
		(unsigned char)(color.b * factor),
		color.a
	};
}

Color lightenColor(Color color, float factor)
{
	if (factor < 0.0f) factor = 0.0f;
	if (factor > 1.0f) factor = 1.0f;

	return (Color){
		(unsigned char) ( color.r + (255-color.r) * factor ),
		(unsigned char) ( color.g + (255-color.g) * factor ),
		(unsigned char) ( color.b + (255-color.b) * factor ),
		color.a
	};
}




int main (int argc, char *argv[]) 
{
	// Set configuration flags for window creation
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TRANSPARENT ); // | FLAG_MSAA_4X_HINT | FLAG_WINDOW_TOPMOST); 
	InitWindow(screenWidth, screenHeight, "sunclock");
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    SetTargetFPS(60);

	// Set UI style
	// Custom GUI font loading
	Font titleFnt = LoadFontEx("fonts/ManropeV5-SemiBold.otf", 36, NULL, 0);
	Font artistFnt = LoadFontEx("fonts/ManropeV5-Regular.otf", 28, NULL, 0);
	Font defaultFnt = LoadFontEx("fonts/DINPro-Medium.otf", 18, NULL, 0);
	Font digitFnt= LoadFontEx("fonts/SF-Mono-Semibold.ttf", 24, NULL, 0); 
	Font filesFnt= LoadFontEx("fonts/PixelOperator.ttf", 16, NULL, 0); 

    // Create framebuffer
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
	

	//world background image
	Image image = LoadImage("assets/map.png");     // Loaded in CPU memory (RAM)
	Texture2D background = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
	UnloadImage(image); 

	// fai riapparire finestra dopo averla nascosta durante il carimento iniziale
	SetWindowSize(screenWidth,screenHeight);
	SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  
	//SetWindowPosition(8,40);  
	ClearWindowState(FLAG_WINDOW_HIDDEN);

while (!WindowShouldClose()) {

//----------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------

		//float deltaTime = GetFrameTime();

        Vector2 mousePos = GetMousePosition();   // abilitare se cursore libero   
        int stepParallel = 3; //3 = 1 grado
        int stepMeridian = 3; //3 = 1 grado
        // // rileva se la posizione mouse e' dentro la mappa del mondo
        // bool mouseHoverMap = CheckCollisionPointRec(mousePos,mapCoord);

//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------
	BeginTextureMode(target);
		ClearBackground(BLANK);
	EndTextureMode();

	BeginDrawing();
		ClearBackground (BLANK);

		drawRectangleRounded(0,0,screenWidth,screenHeight,0.072f,bgColor);
		//load player background image
		DrawTexture(background, screenWidth/2 - background.width/2, screenHeight/2 - background.height/2, WHITE); // WHITE

        // disegna reticolo geograafico
        BeginScissorMode(mapCoord.x, mapCoord.y, mapCoord.width, mapCoord.height);

	        //Disegna paralleli emisfero nord con indicasti gradi 0° /90°
	        for (int i = 90; i > 0; i-=15) {
	        	DrawLineDashed((Vector2){mapCoord.x,i*stepParallel},(Vector2){mapCoord.width,i*stepParallel},2,2,GRAY);
	        	DrawText(TextFormat("%d°",90-i),mapCoord.x+4 ,(i*stepParallel)-15,10,RED);
	        	DrawText(TextFormat("%d°",90-i),mapCoord.width-15,(i*stepParallel)-15,10,RED);
	        }

	        //Disegna paralleli emisfero sud con indicasti gradi 0° / -90°
	        for (int i = 90; i < 180; i+=15) {
	        	DrawLineDashed((Vector2){mapCoord.x,i*stepParallel},(Vector2){mapCoord.width,i*stepParallel},2,2,GRAY);
	        	if (i != 90) {
	        		DrawText(TextFormat("%d°",90-i),mapCoord.x+5 ,(i*stepParallel)-15,10,RED);
	        		DrawText(TextFormat("%d°",90-i),mapCoord.width-25,(i*stepParallel)-15,10,RED);
	        	}
	        }

	        // disegna meridiani Ovest 0 / -180°
	        for (int i = 180; i > 0; i-=15) {
	        	DrawLineDashed((Vector2){i*stepMeridian, mapCoord.y},(Vector2){i*stepMeridian ,mapCoord.height},2,2,GRAY);
	        	if (i != 180) {
	        		DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,mapCoord.y+5,10,BLACK);
	        		DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,mapCoord.height-15,10,BLACK);
	        	}
	        }

	        // disegna meridiani Est 0° / 180°
	        for (int i = 180; i < 360; i+=15) {
	        	DrawLineDashed((Vector2){i*stepMeridian, mapCoord.y},(Vector2){i*stepMeridian ,mapCoord.height},2,2,GRAY);
	        	DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,mapCoord.y+5,10,BLACK);
	            DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,mapCoord.height-15,10,BLACK);
	        }

			// Draw GreenWich meridian
			DrawLine(mapCoord.width/2,mapCoord.y,mapCoord.width/2,mapCoord.height ,WHITE);

	        // Draw Equatore
	        DrawLine(0,mapCoord.height / 2,mapCoord.width, mapCoord.height/2,WHITE);

             DrawRectangleRec((Rectangle) { mousePos.x - 20, mousePos.y, 40, 1 }, RED);
             DrawRectangleRec((Rectangle) { mousePos.x, mousePos.y - 20, 1, 40 }, RED);

        EndScissorMode();

        //info bar
        BeginScissorMode(infoCoord.x, infoCoord.y, infoCoord.width, infoCoord.height);
        EndScissorMode();


           // Draw mouse position
        	int x = GetMouseX()+1;
        	int y = GetMouseY()+1;

			DrawText(TextFormat("[x%i,y%i]", x, y), infoCoord.x +10, infoCoord.y+10,20,RAYWHITE);
            
            if (y < (mapCoord.height/2) ) {
            	latitude = (int) 90-(y/3);
            	strcpy(latCardinal,"Nord");
            } else {
            	latitude = (int) (y/3) -90;
            	strcpy(latCardinal,"South");
            }

            if (x > (mapCoord.width/2)) {
            	longitude = (int) (x/3)-180;
            	strcpy(longCardinal,"East");
            	} else {
            	longitude  = (int) 180-(x/3);
            	strcpy(longCardinal,"Ovest");
            }

            if (x==(mapCoord.width/2)) strcpy(longCardinal,"");
            if (y==(mapCoord.height/2)) strcpy(latCardinal,"");
            
            snprintf(tmpBuffer, sizeof(tmpBuffer), "Lat: %i° %s, Long: %i° %s", latitude, latCardinal, longitude, longCardinal);
            DrawText(TextFormat("[ %s ]", tmpBuffer), infoCoord.x +150, infoCoord.y+10,20,RAYWHITE);

        


	EndDrawing();
}
	

	// unload fonts
	UnloadFont(defaultFnt);
	UnloadFont(titleFnt);
	UnloadFont(artistFnt);
	UnloadFont(digitFnt);
	UnloadFont(filesFnt);
    UnloadRenderTexture(target);
	CloseWindow();
	return 0;
}
