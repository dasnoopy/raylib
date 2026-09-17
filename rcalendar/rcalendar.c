#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>

#define WIDTH 200 //preferibilmento multiplo di 60
#define HEIGHT 280


// NORD colors
#define BACK_COLOR CLITERAL(Color){46, 52, 64, 232}
#define ON_COLOR CLITERAL(Color){ 136, 192, 208, 255 }

struct sysinfo info;
char buffer[64]= { 0 };

int month, year, start; 
const char *months[] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};


int isLeap(int year) { 
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0); 
}

int getDays(int month, int year) {
  int days[] = {31, isLeap(year) ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; 
  return days[month - 1]; 
}

int getStartDay(int month, int year) {
 int y = year, m = month; 

 if (m < 3) { 
      m += 12; 
      y--;
 } 

    int k = y % 100, j = y / 100; 
    int d = (1 + 13*(m + 1)/5 + k + k/4 + j/4 + 5*j) % 7; 
    return (d + 5) % 7; // Sunday = 0
}

void drawRectangleRounded (int X, int Y, int W, int H, Color color)  {
  Rectangle  rect = { X, Y, W, H};   // toplx, toply, width, height
  float radius = 0.086;                        // rotate degrees
  int     segs = 12;
  DrawRectangleRounded ( rect, radius, segs, color );
}

void get_uptime (void) {
    sysinfo(&info);
    snprintf(buffer, sizeof(buffer),"up %02ldh %02ldm", info.uptime / 3600, (info.uptime % 3600) / 60 );
}

int main (int argc, char *argv[]) 
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_TRANSPARENT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TOPMOST); // | 
	InitWindow(WIDTH, HEIGHT, "rcalendar");
	SetWindowPosition(8, GetMonitorHeight(0)- HEIGHT - 8); 
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

  Font textFnt = LoadFontEx("fonts/rmplayerdot.otf", 28, NULL, 0); 
  Font nothOS = LoadFontEx("fonts/NType82-Headline.otf", 88, NULL, 0); 
  Font calFnt = LoadFontEx("fonts/PixelOperator.ttf", 16, NULL, 0); // all other text
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

		month = t->tm_mon + 1; // current month
		year = t->tm_year + 1900; // current year

		// format date and center horizontally
		strftime(buffer, sizeof(buffer), "%a, %d %b", t);
		Vector2 datePos = MeasureTextEx(textFnt, buffer, 28, 0);
		DrawTextEx(textFnt, TextFormat("%s", buffer), (Vector2){(WIDTH-datePos.x)/2, 8}, 28,0, ON_COLOR);

		// format clock and centering horizontally
		snprintf(buffer, sizeof(buffer),"%02i:%02i", t->tm_hour, t->tm_min);
		Vector2 timePos = MeasureTextEx(nothOS, buffer, 88, 0);
		DrawTextEx(nothOS, TextFormat("%s", buffer), (Vector2){(WIDTH - timePos.x)/2, 24}, 88,0, WHITE);

		DrawTextEx(calFnt,TextFormat("%s %04i", months[month], t->tm_year + 1900),(Vector2){12,108},16,0,ON_COLOR);
		DrawTextEx(calFnt,"Mon Tue Wed Thu Fri Sat Sun", (Vector2){12,124},16,0,LIGHTGRAY);

		// monthly calendar
		start = getStartDay(month, year);
		int days = getDays(month, year);

		int offX=18;
		int offY=144;
		// calcola offset primo giorno del mesesettimanaa
		for (int y = 0; y < start; y++) offX+=25;
		// stampa giorni del mese
		for (int i = 1; i <= days; i++)  {
			DrawTextEx(calFnt,TextFormat("%02d",i),(Vector2){offX,offY},16,0,(i==t->tm_mday)?ON_COLOR:WHITE);
		   	offX = offX + 25;
		    if ( (i + start) % 7  == 0) {
		    	offX = 18;
		        offY = offY + 20;
		    }
		}

		// get uptime and horizontal text centering
	    get_uptime();
	    Vector2 uptimePos = MeasureTextEx(textFnt, buffer, 28, 0);
	    DrawTextEx(textFnt, TextFormat("%s", buffer), (Vector2){(WIDTH - uptimePos.x)/2, HEIGHT-36}, 28,0, ON_COLOR);

	EndDrawing();
	}
	
  UnloadRenderTexture(target);
  UnloadFont(textFnt);
  UnloadFont(nothOS);
  UnloadFont(calFnt);
	CloseWindow();
	return 0;

}
