#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH  240
#define HEIGHT 240

const float xCenter = WIDTH/2;
const float yCenter = HEIGHT/2;
const Vector2 center = {xCenter, yCenter};
const float clockRadius = HEIGHT*0.44;
const float hourHandLen = clockRadius * 0.7;
const float minHandLen = clockRadius * 0.9;
const float secHandLen = clockRadius * 0.9;


// NORD colors
#define BACK_COLOR CLITERAL(Color){10, 10, 15, 192}
#define HANDS_COLOR CLITERAL(Color){ 143, 188, 187, 255 }
#define MIN_MARK_COLOR CLITERAL(Color){ 136, 192, 208, 232 }
#define HOUR_MARK_COLOR CLITERAL(Color){ 129, 161, 193, 255 } 
#define TEXT_COLOR CLITERAL(Color){ 94, 129, 172, 255 }
#define ON_COLOR CLITERAL(Color){ 242, 242, 242, 255 }
#define OFF_COLOR CLITERAL(Color){ 132,132,137, 32 } 

void drawRectangleRounded (int X, int Y, int W, int H, Color color)  {
  Rectangle  rect = { X, Y, W, H};   // toplx, toply, width, height
  float radius = 0.2;                        // rotate degrees
  int     segs = 10;
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
   // Vector2 coords = {x ,y};

   alpha_deg += 360/60;
   // int sides = 4;
   // int radius = 3;
   // int rotation = 90;
 DrawCircle (x,y,2,color);
  }
}

void DrawHourMarkers(Color color)
{
	float alpha_deg = 0;
	for (int i=0; i<60; i++)
	  {	
   		float x = center.x + clockRadius * sinf(alpha_deg * DEG2RAD);
		float y = center.y + clockRadius * cosf(alpha_deg * DEG2RAD);
	  // Vector2 coords = {x, y};

   	alpha_deg += 360/12;
   	// int sides = 4;
   	// int radius = 6;
   	// int rotation = 90;
   DrawCircle (x,y,4,color);
  }
}

void DrawHourHand(struct tm *t)
{
	float minute_alpha_progress = ((float) t-> tm_min) / 60.0; 
	float alpha_deg = ((float) (t->tm_hour % 12)) * 30 + 30.0*minute_alpha_progress;
	float x_outer = center.x + hourHandLen * sinf(alpha_deg * DEG2RAD);
	float y_outer = center.y - hourHandLen * cosf(alpha_deg * DEG2RAD);
	Vector2 outer = {x_outer, y_outer};
	DrawLineEx(center, outer,10, WHITE);
}

void DrawMinuteHand(struct tm *t)
{
	float second_alpha_progress = ((float) t-> tm_sec) / 60.0; 
	float alpha_deg = ((float) (t->tm_min % 60)) * 6 + 6.0*second_alpha_progress;
	float x_outer = center.x + minHandLen * sinf(alpha_deg * DEG2RAD);
	float y_outer = center.y - minHandLen * cosf(alpha_deg * DEG2RAD);
	Vector2 outer = {x_outer, y_outer};
	DrawLineEx(center, outer,6, LIGHTGRAY);
}

void DrawSecondHand(struct tm *t)
{
	float alpha_deg = ((float) (t->tm_sec)) * 6;
	float x_outer = center.x + secHandLen * sinf(alpha_deg * DEG2RAD);
	float y_outer = center.y - secHandLen * cosf(alpha_deg * DEG2RAD);
	Vector2 outer = {x_outer, y_outer};
	DrawLineEx(center, outer,2, GRAY);
}

void DrawCenter(int radius, Color color)
{
	// draw center of the clock
  // int sides = 4;
	// int rotation = 0;
	DrawCircle (center.x,center.y,radius,color);
}

int main (int argc, char *argv[])
{
	 SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TOPMOST );// | FLAG_MSAA_4X_HINT); // | 
	InitWindow(WIDTH, HEIGHT, "Analog Clock");
	// center window on the screen
	SetWindowPosition(8, GetMonitorHeight(0)- (HEIGHT + 8)); 
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
	RenderTexture2D target = LoadRenderTexture(WIDTH, HEIGHT);

  // fai riapparire finestra dopo caricamento iniziale
  ClearWindowState(FLAG_WINDOW_HIDDEN);

 	time_t now = time(NULL); 
	struct tm *t = localtime(&now);
	
	while (!WindowShouldClose())
	{
		BeginTextureMode(target);
		ClearBackground(BLANK);
		EndTextureMode();

		BeginDrawing();
		ClearBackground (BLANK);

		// grid
    // for (int i = 8; i < WIDTH; i+=16) DrawLine(i,0,i,HEIGHT,OFF_COLOR);
    // for (int i = 8; i < HEIGHT; i+=16) DrawLine(0,i,WIDTH,i,OFF_COLOR);
		drawRectangleRounded(0,0,WIDTH,HEIGHT,BACK_COLOR);
		
		now = time (NULL);
		t = localtime(&now);
		//DrawCircleV(center, clockRadius,BACK_COLOR);
	    DrawMinuteMarkers(GRAY);	
			DrawHourMarkers(LIGHTGRAY);
			TextHour(WHITE);

			DrawHourHand(t);
			DrawMinuteHand(t);
			DrawSecondHand(t);

			DrawCenter(10, RED);
		EndDrawing();
	}
	
	UnloadRenderTexture(target);
	CloseWindow();
	return 0;


}

