/*******************************************************************************
*
*   SUNCLOCK reborn : a raylib simple sunlight worldMap aand gmt/locale time info
*   Small utility to play mp3 files based on raylib version 6.0
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************/
		
#define TOOL_NAME               "Sunlight World Map"
#define TOOL_SHORT_NAME         "sunclock"
#define TOOL_COMMENT            "Simple but modern sun day/night world map"
#define TOOL_VERSION            "1.4.0"

#include <stdio.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <raymath.h>

// gcc -Wall -Werror simplayer.c  -o simplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

// window initial size
const int screenWidth = 1080;
const int screenHeight = 572;
#define PI 3.14159265358979323846f
#define RAD(deg) ((deg) * PI / 180.0f)
#define SECONDS_PER_DAY 86400 // Secondi in un giorno (24 ore)
#define SECONDS_PER_HOUR 3600 // Secondi in un giorno (24 ore)
#define PATH_RESOLUTION 360

// list of interesting places

// Definizione della struttura per una città
typedef struct {
    const char *name;
    float lat;
    float lon;
    Color color;
} Cities;

const Cities bigCities[] = {
    // --- CITTÀ DELLA TUA LISTA ORIGINALE (Extra-europee ed Europee iniziali) ---
    {"Verona",          45.4383,   10.9916, RED},
    {"Roma",            41.9028,   12.4964, ORANGE},
    {"Londra",          51.5074,   -0.1278, BLUE},
    {"New York",        40.7128,  -74.0060, PINK},
    {"Tokyo",           35.6762,  139.6503, YELLOW},
    {"Sydney",         -33.8688,  151.2093, GREEN},
    {"Il Cairo",        30.0444,   31.2357, LIME},
    {"San Paolo",      -23.5505,  -46.6333, MAROON},
    {"Città del Capo", -33.9249,   18.4241, WHITE},
    {"Nuova Delhi",     28.6139,   77.2090, RAYWHITE},
    {"Reykjavik",       64.1466,  -21.9426, VIOLET},
    {"Parigi",          48.8566,    2.3522, SKYBLUE},
    {"Berlino",         52.5200,   13.4050, GOLD},
    {"Madrid",          40.4168,   -3.7038, ORANGE},
    {"Mosca",           55.7558,   37.6173, RED},
    {"Pechino",         39.9042,  116.4074, DARKBROWN},
    {"Mumbai",          19.0760,   72.8777, LIME},
    {"Buenos Aires",   -34.6037,  -58.3816, SKYBLUE},
    {"Lagos",            6.5244,    3.3792, DARKGREEN},
    {"Toronto",         43.6532,  -79.3832, PINK},
    {"Città del Messico", 19.4326, -99.1332, PURPLE},
    {"Istanbul",        41.0082,   28.9784, BROWN},
    {"Seoul",           37.5665,  126.9780, VIOLET},
    {"Rio de Janeiro", -22.9068,  -43.1729, GREEN},
    {"Singapore",        1.3521,  103.8198, DARKBLUE},
    {"Bangkok",         13.7563,  100.5018, ORANGE},
    {"Dubai",           25.2048,   55.2708, GOLD},
    {"Los Angeles",     34.0522, -118.2437, YELLOW},
    {"Chicago",         41.8781,  -87.6298, DARKGRAY},
    {"Lima",           -12.0464,  -77.0428, BEIGE},

    // --- NUOVE INTEGRAZIONI ESCLUSIVAMENTE EXTRA-EUROPEE ---
    // Nord America
    {"San Francisco",   37.7749, -122.4194, SKYBLUE},
    {"Miami",           25.7617,  -80.1918, MAGENTA},
    {"Vancouver",       49.2827, -123.1207, SKYBLUE},
    {"Montreal",        45.5017,  -73.5673, LIGHTGRAY},
    {"Havana",          23.1136,  -82.3666, ORANGE},
    
    // Sud America
    {"Santiago",       -33.4489,  -70.6693, GREEN},
    {"Bogotà",           4.7110,  -74.0721, DARKBLUE},
    {"Lima",           -12.0464,  -77.0428, BEIGE},
    {"Caracas",         10.4806,  -66.9036, RED},
    
    // Africa
    {"Nairobi",         -1.2921,   36.8219, DARKGREEN},
    {"Johannesburg",   -26.2041,   28.0473, GOLD},
    {"Casablanca",      33.5731,   -7.5898, BEIGE},
    {"Addis Abeba",      9.0192,   38.7468, YELLOW},
    {"Dakar",           14.7167,  -17.4677, PURPLE},
    
    // Asia & Medio Oriente
    {"Shanghai",        31.2304,  121.4737, RED},
    {"Hong Kong",       22.3193,  114.1694, MAROON},
    {"Giacarta",        -6.2088,  106.8456, LIME},
    {"Manila",          14.5995,  120.9842, DARKGRAY},
    {"Osaka",           34.6937,  135.5023, PINK},
    {"Taipei",          25.0330,  121.5654, VIOLET},
    {"Teheran",         35.6892,   51.3890, BROWN},
    {"Gerusalemme",     31.7683,   35.2137, RAYWHITE},
    {"Riad",            24.7136,   46.6753, DARKGREEN},
    {"Baghdad",         33.3152,   44.3661, DARKBROWN},
    
    // Oceania
    {"Melbourne",      -37.8136,  144.9631, BLUE},
    {"Auckland",       -36.8485,  174.7633, DARKBLUE},
    {"Brisbane",       -27.4705,  153.0260, SKYBLUE},
    {"Perth",          -31.9505,  115.8605, GOLD},
    {"Wellington",     -41.2865,  174.7762, WHITE}
};


// Calcolo automatico del numero di città nell'array (C99)
const int numCities = sizeof(bigCities) / sizeof(bigCities[0]);
bool showCities = false; // non mostrare i punti delle citta : premi C per visualizzare/naascondere

// Struttura comoda per i dati geografici
typedef struct {
    float lon; // Longitudine (-180 a 180)
    float lat; // Latitudine (-90 a 90)
} GeoCoords;

typedef struct {
    float sunLon; // in radianti
    float sunLat; // in radianti
} SunPosition;

// some custom colorsq
const Color bgColor = CLITERAL(Color){10, 20, 30, 255};


const int bottomOffset = 32;
    // Definiamo i confini e la dimensione della mappa sullo schermo

Rectangle worldMap = { 
        .x = 0,
        .y = 0,
        .width = 1080,
        .height = 540
    };

Rectangle infoBar = {0,540,1080,bottomOffset};

GeoCoords currentClick = { 0.0f, 0.0f };

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

Vector2 MapGeoToPixel(float Lon, float Lat, int mapWidth, int mapHeight) {
    Vector2 pixel;
    pixel.x = ((Lon + PI) / (2.0f * PI)) * mapWidth;
    pixel.y = ((PI / 2.0f - Lat) / PI) * mapHeight;
    return pixel;
}

// Calcola la posizione del sole basandosi su un timestamp time_t arbitrario
SunPosition CalculateSunPositionFromTime(time_t customTime) {
    SunPosition pos;
    struct tm *utcTime = gmtime(&customTime);

    if (utcTime == NULL) {
        pos.sunLon = 0.0f;
        pos.sunLat = 0.0f;
        return pos;
    }

    // calcolo della longitudine solare
    float utcHours = utcTime->tm_hour + (utcTime->tm_min / 60.0f) + (utcTime->tm_sec / 3600.0f);
    float lonDeg = (12.0f - utcHours) * 15.0f;
    pos.sunLon = RAD(lonDeg);

    // calcolo della latitudine solare (declinazione)
    int dayOfYear = utcTime->tm_yday + 1;
    float latDeg = 23.44f * sinf(RAD(360.0f / 365.0f * (dayOfYear - 80)));
    pos.sunLat = RAD(latDeg);

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

    // Controllo esistenza file shader per evitare crash su macchine Linux
    if (!FileExists("daynight.fs")) {
        TraceLog(LOG_ERROR, "ERRORE: File 'daynight.fs' non trovato nella directory corrente!");
        CloseWindow();
        return 1;
    }

    	// // Set UI style
    	// // Custom GUI font loading
    	// Font defaultFnt= LoadFontEx("fonts/PixelOperator.ttf", 24, NULL, 0); 

		// Create framebuffer
		RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
		SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

        // Caricamento delle due texture aggiornate alla nuova risoluzione
        Texture2D dayMap = LoadTexture("assets/daymap.png"); 
        Texture2D nightMap = LoadTexture("assets/nightmap.png"); 

        //load sun icon
        Texture2D sunIcon= LoadTexture("assets/sunIcon.png");          // Image converted to texture, GPU memory (VRAM)

	    Shader dayNightShader = LoadShader(0, "daynight.fs");
	    int sunPosLoc = GetShaderLocation(dayNightShader, "sunPosGeo");
        int nightLightsLoc = GetShaderLocation(dayNightShader, "nightLightsTex");

	    // Inizializziamo il tempo simulato con l'orario reale del sistema
	    time_t simulatedTime = time(NULL);
        float timeAccumulator = 0.0f; // per agg. in tempo reale
	    SunPosition currentSun = CalculateSunPositionFromTime(simulatedTime);

	    Vector2 eclipticPath[PATH_RESOLUTION];

	    // Calcoliamo una volta sola le coordinate Y fisse per i Tropici e l'Equatore
    	float yCancro = MapGeoToPixel(0.0f, RAD(23.44f), worldMap.width, worldMap.height).y;
    	float yEquatore = MapGeoToPixel(0.0f, RAD(0.0f), worldMap.width, worldMap.height).y;
    	float yCapricorno = MapGeoToPixel(0.0f, RAD(-23.44f), worldMap.width, worldMap.height).y;


    // Inizializzazione del ripetitore (400ms delay iniziale, 80ms tra le ripetizioni)
    KeyRepeater repeater = {
        .currentKey = 0,
        .timer = 0.0f,
        .isRepeating = false,
        .firstDelay = 0.40f,
        .repeatRate = 0.08f
    };

    SetTargetFPS(60);
	// fai riapparire finestra dopo averla nascosta durante il carimento iniziale
	SetWindowSize(screenWidth,screenHeight);
	SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  
	ClearWindowState(FLAG_WINDOW_HIDDEN);

while (!WindowShouldClose()) {

//----------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------

		float dt = GetFrameTime();

        // Correzione scorrimento tempo reale fluido (C99 compatibile)
        timeAccumulator += GetFrameTime();
        if (timeAccumulator >= 1.0f) {
            time_t sec = (time_t)timeAccumulator;
            simulatedTime += sec;
            timeAccumulator -= (float)sec;
        }

        Vector2 mousePos = GetMousePosition();   // abilitare se cursore libero   
        float stepParallel = 3.0f; //3 = 1 grado x disegno griglia paralleli
        float stepMeridian = 3.0f; //3 = 1 grado x disegno griglia meridiani

        //
        if (mousePos.x < 0) mousePos.x = 0;
        if (mousePos.x > 1079) mousePos.x = 1079;
        if (mousePos.y < 0) mousePos.y = 0;
        if (mousePos.y > 539) mousePos.y = 539;

        // Controllo se il mouse si trova dentro la mappa 
        if (CheckCollisionPointRec(mousePos, worldMap)) {
            HideCursor();
                // 1. Calcoliamo la posizione del mouse RELATIVA alla mappa (0..1079, 0..539)
                float relativeX = (mousePos.x - worldMap.x) + 1.0f; // aggiunto 1.0f per scarto 1px centro
                float relativeY = (mousePos.y - worldMap.y) + 1.0f; // aggiunto 1.0f per scarto 1px centro
                // 2. Trasformazione lineare in gradi geografici
                currentClick.lon = (relativeX / worldMap.width) * 360.0f - 180.0f;
                currentClick.lat = 90.0f - (relativeY / worldMap.height) * 180.0f;
        } else ShowCursor();

        struct tm *tSteps = localtime(&simulatedTime);

        // Gestione Input della tastiera ( destra / sinistra : avanzamento x giorno, su / giu avanzamaneto per ora)
        if (UpdateKeyRepeater(&repeater, KEY_UP, dt)) tSteps->tm_mday += 1; 
        if (UpdateKeyRepeater(&repeater, KEY_DOWN, dt)) tSteps->tm_mday -= 1;

        if (UpdateKeyRepeater(&repeater, KEY_LEFT, dt)) tSteps->tm_hour += 1;
        if (UpdateKeyRepeater(&repeater, KEY_RIGHT, dt)) tSteps->tm_hour -= 1;

        tSteps->tm_isdst = -1; // Gestisce automaticamente i cambi d'ora legali/solari locali durante il salto
        simulatedTime = mktime(tSteps);


        // Tasto R per resettare al tempo reale corrente
        if (IsKeyPressed(KEY_R)) simulatedTime = time(NULL);
        
        // mostra nascondi citta
        if (IsKeyPressed(KEY_C)) showCities = !showCities;

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

                    eclipticPath[i] = MapGeoToPixel(pathLon, pathLat, worldMap.width, worldMap.height);
                }
            }

        // Passa i dati aggiornati alla GPU
        Vector2 sunPosShaderValue = { currentSun.sunLon, currentSun.sunLat };
        SetShaderValue(dayNightShader, sunPosLoc, &sunPosShaderValue, SHADER_UNIFORM_VEC2);

        struct tm *localSimTime = localtime(&simulatedTime);

        Vector2 sunPixelPos = MapGeoToPixel(currentSun.sunLon, currentSun.sunLat, worldMap.width, worldMap.height);

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

        //
        // do something when  window loses focus
        //
        if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) SetWindowOpacity(0.92f);
        else SetWindowOpacity(1.0f);

//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------
	BeginTextureMode(target);
		ClearBackground(BLANK);
	EndTextureMode();

	BeginDrawing();
		ClearBackground (BLANK);

    	//load player background image
    	BeginShaderMode(dayNightShader);
                // Assegna la texture delle luci allo slot impostato nello shader ed esegui il draw
                SetShaderValueTexture(dayNightShader, nightLightsLoc, nightMap);
                DrawTexture(dayMap, worldMap.x,worldMap.y, WHITE);
        EndShaderMode();

            
    // disegna reticolo geograafico
    BeginScissorMode(worldMap.x, worldMap.y, worldMap.width, worldMap.height);

		//Disegna paralleli 
		for (int i = 0.0f; i < 180.0f; i+=15.0f) {
			DrawLineDashed((Vector2){worldMap.x,i*stepParallel},(Vector2){worldMap.width,i*stepParallel},2,2,DARKGRAY);
			DrawText(TextFormat("%.f°",90.0f-i),worldMap.x+4.0f ,(i*stepParallel)-12.0f,10,GRAY);
			DrawText(TextFormat("%.f°",90.0f-i),worldMap.width-21.0f,(i*stepParallel)-12.0f,10,GRAY);
		}

		// disegna meridiani 
		for (int i = 0; i < 360; i+=15) {
			DrawLineDashed((Vector2){i*stepMeridian, worldMap.y},(Vector2){i*stepMeridian ,worldMap.height},2,2,DARKGRAY);
			DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+2,worldMap.y+5,10,BLACK);
			DrawText(TextFormat("%d°",i-180),(i*stepMeridian)+2,worldMap.height-15,10,BLACK);
			}

		// Disegna il meridiano 0° di Greenwich
		DrawLine((worldMap.width/2),worldMap.y,(worldMap.width/2),worldMap.height ,bgColor);

        // tropici ed equatore
        DrawLine(0, yCancro, worldMap.width, yCancro, BROWN);
        DrawLine(0, yEquatore, worldMap.width, yEquatore, bgColor);
        DrawLine(0, yCapricorno, worldMap.width, yCapricorno, BROWN);
        // etichett
        DrawText("Tropico del Cancro (+23.44°)", worldMap.x + 18, yCancro-12, 10, Fade(RAYWHITE, 0.6f));
        DrawText("Equatore (0.00°)", worldMap.x + 18, yEquatore-12, 10, Fade(RAYWHITE, 0.5f));
        DrawText("Tropico del Capricorno (-23.44°)", worldMap.x + 18, yCapricorno - 12, 10, Fade(RAYWHITE, 0.6f));


		//  traiettoria annuale del sole
    	DrawLineStrip(eclipticPath, PATH_RESOLUTION, MAROON);

    	// "disegno" del sole con una icona personalizzata
        DrawTextureV(sunIcon,Vector2AddValue(sunPixelPos,-17), ORANGE);

        // Draw Cities point
        if (showCities) {
            for (int i = 0; i < numCities; i++) {
                    Vector2 cityPos = MapGeoToPixel(RAD(bigCities[i].lon), RAD(bigCities[i].lat), worldMap.width, worldMap.height);
                    DrawCircleV(cityPos,4.0f,bigCities[i].color);
                    if ( CheckCollisionPointCircle(GetMousePosition(), cityPos, 4.0f) ) DrawText(bigCities[i].name,cityPos.x,cityPos.y,20,RED);
            }
        }

		// cursore mouse personalizzato
        DrawRectangleRec((Rectangle) { mousePos.x-20, mousePos.y, 40, 1 }, WHITE);
        DrawRectangleRec((Rectangle) { mousePos.x, mousePos.y-20    , 1, 40 }, WHITE);
        DrawPixel(mousePos.x,mousePos.y, BLACK);
    EndScissorMode();




     //info bar
    BeginScissorMode(infoBar.x, infoBar.y, infoBar.width, infoBar.height);

                // posizione sole giorno/notte
    			float currentLonDeg = currentSun.sunLon * (180.0f / PI);
    			float currentLatDeg = currentSun.sunLat * (180.0f / PI);

    			// Se l'utente ha cliccato, mostra un punto e le coordinate
                DrawText(TextFormat("Cur -> x: %.f | y:%.f | Lon: %.2f° | Lat: %.2f°", mousePos.x, mousePos.y, currentClick.lon, currentClick.lat), infoBar.x + 190, infoBar.y+10,10,GRAY);

    			// currentDate
                if (localSimTime != NULL) {
                      DrawText(TextFormat("Data %02d/%02d/%04d %02d:%02d:%02d (LOC)", 
                            localSimTime->tm_mday, localSimTime->tm_mon + 1, localSimTime->tm_year + 1900,
                            localSimTime->tm_hour, localSimTime->tm_min, localSimTime->tm_sec), infoBar.x+8,infoBar.y+10, 10, RAYWHITE);
                } else DrawText("Data Sconosciuta", 10, 60, 10, RAYWHITE);

    			// sun position
    			DrawText(TextFormat("Pos. sole -> Lon: %.2f° | Lat: %.2f°", currentLonDeg, currentLatDeg), infoBar.x+460,infoBar.y+10,10, WHITE);
                
                // Disegna il testo della stagione sulla UI di raylib
                DrawText(testoStagione, infoBar.x +660, infoBar.y+10, 10, GRAY);

		//statusbar with some info
		DrawText(TextFormat("%s", TOOL_SHORT_NAME), 990, infoBar.y + 10, 10, RAYWHITE); 
		DrawText(TextFormat("v%s", TOOL_VERSION), 1040,infoBar.y +10, 10, GRAY); 
		//DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);
    EndScissorMode();

	EndDrawing();
}
	

	// unload fonts
	// UnloadFont(defaultFnt);
    UnloadRenderTexture(target);
    UnloadTexture(dayMap);
    UnloadTexture(sunIcon);
    UnloadTexture(nightMap);
    UnloadShader(dayNightShader);
	CloseWindow();
	return 0;
}
