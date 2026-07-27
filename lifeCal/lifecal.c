#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>

#define WIDTH 260 //preferibilmento multiplo di 20
#define HEIGHT 580


// NORD colors
#define BACK_COLOR CLITERAL(Color){46, 52, 64, 255}
#define ON_COLOR CLITERAL(Color){ 128, 128, 130, 255 }
#define OFF_COLOR CLITERAL(Color){ 50, 50, 52, 255 } 

bool isYear = false;
bool isDay = true;
char desc[32] = { '\0' };
int count = 0;
int truncVal = 0;
int dotGap = 0;
int dotSize = 0;
int currPoint = 0;
int startX=0;

struct Date {
int day;
int month;
int year;
};

int isLeapYear(int year) {
return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int month, int year) {
if (month == 2) {
return isLeapYear(year) ? 29 : 28;
}
return (month == 4 || month == 6 || month == 9 || month == 11) ? 30 : 31;
}

int totalDays(struct Date date) {
int total = 0;
for (int y = 0; y < date.year; y++) {
total += isLeapYear(y) ? 366 : 365;
}
for (int m = 1; m < date.month; m++) {
total += daysInMonth(m, date.year);
}
total += date.day;
return total;
}



void updateTexure(RenderTexture2D target, int currPoint) {

int offX=0;
int offY=0;	

    BeginTextureMode(target);
        ClearBackground(BACK_COLOR); // Sfondo della texture

      	      for (int i=0; i<=count - 1 ; ++i) {
                if (i % truncVal == 0) offX=0, offY++;
                  //DrawRectangleRec((Rectangle){startX + (offX*dotGap),136 + (offY*dotGap), dotSize,dotSize}, (i<currPoint-1)?WHITE:(i==currPoint-1)?SKYBLUE:DARKGRAY);
                  DrawCircle(startX + (offX*dotGap),136 + (offY*dotGap), dotSize, (i<currPoint-1)?WHITE:(i==currPoint-1)?RED:DARKGRAY);
                offX++;
                } 

    EndTextureMode();
}




int main (int argc, char *argv[])
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED ); // | FLAG_MSAA_4X_HINT );
	InitWindow(WIDTH, HEIGHT, "lifecal");
	// center window on the screen
	SetWindowPosition(GetMonitorWidth(0) / 2 - WIDTH/2, GetMonitorHeight(0) / 2 - HEIGHT/2); 
        SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

        Font dateFnt = LoadFontEx("fonts/HelveticaNeue-Light.otf", 24, NULL, 0); // all other text
        Font timeFnt = LoadFontEx("fonts/HelveticaNeue-Medium.otf", 96, NULL, 0); 

  	    // Crea la texture di rendering
    RenderTexture2D canvas = LoadRenderTexture(WIDTH, HEIGHT);

    int lastChange = -1;


  	// fai riapparire finestra dopo caricamento iniziale
        ClearWindowState(FLAG_WINDOW_HIDDEN);

  	
while (!WindowShouldClose()) {
//----------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------

                
time_t now = time (NULL);
struct tm *tm_info = localtime(&now);

 		struct Date date1, date2;
		char dateStr[64] = { 0 };
		char timeStr[64] = { 0 };

		date1.day=01;
	        date1.month=01;
	        date1.year = tm_info->tm_year + 1900;
	        
	        date2.day=tm_info->tm_mday;
	        date2.month=tm_info->tm_mon +1;
	        date2.year = tm_info->tm_year + 1900;
	        
	        int days1 = totalDays(date1);
                int days2 = totalDays(date2);
                  
                  
                //date
                strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y", tm_info);
                Vector2 dateSize = MeasureTextEx(dateFnt, dateStr, 24, 0);
                
                //time
		strftime(timeStr, sizeof(timeStr), "%H:%M", tm_info);
		Vector2 timeSize = MeasureTextEx(dateFnt, timeStr, 96, 0);
		
		if (IsKeyPressed(KEY_SPACE)) { isYear=!isYear; isDay=!isDay; }

		
		if (isYear) {
		  strcpy(desc, "Days per year...");
                  count = isLeapYear(tm_info->tm_year + 1900) ? 366 : 365; 
		  truncVal = 15;
		  dotGap=16;
		  dotSize=6;
		  startX=18;
		  currPoint = abs(days2 - days1);
		}
		
		if (isDay) {
		  strcpy(desc, "Minutes per day...");
		  count = 1440;
		  truncVal=30;
		  dotGap=8;
		  dotSize=3;
		  startX=14;
		  currPoint = (tm_info->tm_hour *60) + tm_info->tm_min;
		}
		
		Vector2 descSize = MeasureTextEx(dateFnt, desc, 24, 0);

      // Aggiorna la texture SOLO se il minuto è cambiato
      if (currPoint != lastChange) {
          updateTexure(canvas, currPoint);
          lastChange = currPoint;
      }

//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------	
    BeginDrawing();
		ClearBackground (BACK_COLOR);

        // NOTA: Le RenderTexture in OpenGL sono capovolte verticalmente.
        // Usiamo un'altezza negativa nel rettangolo sorgente per raddrizzarla.
        Rectangle src = { 0.0f, 0.0f, (float)canvas.texture.width, -(float)canvas.texture.height };
        Rectangle dest = { 0.0f, 0.0f, (float)WIDTH, (float)HEIGHT };
        Vector2 origin = { 0.0f, 0.0f };
        // Unica chiamata di disegno per tutti i punti!
        DrawTexturePro(canvas.texture, src, dest, origin, 0.0f, WHITE);
  
      // current date/time
		DrawTextEx(dateFnt,dateStr,(Vector2){ (WIDTH/2) - (dateSize.x/2), 10}, 24,0, LIGHTGRAY);
		DrawTextEx(timeFnt,timeStr,(Vector2){ (WIDTH/2) - (timeSize.x/2), 24}, 96,0, WHITE);
		DrawTextEx(dateFnt,TextFormat("%s",desc),(Vector2){ (WIDTH/2) - (descSize.x/2), 112},24,0,LIGHTGRAY);
	    DrawTextEx(dateFnt,TextFormat("%i left, %i%%",currPoint,(currPoint*100)/count),(Vector2){64,550},24,0,LIGHTGRAY);

 	EndDrawing();
	}  
	
	UnloadFont(dateFnt);
	UnloadFont(timeFnt);
	UnloadRenderTexture(canvas);
	CloseWindow();
	return 0;


}


