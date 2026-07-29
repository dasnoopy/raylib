#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH  256
#define HEIGHT 256

const float xCenter = WIDTH/2;
const float yCenter = HEIGHT/2;
const Vector2 center = {xCenter, yCenter};
const float clockRadius = HEIGHT*0.44;
const float hourHandLen = clockRadius * 0.7;
const float minHandLen = clockRadius * 0.9;
const float secHandLen = clockRadius * 0.9;


// NORD colors
#define BACK_COLOR CLITERAL(Color){0, 0, 0, 0}
#define HANDS_COLOR CLITERAL(Color){ 143, 188, 187, 255 }
#define MIN_MARK_COLOR CLITERAL(Color){ 136, 192, 208, 232 }
#define HOUR_MARK_COLOR CLITERAL(Color){ 129, 161, 193, 255 } 
#define TEXT_COLOR CLITERAL(Color){ 94, 129, 172, 255 }
#define ON_COLOR CLITERAL(Color){ 242, 242, 242, 255 }
#define OFF_COLOR CLITERAL(Color){ 128,128,128, 32 } 

void drawRectangleRounded (void)  {
  Rectangle  rect = { 0, 0, WIDTH, HEIGHT};   // toplx, toply, width, height
  float radius = 0.1;                        // rotate degrees
  int     segs = 10;
  Color color = BACK_COLOR;  // red, green, blue, alpha
  DrawRectangleRounded ( rect, radius, segs, color );
}

void TextHour(Color color)
{
int fontsize = 20;
float alpha_deg = 180;
for (int i=12; i>0; i--)
	{
	float x =  (center.x + clockRadius*0.86 * sinf(alpha_deg * DEG2RAD));
	float y =  (center.y + clockRadius*0.86 * cosf(alpha_deg * DEG2RAD));
	alpha_deg += 360 / 12;
	//printf("[%i]: x:%f y:%f\n",i,x,y);
	//int xOffset = i % 3 == 0 ? 0 : -8;
	//int yOffset = i % 12 == 0 ? 0 : 8;
	DrawText (TextFormat("%02i",i), x-10, y-8, fontsize, color );
	}
}

void DrawMinuteMarkers(Color color)
{
float alpha_deg = 0;
for (int i=0; i<60; i++)
  {	
   float x = center.x + clockRadius * sinf(alpha_deg * DEG2RAD);
   float y = center.y + clockRadius * cosf(alpha_deg * DEG2RAD);
   Vector2 coords = {x ,y};

   alpha_deg += 360/60;
   int sides = 4;
   int radius = 3;
   int rotation = 90;

   DrawPoly (coords, sides, radius, rotation, color );
  }
}

void DrawHourMarkers(Color color)
{
	float alpha_deg = 0;
	for (int i=0; i<60; i++)
	  {	
   		float x = center.x + clockRadius * sinf(alpha_deg * DEG2RAD);
		float y = center.y + clockRadius * cosf(alpha_deg * DEG2RAD);
	   Vector2 coords = {x, y};

   	alpha_deg += 360/12;
   	int sides = 4;
   	int radius = 6;
   	int rotation = 90;
   DrawPoly (coords, sides, radius, rotation, color);
  }
}

void DrawHourHand(struct tm *t)
{
	float minute_alpha_progress = ((float) t-> tm_min) / 60.0; 
	float alpha_deg = ((float) (t->tm_hour % 12)) * 30 + 30.0*minute_alpha_progress;
	float x_outer = center.x + hourHandLen * sinf(alpha_deg * DEG2RAD);
	float y_outer = center.y - hourHandLen * cosf(alpha_deg * DEG2RAD);
	Vector2 outer = {x_outer, y_outer};
	DrawLineEx(center, outer,12, HANDS_COLOR);
}

void DrawMinuteHand(struct tm *t)
{
	float second_alpha_progress = ((float) t-> tm_sec) / 60.0; 
	float alpha_deg = ((float) (t->tm_min % 60)) * 6 + 6.0*second_alpha_progress;
	float x_outer = center.x + minHandLen * sinf(alpha_deg * DEG2RAD);
	float y_outer = center.y - minHandLen * cosf(alpha_deg * DEG2RAD);
	Vector2 outer = {x_outer, y_outer};
	DrawLineEx(center, outer,8, HANDS_COLOR);
}

void DrawSecondHand(struct tm *t)
{
	float alpha_deg = ((float) (t->tm_sec)) * 6;
	float x_outer = center.x + secHandLen * sinf(alpha_deg * DEG2RAD);
	float y_outer = center.y - secHandLen * cosf(alpha_deg * DEG2RAD);
	Vector2 outer = {x_outer, y_outer};
	DrawLineEx(center, outer,2, WHITE);
}

void DrawCenter(int radius, Color color)
{
	// draw center of the clock
  int sides = 4;
	int rotation = 0;
	DrawPoly (center, sides, radius, rotation, color);
}

int main (int argc, char *argv[])
{
	 SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TOPMOST );// | FLAG_MSAA_4X_HINT); // | 
	InitWindow(WIDTH, HEIGHT, "Analog Clock");
	// center window on the screen
	SetWindowPosition(GetMonitorWidth(0) / 2 - WIDTH/2, GetMonitorHeight(0) / 2 - HEIGHT/2); 
		SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    //SetWindowOpacity(0.1);
	RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);

  // fai riapparire finestra dopo caricamento iniziale
  ClearWindowState(FLAG_WINDOW_HIDDEN);

 	time_t now = time(NULL); 
	struct tm *t = localtime(&now);
	
	while (!WindowShouldClose())
	{
		BeginTextureMode(target);
		ClearBackground(BACK_COLOR);
		EndTextureMode();

		BeginDrawing();
		ClearBackground (BACK_COLOR);

		// grid
    for (int i = 8; i < WIDTH; i+=16) DrawLine(i,0,i,WIDTH,OFF_COLOR);
    for (int i = 8; i < HEIGHT; i+=16) DrawLine(0,i,HEIGHT,i,OFF_COLOR);
		//drawRectangleRounded();
		
		now = time (NULL);
		t = localtime(&now);
		//DrawCircleV(center, clockRadius,BACK_COLOR);
	    DrawMinuteMarkers(MIN_MARK_COLOR);	
			DrawHourMarkers(HOUR_MARK_COLOR);
			TextHour(TEXT_COLOR);
			DrawCenter(12, HANDS_COLOR);
			DrawHourHand(t);
			DrawMinuteHand(t);
			DrawSecondHand(t);
		EndDrawing();
	}
	
	UnloadRenderTexture(target);
	CloseWindow();
	return 0;


}

