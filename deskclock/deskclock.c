#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>

#define WIDTH 240 //preferibilmento multiplo di 60
#define HEIGHT 180


// NORD colors
#define BACK_COLOR CLITERAL(Color){0, 0, 0, 0}
#define ON_COLOR CLITERAL(Color){ 242, 242, 242, 255 }
#define OFF_COLOR CLITERAL(Color){ 128,128,128, 32 } 
// digits style
#define SEGMENT_WIDTH WIDTH/16
#define SEGMENT_THICKNESS SEGMENT_WIDTH/2
#define OFFSET SEGMENT_THICKNESS*1.2
// digits size and positioning
#define START_X WIDTH/10
#define DIGIT_DISTANCE SEGMENT_WIDTH*2.2
#define COLON_DISTANCE DIGIT_DISTANCE/1.4
#define COLON_RADIUS SEGMENT_THICKNESS/1.6

struct sysinfo info;
char uptime_str[64];

int digits[10][7] = { {1,1,1,0,1,1,1}, //digit 0
		   {0,0,1,0,0,1,0}, //digit 1
		   {1,0,1,1,1,0,1}, //digit 2
		   {1,0,1,1,0,1,1}, //digit 3
		   {0,1,1,1,0,1,0}, //digit 4
		   {1,1,0,1,0,1,1}, //digit 5
		   {1,1,0,1,1,1,1}, //digit 6
		   {1,0,1,0,0,1,0}, //digit 7
		   {1,1,1,1,1,1,1}, //digit 8
		   {1,1,1,1,0,1,1}};//digit 9

void DrawSegment(Vector2 center, bool HORIZONTAL, Color color)
{
    // Create Real display segment
    int count = 6;
    Vector2 a,b,c,d,e,f;
    if (HORIZONTAL)
    {
      a = (Vector2){center.x - SEGMENT_WIDTH/2 - SEGMENT_THICKNESS/2, center.y};
      b = (Vector2){center.x - SEGMENT_WIDTH/2, center.y + SEGMENT_THICKNESS/2};
      c = (Vector2){center.x - SEGMENT_WIDTH/2, center.y - SEGMENT_THICKNESS/2};
      d = (Vector2){center.x + SEGMENT_WIDTH/2, center.y + SEGMENT_THICKNESS/2};
      e = (Vector2){center.x + SEGMENT_WIDTH/2, center.y - SEGMENT_THICKNESS/2};
      f = (Vector2){center.x + SEGMENT_WIDTH/2 + SEGMENT_THICKNESS/2, center.y};
    }
    else
    {
      a = (Vector2){center.x, center.y - SEGMENT_WIDTH/2 - SEGMENT_THICKNESS/2};
      b = (Vector2){center.x - SEGMENT_THICKNESS/2, center.y - SEGMENT_WIDTH/2};
      c = (Vector2){center.x + SEGMENT_THICKNESS/2, center.y - SEGMENT_WIDTH/2};
      d = (Vector2){center.x - SEGMENT_THICKNESS/2, center.y + SEGMENT_WIDTH/2};
      e = (Vector2){center.x + SEGMENT_THICKNESS/2, center.y + SEGMENT_WIDTH/2};
      f = (Vector2){center.x, center.y + SEGMENT_WIDTH/2 + SEGMENT_THICKNESS/2};
    }
    
    Vector2 points[] = {a,b,c,d,e,f};
    DrawTriangleStrip(points, count, color);
}

void DrawDots(int x, int y, int seconds)
{
	// poly-lines
  Vector2 center1 = { x, y - SEGMENT_WIDTH/2 };
  Vector2 center2 = { x, y + SEGMENT_WIDTH/2 };
  int   sides    = 4;
  float radius   = SEGMENT_THICKNESS/2;
  float rotation = 90;
  DrawPoly ( center1, sides, radius, rotation, seconds % 2 ? ON_COLOR : OFF_COLOR); // n sided filled polygon (Vector version)
  DrawPoly ( center2, sides, radius, rotation, seconds % 2 ? ON_COLOR : OFF_COLOR);   // n sided filled polygon (Vector version)
}

void DrawDigit (Vector2 center,int digit)
{
  // find out which segments to draw in which color
  int *digit_segments = digits[digit];

  // draw segments
  Vector2 primo = {center.x, center.y - SEGMENT_WIDTH - OFFSET};
  DrawSegment (primo, true, digit_segments[0] ? ON_COLOR : OFF_COLOR );
  
  Vector2 secondo = {center.x - SEGMENT_WIDTH/2 - OFFSET/2, center.y - SEGMENT_WIDTH/2 - OFFSET/2};
  DrawSegment (secondo, false, digit_segments[1] ? ON_COLOR : OFF_COLOR );
  
  Vector2 terzo = {center.x + SEGMENT_WIDTH/2 + OFFSET/2, center.y - SEGMENT_WIDTH/2 - OFFSET/2};
  DrawSegment (terzo, false, digit_segments[2] ? ON_COLOR : OFF_COLOR );
  
  Vector2 quarto = {center.x, center.y};
  DrawSegment (quarto, true, digit_segments[3] ? ON_COLOR : OFF_COLOR );
  
  Vector2 quinto = {center.x - SEGMENT_WIDTH/2 - OFFSET/2, center.y + SEGMENT_WIDTH/2 + OFFSET/2};
  DrawSegment (quinto, false, digit_segments[4] ? ON_COLOR : OFF_COLOR );
  
  Vector2 sesto = {center.x + SEGMENT_WIDTH/2 + OFFSET/2, center.y + SEGMENT_WIDTH/2 + OFFSET/2};
  DrawSegment (sesto, false, digit_segments[5] ? ON_COLOR : OFF_COLOR );
  
  Vector2 settimo = {center.x , center.y + SEGMENT_WIDTH + OFFSET};
  DrawSegment (settimo, true, digit_segments[6] ? ON_COLOR : OFF_COLOR );
}

void DrawTime(int hours, int minutes, int seconds)
{
//hours
  float x = START_X;
  DrawDigit((Vector2){x, HEIGHT/3},hours / 10);
  x += DIGIT_DISTANCE;
  DrawDigit((Vector2){x, HEIGHT/3},hours % 10);
//colon
  x += COLON_DISTANCE;
  DrawDots(x,HEIGHT/3,seconds);
//minutes
  x += COLON_DISTANCE;
  DrawDigit((Vector2){x, HEIGHT/3}, minutes / 10);
  x += DIGIT_DISTANCE;
  DrawDigit((Vector2){x, HEIGHT/3}, minutes % 10);
//colons  
  x += COLON_DISTANCE;
 DrawDots(x, HEIGHT/3,seconds);
//seconds
  x += COLON_DISTANCE;
  DrawDigit((Vector2){x, HEIGHT/3},seconds / 10);
  x += DIGIT_DISTANCE;
  DrawDigit((Vector2){x, HEIGHT/3},seconds % 10);
}


void get_uptime (void)
{
    sysinfo(&info);
    snprintf(uptime_str, sizeof(uptime_str),"uptime   : %02ldh %02ldm", info.uptime / 3600, (info.uptime % 3600) / 60 );
}

int main (int argc, char *argv[])
{
	 SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_TOPMOST); // | 
	InitWindow(WIDTH, HEIGHT, "Digital Clock");
	// center window on the screen
	SetWindowPosition(0, GetMonitorHeight(0) ); 
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

  Font textFnt = LoadFontEx("fonts/rmplayerdot.otf", 28, NULL, 0); 
  Font dgtFnt = LoadFontEx("fonts/rmdigit.otf", 20, NULL, 0); 
	RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);	


    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);


	while (!WindowShouldClose())
	{
		BeginTextureMode(target);
		ClearBackground(BACK_COLOR);
		EndTextureMode();

		BeginDrawing();
		ClearBackground (BACK_COLOR);
		//drawRectangleRounded();
		time_t now = time (NULL);
		struct tm *t = localtime(&now);
		DrawTime(t->tm_hour, t->tm_min, t->tm_sec);
    DrawLine(10,28,240,28,DARKGRAY);
		DrawTextEx(textFnt, TextFormat("%02i/%02i/%04i", t->tm_mday, t->tm_mon +1, t->tm_year + 1900), (Vector2){WIDTH/4, 0}, 28,0, LIGHTGRAY);
    DrawLine(10,94,240,94,DARKGRAY);
    get_uptime();
    DrawTextEx(textFnt, TextFormat("%s", uptime_str), (Vector2){WIDTH/8, 96}, 28,0, LIGHTGRAY);
		//DrawText("Digital Clock v1.0 @2026 by Andrea Antolini", 12, 8 ,20, YELLOW);
		EndDrawing();
	}
	
	UnloadRenderTexture(target);
  UnloadFont(textFnt);
  UnloadFont(dgtFnt);
	CloseWindow();
	return 0;


}

