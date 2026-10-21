#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <raylib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>

#define screenWidth 200 
#define screenHeight 420

// NORD colors
#define bgColor CLITERAL(Color){10, 20, 30, 248}
#define accentColor CLITERAL(Color){ 136, 192, 208, 255 }
// Color accentColor = SKYBLUE; //CLITERAL(Color){ 198, 208, 245, 255 }

// BCD dots color
#define inner_Color CLITERAL(Color){ 136, 192, 208, 255}
#define outer_Color CLITERAL(Color){ 76, 86, 106, 255 }      // Dark Gray

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

void get_uptime (void) {
    sysinfo(&info);
    snprintf(buffer, sizeof(buffer),"up %02ldh %02ldm", info.uptime / 3600, (info.uptime % 3600) / 60 );
}

void dec2bin(int x, int y, int size, int value, Color outerColor,Color innerColor) 
{
	int a[4] = { 0 };
	for (int i = 0; value > 0; i++)
	{
		a[i] = value % 2;
		value /= 2;
	}
	//for (int j = 3; j>=0 ; j--) DrawCircleGradient ((Vector2){x,y-(j*size)},size/3,a[j]?innerColor:GRAY, a[j]?outerColor:DARKGRAY);
	for (int j = 3; j>=0 ; j--) DrawCircle(x,y-(j*size),size/3,a[j]?innerColor:outerColor);
}

typedef struct {
    Vector2 position;
    Vector2 normal; 
} PathPoint;

// posizione tick lungo il perimtro di un rettangolo, quadrato
PathPoint GetRoundedRectPathPoint(Rectangle rect, float radius, float progress) 
{
    PathPoint result = { 0 };
    
    float w_straight = rect.width - 2.0f * radius;
    float h_straight = rect.height - 2.0f * radius;
    
    float len_top = w_straight;
    float len_corner1 = (PI / 2.0f) * radius;
    float len_right = h_straight;
    float len_corner2 = (PI / 2.0f) * radius;
    float len_bottom = w_straight;
    float len_corner3 = (PI / 2.0f) * radius;
    float len_left = h_straight;
    float len_corner4 = (PI / 2.0f) * radius;
    
    float total_perimeter = len_top + len_corner1 + len_right + len_corner2 + 
                            len_bottom + len_corner3 + len_left + len_corner4;
    
    float target_dist = progress * total_perimeter;
    target_dist = fmodf(target_dist + (len_top / 2.0f), total_perimeter);
    
    Vector2 c_top_right = { rect.x + rect.width - radius, rect.y + radius };
    Vector2 c_bot_right = { rect.x + rect.width - radius, rect.y + rect.height - radius };
    Vector2 c_bot_left  = { rect.x + radius, rect.y + rect.height - radius };
    Vector2 c_top_left  = { rect.x + radius, rect.y + radius };

    if (target_dist < len_top) {
        result.position.x = rect.x + radius + target_dist;
        result.position.y = rect.y;
        result.normal = (Vector2){ 0.0f, 1.0f };
        return result;
    }
    target_dist -= len_top;
    
    if (target_dist < len_corner1) {
        float angle = -PI/2.0f + (target_dist / len_corner1) * (PI/2.0f);
        result.position.x = c_top_right.x + cosf(angle) * radius;
        result.position.y = c_top_right.y + sinf(angle) * radius;
        result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
        return result;
    }
    target_dist -= len_corner1;
    
    if (target_dist < len_right) {
        result.position.x = rect.x + rect.width;
        result.position.y = rect.y + radius + target_dist;
        result.normal = (Vector2){ -1.0f, 0.0f };
        return result;
    }
    target_dist -= len_right;
    
    if (target_dist < len_corner2) {
        float angle = 0.0f + (target_dist / len_corner2) * (PI/2.0f);
        result.position.x = c_bot_right.x + cosf(angle) * radius;
        result.position.y = c_bot_right.y + sinf(angle) * radius;
        result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
        return result;
    }
    target_dist -= len_corner2;
    
    if (target_dist < len_bottom) {
        result.position.x = rect.x + rect.width - radius - target_dist;
        result.position.y = rect.y + rect.height;
        result.normal = (Vector2){ 0.0f, -1.0f };
        return result;
    }
    target_dist -= len_bottom;
    
    if (target_dist < len_corner3) {
        float angle = PI/2.0f + (target_dist / len_corner3) * (PI/2.0f);
        result.position.x = c_bot_left.x + cosf(angle) * radius;
        result.position.y = c_bot_left.y + sinf(angle) * radius;
        result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
        return result;
    }
    target_dist -= len_corner3;
    
    if (target_dist < len_left) {
        result.position.x = rect.x;
        result.position.y = rect.y + rect.height - radius - target_dist;
        result.normal = (Vector2){ 1.0f, 0.0f };
        return result;
    }
    target_dist -= len_left;
    
    float angle = PI + (target_dist / len_corner4) * (PI/2.0f);
    result.position.x = c_top_left.x + cosf(angle) * radius;
    result.position.y = c_top_left.y + sinf(angle) * radius;
    result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
    
    return result;
}


int main (int argc, char *argv[]) 
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TOPMOST); 
	InitWindow(screenWidth, screenHeight, "rcalendar");
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

  Font textFnt = LoadFontEx("fonts/rmplayerdot.otf", 28, NULL, 0); 
  Font nothOS = LoadFontEx("fonts/IosevkaFixedSS13-SemiBold.ttf", 72, NULL, 0); 
  Font calFnt = LoadFontEx("fonts/IosevkaFixedSS13-SemiBold.ttf", 16, NULL, 0); // all other text

    // dimensioni orologio
    Rectangle clockRect = { 18,52,screenWidth-36,88};
    
    // roundess globale orologio
    // 0.0f quadrato / max dipende da dimensioni..fare delle prove!
    float cornerRadius = 14.0f; 

    SetTargetFPS(5); // disegna 5 fps al secondo.. dovrebbe baastare visto che cmq tutto mi si aggiorna ogni secondo

    // fai riapparire finestra dopo averla nascosta durante il carimento iniziale
    SetWindowSize(screenWidth,screenHeight);
    //SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  
    SetWindowPosition(8,GetMonitorHeight(0) - screenHeight -8);  
    ClearWindowState(FLAG_WINDOW_HIDDEN);

while (!WindowShouldClose()) {
//
// Update
//

	    Vector2 center = { clockRect.x + clockRect.width / 2.0f, clockRect.y + clockRect.height / 2.0f };
	    
	    // do something when  window loses focus
        if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) SetWindowOpacity(0.80f);
        else SetWindowOpacity(1.0f);
//
// Draw
// 
		BeginDrawing();
		ClearBackground (bgColor);

        //DrawRectangleLines(0,0,screenWidth,screenHeight, outer_Color);
		
        time_t now = time (NULL);
		struct tm *t = localtime(&now);

		month = t->tm_mon + 1; // current month
		year = t->tm_year + 1900; // current year

		// format date and center horizontally
		strftime(buffer, sizeof(buffer), "%a, %d %b", t);
		Vector2 datePos = MeasureTextEx(textFnt, buffer, textFnt.baseSize, 0);
		DrawTextEx(textFnt, TextFormat("%s", buffer), (Vector2){(screenWidth-datePos.x)/2, 12}, textFnt.baseSize,0, WHITE);

            // Disegno dei 60 secondi
            for (int i = 0; i < 60; i++) 
            {
                float progress = (float)i / 60.0f;
                PathPoint pt = GetRoundedRectPathPoint(clockRect, cornerRadius, progress);
                
                float thickness = 2.0f; // diametro punti secondi
				Color markerColor = outer_Color; 
                
                Vector2 drawNormal;
                float dirX = center.x - pt.position.x;
                float dirY = center.y - pt.position.y;
                float len = sqrtf(dirX * dirX + dirY * dirY);
                
                if (len > 0.0f) {
                    drawNormal.x = dirX / len;
                    drawNormal.y = dirY / len;
                } 
                else drawNormal = pt.normal;

                if (i % 15 == 0) // marker ogni 15 min in evidenza con diametro e colore diversi
                {
                    thickness = 4.0f;
                    markerColor = WHITE; 
                }

                // disegna secondo che avanza
				if (i == t->tm_sec) {
					thickness = 4.0f;
					markerColor = accentColor; 
				}

                // distanza dal bordo dei marker
                float edgeOffset = -1.0f;//cambiando offset cambia anche problema marker con andamento a onda
                Vector2 startPoint = {
                    pt.position.x + drawNormal.x * edgeOffset,
                    pt.position.y + drawNormal.y * edgeOffset
                };
                DrawCircle(startPoint.x, startPoint.y, thickness, markerColor);
                }

		// format clock and centering horizontally
		snprintf(buffer, sizeof(buffer),"%02i%02i", t->tm_hour, t->tm_min);
		Vector2 timePos = MeasureTextEx(nothOS, buffer, nothOS.baseSize, 0);
		DrawTextEx(nothOS, TextFormat("%s", buffer), (Vector2){(screenWidth - timePos.x)/2, 60}, nothOS.baseSize,0, accentColor);


		// monthly calendar header and week number
		strftime(buffer, sizeof(buffer), "%B %G (w%V)", t);
		// calendar header
		Vector2 mywPos = MeasureTextEx(calFnt, buffer, calFnt.baseSize, 0);
		DrawTextEx(calFnt,TextFormat("%s", buffer),(Vector2){(screenWidth/2)-(mywPos.x/2),156},calFnt.baseSize,0,LIGHTGRAY);
		DrawTextEx(calFnt,"Mon Tue Wed Thu Fri Sat Sun", (Vector2){20,172},calFnt.baseSize,0,GRAY);

		// monthly calendar
		start = getStartDay(month, year);
		int days = getDays(month, year);
		Color dayColor;
		int offX=22;
		int offY=188;
		// calcola offset primo giorno del mese/settimanaa
		for (int y = 0; y < start; y++) offX+=25;
		// stampa giorni del mese
		for (int i = 1; i <= days; i++)  {
			if ( (i + start) % 7  == 0 || 6 - ((i + start)%7) == 0 )  dayColor = inner_Color;
			else dayColor=WHITE;
			if (i == t->tm_mday) DrawCircle(offX+7,offY+8,10,accentColor);
			DrawTextEx(calFnt,TextFormat("%02d",i),(Vector2){offX,offY},calFnt.baseSize,0,(i==t->tm_mday)?BLACK:dayColor);
		   	offX = offX + 24;
		    if ( (i + start) % 7  == 0) {
		    	offX = 22;
		        offY = offY + 20;
		    }
		}

		//BDC clock
		snprintf(buffer, sizeof(buffer),"%02i:%02i:%02i", t->tm_hour, t->tm_min,t->tm_sec);
		int num;
		int count = 0;
		int size = 18;
		int dist = 4;
		for (int i = 0; buffer[i] != '\0' && count < 6; i++)
		{
			 if (isdigit((unsigned char)buffer[i])) 
			{
				num = buffer[i] - '0';
				dec2bin(44+(count*(size+dist)),360,size,num,outer_Color,accentColor);
				count++;
			}
		}

		// get uptime and horizontal text centering
	    get_uptime();
	    Vector2 uptimePos = MeasureTextEx(textFnt, buffer, textFnt.baseSize, 0);
	    DrawTextEx(textFnt, TextFormat("%s", buffer), (Vector2){(screenWidth - uptimePos.x)/2, screenHeight-40}, textFnt.baseSize,0, WHITE);


	EndDrawing();
	}
	
  UnloadFont(textFnt);
  UnloadFont(nothOS);
  UnloadFont(calFnt);
	CloseWindow();
	return 0;

}
