#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>

#define WIDTH 180 //preferibilmento multiplo di 60
#define HEIGHT 160


// NORD colors
#define BACK_COLOR CLITERAL(Color){25, 29, 27, 196}
#define ON_COLOR CLITERAL(Color){ 215, 25, 33, 255 }

struct sysinfo info;
char uptime_str[64];

void drawRectangleRounded (int X, int Y, int W, int H, Color color)  {
  Rectangle  rect = { X, Y, W, H};   // toplx, toply, width, height
  float radius = 0.2;                        // rotate degrees
  int     segs = 12;
  DrawRectangleRounded ( rect, radius, segs, color );
}

void get_uptime (void) {
    sysinfo(&info);
    snprintf(uptime_str, sizeof(uptime_str),"%02ldh %02ldm", info.uptime / 3600, (info.uptime % 3600) / 60 );
}

int main (int argc, char *argv[]) 
{
	 SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TOPMOST); // | 
	InitWindow(WIDTH, HEIGHT, "deskclock");
	SetWindowPosition(8, GetMonitorHeight(0)- (HEIGHT + 8)); 
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

  Font textFnt = LoadFontEx("fonts/rmplayerdot.otf", 28, NULL, 0); 
  Font nothOS = LoadFontEx("fonts/NType82.otf", 88, NULL, 0); 
	RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);	

  // fai riapparire finestra dopo caricamento iniziale
  ClearWindowState(FLAG_WINDOW_HIDDEN);

	while (!WindowShouldClose())
	{
		BeginTextureMode(target);
		ClearBackground(BLANK);
		EndTextureMode();

		BeginDrawing();
		ClearBackground (BLANK);

		drawRectangleRounded(0,0,WIDTH,HEIGHT,BACK_COLOR);

		time_t now = time (NULL);
		struct tm *t = localtime(&now);
    DrawTextEx(nothOS, TextFormat("%02i:%02i", t->tm_hour, t->tm_min), (Vector2){10, HEIGHT/5}, 88,0, WHITE);
    //DrawLine(10,28,240,28,ORANGE);
		DrawTextEx(textFnt, TextFormat("%02i.%02i.%04i", t->tm_mday, t->tm_mon +1, t->tm_year + 1900), (Vector2){WIDTH/6, 12}, 28,0, SKYBLUE);
    //DrawLine(10,94,240,94,ORANGE);
    get_uptime();
    DrawTextEx(textFnt, TextFormat("%s", uptime_str), (Vector2){WIDTH/4, HEIGHT-36}, 28,0, SKYBLUE);
		//DrawText("Digital Clock v1.0 @2026 by Andrea Antolini", 12, 8 ,20, YELLOW);
		EndDrawing();
	}
	
	UnloadRenderTexture(target);
  UnloadFont(textFnt);
  UnloadFont(nothOS);
	CloseWindow();
	return 0;

}

