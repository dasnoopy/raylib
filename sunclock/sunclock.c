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
#define TOOL_VERSION            "1.6.5"

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
bool isDay = false;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD(deg) ((deg) * M_PI / 180.0f)
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
    {"Verona",          45.4383,   10.9916, RED},
    {"Roma",            41.9028,   12.4964, ORANGE},
    {"Londra",          51.5074,   -0.1278, BLUE},
    {"New York",        40.7128,  -74.0060, PINK},
    {"Tokyo",           35.6762,  139.6503, YELLOW},
    {"Sydney",         -33.8688,  151.2093, GREEN},
    {"Il Cairo",        30.0444,   31.2357, LIME},
    {"San Paolo",      -23.5505,  -46.6333, MAROON},
    {"Citta' del Capo", -33.9249,   18.4241, WHITE},
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
    {"Citta' del Messico", 19.4326, -99.1332, PURPLE},
    {"Istanbul",        41.0082,   28.9784, BROWN},
    {"Seoul",           37.5665,  126.9780, VIOLET},
    {"Rio de Janeiro", -22.9068,  -43.1729, GREEN},
    {"Singapore",        1.3521,  103.8198, DARKBLUE},
    {"Bangkok",         13.7563,  100.5018, ORANGE},
    {"Dubai",           25.2048,   55.2708, GOLD},
    {"Los Angeles",     34.0522, -118.2437, YELLOW},
    {"Chicago",         41.8781,  -87.6298, DARKGRAY},
    {"Lima",           -12.0464,  -77.0428, BEIGE},

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

// offset status bar
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

// Struttura per gestire lo stato della ripetizione dei tasti
typedef struct {
    int currentKey;       // Il tasto attualmente premuto/tracciato
    float timer;         // Timer per calcolare i delay
    bool isRepeating;    // true se ha superato il delay iniziale
    float firstDelay;    // Tempo di attesa iniziale (es. 0.4 secondi)
    float repeatRate;    // Intervallo di ripetizione (es. 0.05 secondi)
} KeyRepeater;

typedef struct {
    float latitude;         // Da -90 a +90
    float longitude;        // Da -180 a +180
    int phaseFrameIndex;    // Indice da 0 a 7 per lo spritesheet
    const char* phaseName;  // Nome testuale della fase
} MoonData;

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


// Ritorna true se è giorno, false se è notte
bool IsDaytimeAtCoordinates(float cursorLon, float cursorLat, float sunLon, float sunLat) {
    // 1. Convertiamo tutte le coordinate in radianti per le funzioni di <math.h>
    float phi = cursorLat * DEG2RAD;      // Latitudine del cursore
    float delta = sunLat * DEG2RAD;       // Declinazione solare (Latitudine del Sole)
    
    // 2. Calcoliamo l'Angolo Orario (H)
    // È la differenza di longitudine tra il cursore e il sub-solar point.
    float H = (cursorLon - sunLon) * DEG2RAD;

    // 3. Applichiamo la formula della trigonometria sferica
    float sin_h = sinf(phi) * sinf(delta) + cosf(phi) * cosf(delta) * cosf(H);

    // 4. Se sin_h è maggiore o uguale a 0, il Sole è sopra l'orizzonte (Giorno)
    return (sin_h >= 0.0f);
}


double NormalizeDegrees(double angle) {
// fmod calcola il resto della divisione tra double (equivalente a % per gli interi)
    angle = fmod(angle, 360.0);
    if (angle < 0) angle += 360.0;
    return angle;
}

// Funzione che calcola il Giorno Giuliano a partire da una data/ora UTC
double getJulianDate(int year, int month, int day, int hour, int min, int sec) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    
    int A = year / 100;
    int B = 2 - A + (A / 4);
    
    long E = (long)(365.25 * (year + 4716));
    long F = (long)(30.6001 * (month + 1));
    
    // Calcola la frazione del giorno espressa in ore, minuti e secondi
    double day_fraction = day + (hour + min / 60.0 + sec / 3600.0) / 24.0;
    
    double jd = B + day_fraction + E + F - 1524.5;
    return jd;
}

// Funzione helper per ottenere il giorno giuliano dato un custom time
double getJulianDateTime(time_t customTime) {
    struct tm *utc_time = gmtime(&customTime); // gmtime restituisce il tempo UTC, richiesto in astronomia

    return getJulianDate(
        utc_time->tm_year + 1900, // tm_year conta gli anni dal 1900
        utc_time->tm_mon + 1,     // tm_mon va da 0 a 11
        utc_time->tm_mday,
        utc_time->tm_hour,
        utc_time->tm_min,
        utc_time->tm_sec
    );
}

double getIlluminatedFractionOfMoon(double jd) {
    const double toRad = M_PI / 180.0;
    const double T = (jd - 2451545.0) / 36525.0;

    const double D = NormalizeDegrees(297.8501921 + 445267.1114034 * T - 0.0018819 * T * T + 1.0 / 545868.0 * T * T * T - 1.0 / 113065000.0 * T * T * T * T) * toRad;
    const double M = NormalizeDegrees(357.5291092 + 35999.0502909 * T - 0.0001536 * T * T + 1.0 / 24490000.0 * T * T * T) * toRad;
    const double Mp = NormalizeDegrees(134.9633964 + 477198.8675055 * T + 0.0087414 * T * T + 1.0 / 69699.0 * T * T * T - 1.0 / 14712000.0 * T * T * T * T) * toRad;

    const double i = NormalizeDegrees(180.0 - D * 180.0 / M_PI - 6.289 * sin(Mp) + 2.1 * sin(M) - 1.274 * sin(2.0 * D - Mp) - 0.658 * sin(2.0 * D) - 0.214 * sin(2.0 * Mp) - 0.11 * sin(D)) * toRad;

    const double k = (1.0 + cos(i)) / 2.0;
    return k;
}

MoonData CalculateMoonData(time_t utc_time) {
    // ==========================================================
    // calcolo delle coordinate (lat / lon) sulla mappa
    // ==========================================================

    struct tm *utc = gmtime(&utc_time);
    // 1. Calcolo del giorno giuliano abbreviato rispetto a J2000.0 (1 Gennaio 2000, 12:00 UTC)
    int year = utc->tm_year + 1900;
    int month = utc->tm_mon + 1;
    int day = utc->tm_mday;
    double hour = utc->tm_hour + utc->tm_min / 60.0 + utc->tm_sec / 3600.0;

    if (month <= 2) {
        year--;
        month += 12;
    }
    
    int A = year / 100;
    int B = A / 4;
    int C = 2 - A + B;
    int E = (int)(365.25 * (year + 4716));
    int F = (int)(30.6001 * (month + 1));
    double JD = C + day + hour / 24.0 + E + F - 1524.5;
    double T = (JD - 2451545.0) / 36525.0; // Secoli Giuliani da J2000.0

    // 2. Elementi orbitali della Luna (in radianti)
    double L0 = (218.316 + 481267.881 * T) * PI / 180.0; // Longitudine media
    double M  = (134.963 + 477198.867 * T) * PI / 180.0; // Anomalia media della Luna
    double F_arg = (93.272 + 483202.018 * T) * PI / 180.0; // Argomento della latitudine
    double D  = (297.850 + 445267.111 * T) * PI / 180.0; // Elongazione media

    // 3. Longitudine e Latitudine Eclittica della Luna (Principali perturbazioni)
    double eclipticLong = L0 + (6.289 * sin(M) + 1.274 * sin(2*D - M) + 0.658 * sin(2*D) + 0.214 * sin(2*M)) * PI / 180.0;
    double eclipticLat  = (5.128 * sin(F_arg) + 0.280 * sin(M + F_arg) + 0.277 * sin(M - F_arg) + 0.173 * sin(2*D - F_arg)) * PI / 180.0;
    
    // Obliquità dell'eclittica
    double epsilon = (23.439 - 0.013 * T) * PI / 180.0;

    // 4. Conversione in Coordinate Celesti Equatoriali (Ascensione Retta RA e Declinazione Dec)
    double ra = atan2(sin(eclipticLong) * cos(epsilon) - tan(eclipticLat) * sin(epsilon), cos(eclipticLong));
    double dec = asin(sin(eclipticLat) * cos(epsilon) + cos(eclipticLat) * sin(epsilon) * sin(eclipticLong));

    // 5. Calcolo del Tempo Siderale di Greenwich (GST) per orientare la rotazione terrestre
    double GST = 280.46061837 + 360.98564736629 * (JD - 2451545.0) + 0.000387933 * T * T;
    GST = fmod(GST, 360.0);
    if (GST < 0) GST += 360.0;
    double gstRad = GST * PI / 180.0;


    // ==========================================================
    // calcolo  della fase lunare (ciclo sinodico)
    // ==========================================================
    // Usiamo una Luna Nuova di riferimento nota ed estremamente precisa (6 Gennaio 2000, 18:14 UTC)
    const double base_new_moon = 947182440.0; 
    // Durata esatta in secondi del mese sinodico (29.530588853 giorni)
    const double synodic_period = 2551442.8769; 

    // Calcoliamo quanti secondi sono passati dalla luna nuova di riferimento
    double delta_time = (double)utc_time - base_new_moon;
    double age_in_seconds = fmod(delta_time, synodic_period);
    if (age_in_seconds < 0) age_in_seconds += synodic_period;

    // Convertiamo l'età in un angolo (0 - 360 gradi equivalenti)
    double phase_angle_rad = (age_in_seconds / synodic_period) * 2.0 * M_PI;

    // Mappatura sui tuoi 8 frame dello spritesheet (spostata di 22.5° per centrare i quarti)
    double phase_angle_deg = phase_angle_rad * 180.0 / M_PI;
    int frameIndex = (int)(fmod(phase_angle_deg + 22.5, 360.0) / 45.0);

    static const char* phaseNames[] = {
        "Luna nuova",
        "Luna crescente",
        "Primo quarto",
        "Gibbosa crescente",
        "Luna piena",
        "Gibbosa calante",
        "Ultimo quarto",
        "Luna calante"
    };

    MoonData result;

    // 6. Conversione finale in coordinate geografiche terrestri
    result.latitude = dec * 180.0 / PI; // Declinazione celeste = Latitudine terrestre
    
    double longitudeRad = ra - gstRad;
    result.longitude = longitudeRad * 180.0 / PI;
    result.longitude = fmod(result.longitude + 180.0, 360.0);
    if (result.longitude < 0) result.longitude += 360.0;
    result.longitude -= 180.0; // Riporta nel range [-180, 180]


    result.phaseFrameIndex = frameIndex;
    result.phaseName = phaseNames[frameIndex];

    return result;
}

Vector2 MapGeoToPixel(float Lon, float Lat, int mapWidth, int mapHeight) {
    Vector2 pixel;
    pixel.x = ((Lon + M_PI) / (2.0f * M_PI)) * mapWidth;
    pixel.y = ((M_PI / 2.0f - Lat) / M_PI) * mapHeight;
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

//
// implement KEYREPEAT & KEYRATE instead use of IsKeyDown / IsKeyPressed
// Funzione che aggiorna lo stato di  e restituisce true se l'azione deve essere eseguita
//
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


		// Create framebuffer
		RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
		SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

        // custom font
        Font timeFnt= LoadFontEx("fonts/SF-Mono-Semibold.ttf", 18, NULL, 0); 

        // Caricamento delle due texture aggiornate alla nuova risoluzione
        Texture2D dayMap = LoadTexture("assets/daymap.png"); 
        Texture2D nightMap = LoadTexture("assets/nightmap.png"); 

        //load sun icon
        Texture2D sunIcon= LoadTexture("assets/sun.png");          // Image converted to texture, GPU memory (VRAM)

        // load 8x1 sprites per le fasi lunari (256x32 oppure 128x16)
        Texture2D moonSpritesheet = LoadTexture("assets/moonphases.png"); 

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

    const int frameWidth = moonSpritesheet.width / 8; // Dovrebbe essere 16 se l'immagine è larga 128
    const int frameHeight = moonSpritesheet.height;   // Dovrebbe essere 16

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
        Vector2 mousePos = GetMousePosition();

        // Avanzamento in tempo reale del movimento sole/lunaa giorno/notte
        timeAccumulator += GetFrameTime();
        if (timeAccumulator >= 1.0f) {
            time_t sec = (time_t)timeAccumulator;
            simulatedTime += sec;
            timeAccumulator -= (float)sec;
        }

        float stepParallel = 3.0f; //3 = 1 grado x disegno griglia paralleli
        float stepMeridian = 3.0f; //3 = 1 grado x disegno griglia meridiani

        // ---------------------------------------------------------------------
        // calcolo dati luna 
        // ---------------------------------------------------------------------
        MoonData moon = CalculateMoonData(simulatedTime);
        // Calcola la posizione dei pixel sulla mappa equirettangolare dello schermo
        Vector2 moonPos = MapGeoToPixel(RAD(moon.longitude), RAD(moon.latitude),worldMap.width, worldMap.height);

        // percentuale di illuminazione della luna
        // *** BUG??? se questo pezzo di codice dopo calcoli sole tempo indietro di 1h (GMT)
        double jd_now = getJulianDateTime(simulatedTime);
        float moonLightPerc = (float)getIlluminatedFractionOfMoon(jd_now);

        // ---------------------------------------------------------------------
        // traiettoria del sole
        // ---------------------------------------------------------------------
        currentSun = CalculateSunPositionFromTime(simulatedTime);

        struct tm *baseTm = gmtime(&simulatedTime);
        if (baseTm != NULL) {
            for (int i = 0; i < PATH_RESOLUTION; i++) {
                float t = (float)i / (float)(PATH_RESOLUTION - 1);
                float pathLon = -M_PI + t * (2.0f * M_PI);
                
                float lonDeg = pathLon * (180.0f / M_PI);
                float targetUtcHours = 12.0f - (lonDeg / 15.0f);
                if (targetUtcHours < 0.0f) targetUtcHours += 24.0f;
                if (targetUtcHours >= 24.0f) targetUtcHours -= 24.0f;

                int targetDay = (int)(t * 365.0f) % 365;

                float latDeg = 23.44f * sinf(RAD(360.0f / 365.0f * (targetDay - 80)));
                float pathLat = RAD(latDeg);

                eclipticPath[i] = MapGeoToPixel(pathLon, pathLat, worldMap.width, worldMap.height);
            }
        }
        // ---------------------------------------------------------------------
        // posizione sole giorno/notte
        // ---------------------------------------------------------------------
        float currentLonDeg = currentSun.sunLon * (180.0f / M_PI);
        float currentLatDeg = currentSun.sunLat * (180.0f / M_PI);

        // Passa i dati aggiornati alla GPU
        Vector2 sunPosShaderValue = { currentSun.sunLon, currentSun.sunLat };
        SetShaderValue(dayNightShader, sunPosLoc, &sunPosShaderValue, SHADER_UNIFORM_VEC2);

        struct tm *localSimTime = localtime(&simulatedTime);
        Vector2 sunPixelPos = MapGeoToPixel(currentSun.sunLon, currentSun.sunLat, worldMap.width, worldMap.height);

        // ---------------------------------------------------------------------
        // momenti dell'anno
        // ---------------------------------------------------------------------
        int mese = localSimTime->tm_mon + 1; // tm_mon va da 0 a 11
        int giorno = localSimTime->tm_mday;
        
        const char* testoStagione = "";

        // Primavera (Dal 21 Marzo al 20 Giugno)
        if ((mese == 3 && giorno >= 21) || mese == 4 || mese == 5 || (mese == 6 && giorno <= 20)) {
            if (mese == 3 && giorno == 21) testoStagione = "Equinozio di primavera";
            else testoStagione = "Primavera.";
        }

        // Estate (Dal 21 Giugno al 20 Settembre)
        else if ((mese == 6 && giorno >= 21) || mese == 7 || mese == 8 || (mese == 9 && giorno <= 20)) {
            if (mese == 6 && giorno == 21) testoStagione = "Solstizio d'estate";
            else testoStagione = "Estate.";
        }

        // Autunno (Dal 21 Settembre al 20 Dicembre)
        else if ((mese == 9 && giorno >= 21) || mese == 10 || mese == 11 || (mese == 12 && giorno <= 20)) {
            // Nota: fix per chiarezza di lettura nel blocco: mese == 12 && giorno <= 20
            if (mese == 9 && giorno == 21) testoStagione = "Equinozio d'autunno";
            else testoStagione = "Autunno.";
        }

        // Inverno (Dal 21 Dicembre al 20 Marzo)
        else {
            if (mese == 12 && giorno == 21) testoStagione = "Solstizio d'inverno";
            else testoStagione = "Inverno.";
        }

        // serve ancora?
        if (mousePos.x < 0) mousePos.x = 0;
        if (mousePos.x > 1079) mousePos.x = 1079;
        if (mousePos.y < 0) mousePos.y = 0;
        if (mousePos.y > 539) mousePos.y = 539;

        // ---------------------------------------------------------------------
        // Controllo se il mouse si trova dentro la mappa 
        // ---------------------------------------------------------------------
        // interatttivita con tastiera
        // ---------------------------------------------------------------------
        if (CheckCollisionPointRec(mousePos, worldMap)) {
            HideCursor();
                // questo fa l'opposto della funzione MapGeoToPixel()
                // Calcoliamo la posizione del mouse sulla mappa
                float relativeX = (mousePos.x - worldMap.x) + 1.0f; // aggiunto 1.0f per scarto 1px centro
                float relativeY = (mousePos.y - worldMap.y) + 1.0f; // aggiunto 1.0f per scarto 1px centro
                // conversione lineare da pixel a gradi geografici
                currentClick.lon = (relativeX / worldMap.width) * 360.0f - 180.0f;
                currentClick.lat = 90.0f - (relativeY / worldMap.height) * 180.0f;

                // verifica se il mouse si trova sopra la zona luce/ombra
                // (uso le variabili già a disposizione...)
                isDay = IsDaytimeAtCoordinates(currentClick.lon, currentClick.lat, currentLonDeg, currentLatDeg);
                // bool isDay puo' ora essere impiegata in base alle necesità
        } else ShowCursor();

        struct tm *tSteps = localtime(&simulatedTime);

        // Gestione Input della tastiera ( destra / sinistra : avanzamento x giorno, su / giu avanzamaneto per ora)
        if (UpdateKeyRepeater(&repeater, KEY_UP, dt)) tSteps->tm_mday += 1; 
        if (UpdateKeyRepeater(&repeater, KEY_DOWN, dt)) tSteps->tm_mday -= 1;

        if (UpdateKeyRepeater(&repeater, KEY_LEFT, dt)) tSteps->tm_hour += 1;
        if (UpdateKeyRepeater(&repeater, KEY_RIGHT, dt)) tSteps->tm_hour -= 1;

        tSteps->tm_isdst = -1; // Gestisce automaticamente i cambi d'ora legali/solari locali durante il salto
        simulatedTime = mktime(tSteps);

        // resettare al tempo corrente
        if (IsKeyPressed(KEY_R)) simulatedTime = time(NULL);
        
        // mostra / nascondi citta
        if (IsKeyPressed(KEY_C)) showCities = !showCities;

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
        DrawText("Tropico del Cancro (+23.44°)", worldMap.width - 192, yCancro - 12, 10, LIGHTGRAY);
        DrawText("Equatore (0.00°)", worldMap.width - 132, yEquatore - 12, 10, LIGHTGRAY);
        DrawText("Tropico del Capricorno (-23.44°)", worldMap.width - 212, yCapricorno -12 , 10, LIGHTGRAY);

        // Draw Cities point
        if (showCities) {
            for (int i = 0; i < numCities; i++) {
                    Vector2 cityPos = MapGeoToPixel(RAD(bigCities[i].lon), RAD(bigCities[i].lat), worldMap.width, worldMap.height);
                    DrawCircleV(cityPos,4.0f,MAROON);//bigCities[i].color);
                    if (CheckCollisionPointCircle(GetMousePosition(), cityPos, 6.0f))
                        DrawTextEx(timeFnt,bigCities[i].name,cityPos,(float)timeFnt.baseSize,0,isDay?BLACK:WHITE);
            }
        }

		//  traiettoria annuale del sole
    	DrawLineStrip(eclipticPath, PATH_RESOLUTION, ORANGE);

    	// "disegno" centrato del sole con una icona personalizzata
        Vector2 sunOrigin = { sunIcon.width / 2.0f, sunIcon.height / 2.0f };
        DrawTexturePro(sunIcon, (Rectangle){ 0, 0, sunIcon.width, sunIcon.height }, (Rectangle){ sunPixelPos.x, sunPixelPos.y, 32, 32 }, // Dimensione icona
            sunOrigin, 0.0f, WHITE );


        // LUNA : --------------------------------------------------------------
        // Disegnao della luna tramite "sprites"
        // Definiamo quale rettangolo della texture originale andare a prendere
        Rectangle sourceRec = {
            .x = (float)(moon.phaseFrameIndex * frameWidth),
            .y = 0.0f,
            .width = (float)frameWidth,
            .height = (float)frameHeight
        };

        // Definiamo dove disegnarlo a schermo e a che dimensione (es: scaliamo a 32x32 per visibilità)
        float displaySize = 32.0f;
        Rectangle destRec = {
            .x = moonPos.x,
            .y = moonPos.y,
            .width = displaySize,
            .height = displaySize
        };

        // Centriamo l'origine dell'icona (cosicché screenPos coincida esattamente con il centro della luna)
        Vector2 moonOrigin = { displaySize / 2.0f, displaySize / 2.0f };

        // Disegna il pezzo di texture ritagliato
        DrawTexturePro(moonSpritesheet, sourceRec, destRec, moonOrigin, 0.0f, WHITE);


        //-------------------------------------------------------------------------------------------------------------------

		// cursore mouse personalizzato
        DrawRectangleRec((Rectangle) { mousePos.x-20, mousePos.y, 40, 1 }, isDay?BLACK:WHITE);
        DrawRectangleRec((Rectangle) { mousePos.x, mousePos.y-20    , 1, 40 }, isDay?BLACK:WHITE);
        DrawPixel(mousePos.x,mousePos.y, BLACK);

    EndScissorMode();

    BeginScissorMode(infoBar.x, infoBar.y, infoBar.width, infoBar.height);

        // // currentDate
        if (localSimTime != NULL) {
            strftime(tempBuffer, sizeof(tempBuffer), "%a, %d %b %Y | %H:%M:%S (LOC)", localSimTime);
            DrawTextEx(timeFnt,TextFormat("%s", tempBuffer),(Vector2){infoBar.x+12, infoBar.y+6},(float)timeFnt.baseSize,-1,WHITE);
        } else DrawText("Data Sconosciuta", 10, 60, 10, RAYWHITE);


		// quando muovi mouse su mappa mostra coordinate x,y e relativa LON,LAT
        DrawText(TextFormat("Cur -> Lon: %.2f° | Lat: %.2f°", currentClick.lon, currentClick.lat), infoBar.x + 290, infoBar.y+10,10,GRAY);

		// sun position e periodo anno
		DrawText(TextFormat("Sole -> Lon: %.2f° | Lat: %.2f° (%s)", currentLonDeg, currentLatDeg, testoStagione), infoBar.x+470,infoBar.y+10,10, WHITE);

            
        // info Moon
        DrawText(TextFormat("Luna -> %s %.1f%% | ", moon.phaseName, moonLightPerc * 100.f), infoBar.x +760, infoBar.y +10, 10, GRAY);
        // DrawText(TextFormat("Luna -> Lon: %.2f° | Lat: %.2f° (%s)", moon.longitude , moon.latitude, testoStagione), infoBar.x+470,infoBar.y+10,10, WHITE);
        
        DrawText(TextFormat("%.2f° | %.2f°", moon.longitude, moon.latitude), infoBar.x +930, infoBar.y +10, 10, GREEN);
        //DrawText(TextFormat("%.2f%c | ", fabs(moon.longitude), moon.longitude >= 0 ? 'E' : 'W'), infoBar.x +930, infoBar.y +10, 10, GREEN);
        //DrawText(TextFormat("%.2f%c", fabs(moon.latitude), moon.latitude >= 0 ? 'N' : 'S'), infoBar.x +980, infoBar.y +10, 10, GREEN);




		//statusbar with some info
		//DrawText(TextFormat("%s", TOOL_SHORT_NAME), 990, infoBar.y + 10, 10, RAYWHITE); 
		DrawText(TextFormat("v%s", TOOL_VERSION), 1040,infoBar.y +10, 10, GRAY); 
		//DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);
    EndScissorMode();

	EndDrawing();
}

    UnloadFont(timeFnt);
    UnloadTexture(moonSpritesheet);
    UnloadRenderTexture(target);
    UnloadTexture(dayMap);
    UnloadTexture(sunIcon);
    UnloadTexture(nightMap);
    UnloadShader(dayNightShader);
	CloseWindow();
	return 0;
}
