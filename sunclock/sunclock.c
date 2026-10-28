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
#define TOOL_VERSION            "1.2.2"
#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <raymath.h>
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

// some custom colorsq
const Color bgColor = CLITERAL(Color){10, 20, 30, 255};


const int bottomOffset = 32;
Rectangle worldmap = {0,0,1080,540};
Rectangle infoBar = {0,540,1080,32};

int latitude;
int longitude;

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

Vector2 MapGeoToPixel(float lon, float lat, int mapWidth, int mapHeight) {
    Vector2 pixel;
    pixel.x = ((lon + PI) / (2.0f * PI)) * mapWidth;
    pixel.y = ((PI / 2.0f - lat) / PI) * mapHeight;
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

    // calcolo della longitudine solare
    float utcHours = utcTime->tm_hour + (utcTime->tm_min / 60.0f) + (utcTime->tm_sec / 3600.0f);
    float lonDeg = (12.0f - utcHours) * 15.0f;
    pos.lon = RAD(lonDeg);

    // calcolo della latitudine solare (declinazione)
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
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window
    SetTargetFPS(60);

	// Set UI style
	// Custom GUI font loading
	Font defaultFnt= LoadFontEx("fonts/PixelOperator.ttf", 24, NULL, 0); 

		// Create framebuffer
		RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
		SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

        // Caricamento delle due texture aggiornate alla nuova risoluzione
        Texture2D worldMap = LoadTexture("assets/map.png"); 
        Texture2D nightLights = LoadTexture("assets/nightmap.png"); 

        //load sun icon
        Texture2D sunIcon= LoadTexture("assets/sunIcon.png");          // Image converted to texture, GPU memory (VRAM)

	    Shader dayNightShader = LoadShader(0, "daynight.fs");
	    int sunPosLoc = GetShaderLocation(dayNightShader, "sunPosGeo");
        int nightLightsLoc = GetShaderLocation(dayNightShader, "nightLightsTex");

	    // Inizializziamo il tempo simulato con l'orario reale del sistema
	    time_t simulatedTime = time(NULL);
	    SunPosition currentSun = CalculateSunPositionFromTime(simulatedTime);

	    Vector2 eclipticPath[PATH_RESOLUTION];
	    bool updatePath = true;

	    // Calcoliamo una volta sola le coordinate Y fisse per i Tropici e l'Equatore
    	float yCancro = MapGeoToPixel(0.0f, RAD(23.44f), worldmap.width, worldmap.height).y;
    	float yEquatore = MapGeoToPixel(0.0f, RAD(0.0f), worldmap.width, worldmap.height).y;
    	float yCapricorno = MapGeoToPixel(0.0f, RAD(-23.44f), worldmap.width, worldmap.height).y;


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
        if (CheckCollisionPointRec(mousePos,worldmap)) HideCursor(); 
        else ShowCursor();

        // gestione cambio data in real time
        bool timeChanged = false;
        simulatedTime += (time_t)(GetFrameTime());


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

                    eclipticPath[i] = MapGeoToPixel(pathLon, pathLat, worldmap.width, worldmap.height);
                }
            }
            updatePath = false;
        }

        // Passa i dati aggiornati alla GPU
        Vector2 sunPosShaderValue = { currentSun.lon, currentSun.lat };
        SetShaderValue(dayNightShader, sunPosLoc, &sunPosShaderValue, SHADER_UNIFORM_VEC2);

        struct tm *localSimTime = localtime(&simulatedTime);

        Vector2 sunPixelPos = MapGeoToPixel(currentSun.lon, currentSun.lat, worldmap.width, worldmap.height);

            // momenti dell'anno
            int mese = localSimTime->tm_mon + 1; // tm_mon va da 0 a 11
            int giorno = localSimTime->tm_mday;
            
            const char* testoStagione = "";


            // Primavera (Dal 21 Marzo al 20 Giugno)
            if ((mese == 3 && giorno >= 21) || mese == 4 || mese == 5 || (mese == 6 && giorno <= 20)) {
                if (mese == 3 && giorno == 21) testoStagione = "Cambio stagione: equinozio di primavera!";
                else testoStagione = "Siamo in primavera...";
            }

            // Estate (Dal 21 Giugno al 20 Settembre)
            else if ((mese == 6 && giorno >= 21) || mese == 7 || mese == 8 || (mese == 9 && giorno <= 20)) {
                if (mese == 6 && giorno == 21) testoStagione = "Cambio stagione: solstizio d'estate!";
                else testoStagione = "Siamo in estate...";
            }

            // Autunno (Dal 21 Settembre al 20 Dicembre)
            else if ((mese == 9 && giorno >= 21) || mese == 10 || mese == 11 || (mese == 12 && giorno <= 20)) {
                // Nota: fix per chiarezza di lettura nel blocco: mese == 12 && giorno <= 20
                if (mese == 9 && giorno == 21) testoStagione = "Cambio stagione: equinozio d'autunno!";
                else testoStagione = "Siamo in autunno...";
            }

            // Inverno (Dal 21 Dicembre al 20 Marzo)
            else {
                if (mese == 12 && giorno == 21) testoStagione = "Cambio stagione: solstizio d'inverno!";
                else testoStagione = "Siamo in inverno...";
            }

//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------
	BeginTextureMode(target);
		ClearBackground(BLANK);
	EndTextureMode();

	BeginDrawing();
		ClearBackground (BLANK);

	   //drawRectangleRounded(0,0,screenWidth,screenHeight,0.056f,bgColor);
	
    	//load player background image
    	BeginShaderMode(dayNightShader);
                // Assegna la texture delle luci allo slot impostato nello shader ed esegui il draw
                SetShaderValueTexture(dayNightShader, nightLightsLoc, nightLights);
                DrawTexture(worldMap, 0, 0, WHITE);
        EndShaderMode();

            
        // disegna reticolo geograafico
    BeginScissorMode(worldmap.x, worldmap.y, worldmap.width, worldmap.height);

				//Disegna paralleli emisfero nord con indicasti gradi 0° /90°
				for (int i = 90; i > 0; i-=15) {
					DrawLineDashed((Vector2){worldmap.x,i*stepParallel},(Vector2){worldmap.width,i*stepParallel},2,2,DARKGRAY);
					DrawText(TextFormat("%d°",90-i),worldmap.x+4 ,(i*stepParallel)-15,10,GRAY);
					DrawText(TextFormat("%d°",90-i),worldmap.width-20,(i*stepParallel)-15,10,GRAY);
				}

				//Disegna paralleli emisfero sud con indicasti gradi 0° / -90°
				for (int i = 90; i < 180; i+=15) {
					DrawLineDashed((Vector2){worldmap.x,i*stepParallel},(Vector2){worldmap.width,i*stepParallel},2,2,DARKGRAY);
					if (i != 90) {
					DrawText(TextFormat("%d°",90-i),worldmap.x+5 ,(i*stepParallel)-15,10,GRAY);
					DrawText(TextFormat("%d°",90-i),worldmap.width-25,(i*stepParallel)-15,10,GRAY);
					}
				}

				// disegna meridiani Ovest 0 / -180°
				for (int i = 180; i > 0; i-=15) {
					DrawLineDashed((Vector2){i*stepMeridian, worldmap.y},(Vector2){i*stepMeridian ,worldmap.height},2,2,DARKGRAY);
					if (i != 180) {
					DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,worldmap.y+5,10,BLACK);
					DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,worldmap.height-15,10,BLACK);
					}
				}

				// disegna meridiani Est 0° / 180°
				for (int i = 180; i < 360; i+=15) {
					DrawLineDashed((Vector2){i*stepMeridian, worldmap.y},(Vector2){i*stepMeridian ,worldmap.height},2,2,DARKGRAY);
					DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,worldmap.y+5,10,BLACK);
					DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+5,worldmap.height-15,10,BLACK);
				}

				// Draw GreenWich meridian
				DrawLine(worldmap.width/2,worldmap.y,worldmap.width/2,worldmap.height ,LIGHTGRAY);

                // tropici ed equatore
	            DrawLine(0, yCancro, worldmap.width, yCancro, BLACK);
	            DrawLine(0, yEquatore, worldmap.width, yEquatore, LIGHTGRAY);
	            DrawLine(0, yCapricorno, worldmap.width, yCapricorno, BLACK);
                // Testo etichette linee (posizionate sul lato destro dello schermo)
                DrawText("Tropico del Cancro (+23.44°)", screenWidth - 200, yCancro - 12, 10, Fade(RAYWHITE, 0.6f));
                DrawText("Equatore (0.00°)", screenWidth - 140, yEquatore - 12, 10, Fade(RAYWHITE, 0.5f));
                DrawText("Tropico del Capricorno (-23.44°)", screenWidth - 220, yCapricorno + 4, 10, Fade(RAYWHITE, 0.6f));


        		//  traiettoria annuale del sole
            	DrawLineStrip(eclipticPath, PATH_RESOLUTION, RED);

            	// "disegno" del sole
            	//DrawCircleGradient(sunPixelPos, 10.0f, ORANGE, YELLOW); 
                DrawTextureV(sunIcon,Vector2AddValue(sunPixelPos,-16.0f), WHITE);

					// cursore mouse personalizzato
				DrawRectangleRec((Rectangle) { mousePos.x - 20, mousePos.y, 40, 1 }, RED);
				DrawRectangleRec((Rectangle) { mousePos.x, mousePos.y - 20, 1, 40 }, RED);
    EndScissorMode();



    //info bar
    BeginScissorMode(infoBar.x, infoBar.y, infoBar.width, infoBar.height);

		            // posizione sole giorno/notte
        			float currentLonDeg = currentSun.lon * (180.0f / PI);
        			float currentLatDeg = currentSun.lat * (180.0f / PI);

					// Draw mouse position
					int x = GetMouseX()+1;
					int y = GetMouseY()+1;
                    const char* latCardinal = "";
                    const char* lonCardinal = "";

					if (y > worldmap.height) y=worldmap.height;

					if (y < (worldmap.height/2) ) {
						latitude = (int) 90-(y/3);
						latCardinal = "Nord";
					} else {
						latitude = (int) (y/3) -90;
						latCardinal = "Sud";
					}

					if (x > (worldmap.width/2)) {
						longitude = (int) (x/3)-180;
						lonCardinal = "Est";
						} else {
						longitude  = (int) 180-(x/3);
						lonCardinal = "Ovest";
					}

					if (x==(worldmap.width/2)) latCardinal = "";
					if (y==(worldmap.height/2)) lonCardinal = "";
					// currentDate
                    if (localSimTime != NULL) {
                          DrawText(TextFormat("Data Corrente: %02d/%02d/%04d %02d:%02d:%02d", 
                                localSimTime->tm_mday, localSimTime->tm_mon + 1, localSimTime->tm_year + 1900,
                                localSimTime->tm_hour, localSimTime->tm_min, localSimTime->tm_sec), infoBar.x+8,infoBar.y+10, 10, RAYWHITE);
            } else {
                DrawText("Data Corrente: Data Sconosciuta", 10, 60, 10, RAYWHITE);
            }
            
					// cursor position
					snprintf(tempBuffer, sizeof(tempBuffer), "Pos. cursore -> Lon: %i° %s | Lat: %i° %s", longitude, lonCardinal, latitude, latCardinal);
					DrawText(TextFormat("%s", tempBuffer), infoBar.x + 210, infoBar.y+10,10,GRAY);
					// sun position
					DrawText(TextFormat("Pos. sole -> Lon: %.2f° | Lat: %.2f°", currentLonDeg, currentLatDeg), infoBar.x+460,infoBar.y+10,10, WHITE);
                    // Disegna il testo della stagione sulla UI di raylib
            		DrawText(testoStagione, infoBar.x +660, infoBar.y+10, 10, GRAY);
    EndScissorMode();

		//statusbar with some info
		DrawText(TextFormat("%s", TOOL_SHORT_NAME), 990, infoBar.y + 10, 10, RAYWHITE); 
		DrawText(TextFormat("v%s", TOOL_VERSION), 1040,infoBar.y +10, 10, GRAY); 
		//DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);


	EndDrawing();
}
	

	// unload fonts
	UnloadFont(defaultFnt);
    UnloadRenderTexture(target);
    UnloadTexture(worldMap);
    UnloadTexture(sunIcon);

    UnloadTexture(nightLights);
    UnloadShader(dayNightShader);
	CloseWindow();
	return 0;
}
