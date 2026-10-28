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
#define TOOL_VERSION            "0.9.5"

#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>

// gcc -Wall -Werror simplayer.c  -o simplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

// window initial size
const int screenWidth = 1080;
const int screenHeight = 572;
#define PI 3.14159265358979323846f
#define RAD(deg) ((deg) * PI / 180.0f)
#define SECONDS_PER_DAY 86400 // Secondi in un giorno (24 ore)
#define PATH_RESOLUTION 200 

typedef struct {
    float lon; // in radianti
    float lat; // in radianti
} SunPosition;

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
char lonCardinal[6] = "";

char tempBuffer[64] = { 0 };
char dateBuffer[64] = { 0 };

// Struttura per gestire lo stato della ripetizione dei tasti
typedef struct {
    int currentKey;       // Il tasto attualmente premuto/tracciato
    float timer;         // Timer per calcolare i delay
    bool isRepeating;    // true se ha superato il delay iniziale
    float firstDelay;    // Tempo di attesa iniziale (es. 0.4 secondi)
    float repeatRate;    // Intervallo di ripetizione (es. 0.05 secondi)
} KeyRepeater;

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------

void drawRectangleRounded (int X, int Y, int W, int H, float radius, Color color)  {
// raylib vuole il "roundness" espresso come frazione rispetto al lato più corto (300)
  Rectangle  rect = { X, Y, W, H};   // toplx, toply, width, height
  int segments = 36;
  DrawRectangleRounded ( rect, radius, segments, color );
}

Vector2 MapGeoToPixel(float lon, float lat, int screenWidth, int screenHeight) {
    Vector2 pixel;
    pixel.x = ((lon + PI) / (2.0f * PI)) * screenWidth;
    pixel.y = ((PI / 2.0f - lat) / PI) * screenHeight;
    return pixel;
}

// Calcola la posizione del sole basandosi su un timestamp time_t arbitrario
SunPosition CalculateSunPositionFromTime(time_t customTime) {
    SunPosition pos;
    struct tm *utcTime = gmtime(&customTime);

    if (utcTime == NULL) {
        pos.lon = 0.0f;
        pos.lat = 0.0f;
        return pos;
    }

    // 1. CALCOLO DELLA LONGITUDINE SOLARE
    float utcHours = utcTime->tm_hour + (utcTime->tm_min / 60.0f) + (utcTime->tm_sec / 3600.0f);
    float lonDeg = (12.0f - utcHours) * 15.0f;
    pos.lon = RAD(lonDeg);

    // 2. CALCOLO DELLA LATITUDINE SOLARE (Declinazione)
    int dayOfYear = utcTime->tm_yday + 1;
    float latDeg = 23.44f * sinf(RAD(360.0f / 365.0f * (dayOfYear - 80)));
    pos.lat = RAD(latDeg);

    return pos;
}

// Funzione che aggiorna lo stato e restituisce true se l'azione deve essere eseguita
bool UpdateKeyRepeater(KeyRepeater *repeater, int key, float dt) {
    // Se il tasto passato non è premuto, non fare nulla
    if (!IsKeyDown(key)) {
        // Se era il tasto che stavamo tracciando, resettiamo lo stato
        if (repeater->currentKey == key) {
            repeater->currentKey = 0;
            repeater->timer = 0.0f;
            repeater->isRepeating = false;
        }
        return false;
    }

    // Se un nuovo tasto viene premuto (o cambiamo direzione), resettiamo il timer sul nuovo tasto
    if (repeater->currentKey != key) {
        repeater->currentKey = key;
        repeater->timer = 0.0f;
        repeater->isRepeating = false;
        return true; // Ritorna true immediatamente per il primo click (reattività)
    }

    // Se stiamo continuando a tenere premuto lo stesso tasto
    repeater->timer += dt;

    if (!repeater->isRepeating) {
        // Fase 1: Aspetta il delay iniziale
        if (repeater->timer >= repeater->firstDelay) {
            repeater->timer = 0.0f;
            repeater->isRepeating = true;
            return true;
        }
    } else {
        // Fase 2: Ripetizione continua a ritmo fisso
        if (repeater->timer >= repeater->repeatRate) {
            repeater->timer -= repeater->repeatRate;
            return true;
        }
    }

    return false;
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
	Font defaultFnt= LoadFontEx("fonts/PixelOperator.ttf", 24, NULL, 0); 


		// Create framebuffer
		RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
		SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);


		//world background image
		Image image = LoadImage("assets/map.png");     // Loaded in CPU memory (RAM)
		Texture2D worldMap = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
		UnloadImage(image); 

	    Shader dayNightShader = LoadShader(0, "daynight.fs");
	    int sunPosLoc = GetShaderLocation(dayNightShader, "sunPosGeo");

	    // Inizializziamo il tempo simulato con l'orario reale del sistema
	    time_t simulatedTime = time(NULL);
	    SunPosition currentSun = CalculateSunPositionFromTime(simulatedTime);

	    Vector2 eclipticPath[PATH_RESOLUTION];
	    bool updatePath = true;

	    // Calcoliamo una volta sola le coordinate Y fisse per i Tropici e l'Equatore
    	float yCancro = MapGeoToPixel(0.0f, RAD(23.44f), mapCoord.width, mapCoord.height).y;
    	float yEquatore = MapGeoToPixel(0.0f, RAD(0.0f), mapCoord.width, mapCoord.height).y;
    	float yCapricorno = MapGeoToPixel(0.0f, RAD(-23.44f), mapCoord.width, mapCoord.height).y;


    // Inizializzazione del ripetitore (400ms delay iniziale, 80ms tra le ripetizioni)
    KeyRepeater repeater = {
        .currentKey = 0,
        .timer = 0.0f,
        .isRepeating = false,
        .firstDelay = 0.40f,
        .repeatRate = 0.08f
    };

	// fai riapparire finestra dopo averla nascosta durante il carimento iniziale
	SetWindowSize(screenWidth,screenHeight);
	SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  
	//SetWindowPosition(8,40);  
	ClearWindowState(FLAG_WINDOW_HIDDEN);

while (!WindowShouldClose()) {

//----------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------

		float dt = GetFrameTime();

        Vector2 mousePos = GetMousePosition();   // abilitare se cursore libero   
        int stepParallel = 3; //3 = 1 grado
        int stepMeridian = 3; //3 = 1 grado

        // nascondi cursore quando sopra worldmap
        if (CheckCollisionPointRec(mousePos,mapCoord)) HideCursor(); 
        else ShowCursor();

        // gestione cambio data in real time
        bool timeChanged = false;

        // Gestione Input della tastiera (Frecce Destra / Sinistra)
        // implementa funzione  keyrepeater
        if (UpdateKeyRepeater(&repeater, KEY_RIGHT, dt)) {
            simulatedTime += SECONDS_PER_DAY;
            timeChanged = true;
        }
        if (UpdateKeyRepeater(&repeater, KEY_LEFT, dt)) {
            simulatedTime -= SECONDS_PER_DAY;
            timeChanged = true;
        }
        
        // Tasto R per resettare al tempo reale corrente
        if (IsKeyPressed(KEY_R)) {
            simulatedTime = time(NULL);
            timeChanged = true;
        }

        // Calcola la nuova posizione del sole solo se l'utente ha modificato il tempo
        if (timeChanged || IsWindowResized() || updatePath) {
            currentSun = CalculateSunPositionFromTime(simulatedTime);

            // traiettoria del sole
            struct tm *baseTm = gmtime(&simulatedTime);
            if (baseTm != NULL) {
                for (int i = 0; i < PATH_RESOLUTION; i++) {
                    float t = (float)i / (float)(PATH_RESOLUTION - 1);
                    float pathLon = -PI + t * (2.0f * PI);
                    
                    float lonDeg = pathLon * (180.0f / PI);
                    float targetUtcHours = 12.0f - (lonDeg / 15.0f);
                    if (targetUtcHours < 0.0f) targetUtcHours += 24.0f;
                    if (targetUtcHours >= 24.0f) targetUtcHours -= 24.0f;

                    int targetDay = (int)(t * 365.0f) % 365;

                    float latDeg = 23.44f * sinf(RAD(360.0f / 365.0f * (targetDay - 80)));
                    float pathLat = RAD(latDeg);

                    eclipticPath[i] = MapGeoToPixel(pathLon, pathLat, screenWidth, screenHeight);
                }
            }
            updatePath = false;
        }

        // Passa i dati aggiornati alla GPU
        Vector2 sunPosShaderValue = { currentSun.lon, currentSun.lat };
        SetShaderValue(dayNightShader, sunPosLoc, &sunPosShaderValue, SHADER_UNIFORM_VEC2);

        struct tm *localSimTime = localtime(&simulatedTime);
        if (localSimTime != NULL) {
            strftime(dateBuffer, sizeof(dateBuffer), "%d-%m-%Y %H:%M:%S", localSimTime);
        } else TextCopy (dateBuffer, "Data Sconosciuta");

        Vector2 sunPixelPos = MapGeoToPixel(currentSun.lon, currentSun.lat, screenWidth, screenHeight);


            // --- LOGICA DELLE STAGIONI INTERATTIVE ---
            int mese = localSimTime->tm_mon + 1; // tm_mon va da 0 a 11
            int giorno = localSimTime->tm_mday;
            
            const char* testoStagione = "";
            Color coloreStagione = LIGHTGRAY;

            // Primavera (Dal 21 Marzo al 20 Giugno)
            if ((mese == 3 && giorno >= 21) || mese == 4 || mese == 5 || (mese == 6 && giorno <= 20)) {
                if (mese == 3 && giorno == 21) {
                    testoStagione = "Cambio stagione: equinozio di primavera!";
                    coloreStagione = GREEN;
                } else {
                    testoStagione = "Siamo in primavera...";
                    coloreStagione = LIME;
                }
            }
            // Estate (Dal 21 Giugno al 20 Settembre)
            else if ((mese == 6 && giorno >= 21) || mese == 7 || mese == 8 || (mese == 9 && giorno <= 20)) {
                if (mese == 6 && giorno == 21) {
                    testoStagione = "Cambio stagione: solstizio d'estate!";
                    coloreStagione = GOLD;
                } else {
                    testoStagione = "Siamo in estate...";
                    coloreStagione = ORANGE;
                }
            }
            // Autunno (Dal 21 Settembre al 20 Dicembre)
            else if ((mese == 9 && giorno >= 21) || mese == 10 || mese == 11 || (mese == 12 && giorno <= 20)) {
                // Nota: fix per chiarezza di lettura nel blocco: mese == 12 && giorno <= 20
                if (mese == 9 && giorno == 21) {
                    testoStagione = "Cambio stagione: equinozio d'autunno!";
                    coloreStagione = ORANGE;
                } else {
                    testoStagione = "Siamo in autunno...";
                    coloreStagione = GOLD;
                }
            }
            // Inverno (Dal 21 Dicembre al 20 Marzo)
            else {
                if (mese == 12 && giorno == 21) {
                    testoStagione = "Cambio stagione: solstizio d'inverno!";
                    coloreStagione = SKYBLUE;
                } else {
                    testoStagione = "Siamo in inverno...";
                    coloreStagione = BLUE;
                }
            }


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
	BeginShaderMode(dayNightShader);
       DrawTexture(worldMap, 0, 0, WHITE);
    EndShaderMode();

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

	            DrawLine(0, yCancro, mapCoord.width, yCancro, Fade(DARKGRAY, 0.5f));
	            DrawLine(0, yEquatore, mapCoord.width, yEquatore, Fade(GRAY, 0.4f));
	            DrawLine(0, yCapricorno, mapCoord.width, yCapricorno, Fade(DARKGRAY, 0.5f));

        		// 3. Disegno dell'Eclittica (Percorso annuale del sole)
            	DrawLineStrip(eclipticPath, PATH_RESOLUTION, Fade(RED, 0.6f));

            	// 4. Indicatore del Sole Corrente
            	DrawCircleV(sunPixelPos, 10.0f, Fade(GOLD, 0.5f)); 
            	DrawCircleV(sunPixelPos, 4.0f, YELLOW);

					// cursore mouse personalizzato
				DrawRectangleRec((Rectangle) { mousePos.x - 20, mousePos.y, 40, 1 }, RED);
				DrawRectangleRec((Rectangle) { mousePos.x, mousePos.y - 20, 1, 40 }, RED);
    EndScissorMode();

    		// real time sun movement //TODO: impact with interactive keys
            // simulatedTime = time(NULL);
            // timeChanged = true;

    //info bar
    BeginScissorMode(infoCoord.x, infoCoord.y, infoCoord.width, infoCoord.height);

		            // posizione sole giorno/notte
        			float currentLonDeg = currentSun.lon * (180.0f / PI);
        			float currentLatDeg = currentSun.lat * (180.0f / PI);

					// Draw mouse position
					int x = GetMouseX()+1;
					int y = GetMouseY()+1;

					if (y > mapCoord.height) y=mapCoord.height;

					//DrawText(TextFormat("[x%i,y%i]", x, y), infoCoord.x + 6, infoCoord.y+6,20,GRAY);

					if (y < (mapCoord.height/2) ) {
						latitude = (int) 90-(y/3);
						TextCopy(latCardinal,"Nord");
					} else {
						latitude = (int) (y/3) -90;
						TextCopy(latCardinal,"Sud");
					}

					if (x > (mapCoord.width/2)) {
						longitude = (int) (x/3)-180;
						TextCopy(lonCardinal,"Est");
						} else {
						longitude  = (int) 180-(x/3);
						TextCopy(lonCardinal,"Ovest");
					}

					if (x==(mapCoord.width/2)) TextCopy(lonCardinal,"");
					if (y==(mapCoord.height/2)) TextCopy(latCardinal,"");
					// currentDate
					DrawText(TextFormat("Data: %s (Local time)", dateBuffer), infoCoord.x + 10, infoCoord.y+10, 10, WHITE);
					// cursor position
					snprintf(tempBuffer, sizeof(tempBuffer), "Pos. cursore -> Lon: %i° %s | Lat: %i° %s", longitude, lonCardinal, latitude, latCardinal);
					DrawText(TextFormat("%s", tempBuffer), infoCoord.x + 250, infoCoord.y+10,10,GRAY);
					// sun position
					DrawText(TextFormat("Pos. sole -> Lon: %.2f° | Lat: %.2f°", currentLonDeg, currentLatDeg), infoCoord.x+550,infoCoord.y+10,10, WHITE);
                    // Disegna il testo della stagione sulla UI di raylib
            		DrawText(testoStagione, infoCoord.x +800, infoCoord.y+10, 10, coloreStagione);
    EndScissorMode();

		//statusbar with some info
		//DrawText(TextFormat("%s", TOOL_SHORT_NAME), 18, screenHeight-18, 10, secondaryColor); 
		DrawText(TextFormat("v%s", TOOL_VERSION), 1040,infoCoord.y +10, 10, borderColor); 
		//DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);


	EndDrawing();
}
	

	// unload fonts
	UnloadFont(defaultFnt);
  UnloadRenderTexture(target);
	CloseWindow();
	return 0;
}
