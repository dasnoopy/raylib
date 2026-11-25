/*******************************************************************************************
*
*   FIT VIEWER
*   A simple .fit decoder/viewer with map and track drawing on a map, made in C99 using raylib v6.0
*
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************/

#define TOOL_NAME               "FIT FILE VIEWER & ANALYZER"
#define TOOL_SHORT_NAME         "fitview"
#define TOOL_VERSION            "3.0.5"

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <raylib.h>
#include <raymath.h>
#include <strings.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <curl/curl.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Garmin .FIT conversion
#include "fit_convert.h"
#define _CRT_SECURE_NO_WARNINGS
// Costante per convertire i semicerchi in gradi decimali
#define SEMICIRCLES_TO_DEGREES (180.0 / 2147483648.0)
// Epoca Garmin in formato timestamp Unix (31 Dicembre 1989 00:00:00 UTC)
#define GARMIN_TIME_OFFSET 631065600

// window size (16:9)
const int screenWidth = 1184; 
const int screenHeight = 736;

Rectangle dataRect = { 8,32,340,295};     // top/left
Rectangle mapRect = { 354,8.0,822,492};  // top/right
Rectangle filesRect = { 8,333,340,369}; // bottom/left
Rectangle chartRect = {354,506,822,196};  // bottom/right

const int fntSize = 16;
char tempBuffer[64];
bool showKM=false;
bool isDarkMode = false;

typedef struct {
    Color accentColor;
    Color fgColor;
    Color bgColor;
    Color borderColor;
    Color gridColor;
    Color gridBgColor;
} colorTheme;

static const colorTheme lightTheme = {
    .accentColor = (Color){94, 129, 172, 255}, 
    .fgColor = (Color){55, 65, 70, 255},
    .bgColor = (Color){218, 227, 227, 255},
    .borderColor = (Color){76, 86, 106, 255},
    .gridColor = (Color){180, 185, 185, 255},
    .gridBgColor = (Color){239, 246, 246, 255},
};

static const colorTheme darkTheme = {
    .accentColor = (Color){208, 135, 112, 255}, 
    .fgColor = (Color){216, 222, 233, 255},
    .bgColor = (Color){50, 55, 66, 255},
    .borderColor = (Color){116, 122, 133, 255},
    .gridColor = (Color){86, 96, 116, 255},
    .gridBgColor = (Color){61, 68, 83, 255},
};

colorTheme currentTheme;

// Key repeat structure
typedef struct {
    int currentKey;      // Il tasto attualmente premuto
    float timer;         // Timer per calcolare i delay
    bool isRepeating;    // true se ha superato il delay iniziale
    float firstDelay;    // Tempo di attesa iniziale 
    float repeatRate;    // Intervallo di ripetizione 
} KeyRepeater;

// Struttura per le coordinate geografiche di tutti i punti che registro
typedef struct {
    int record;
    char data[64];
    double lat;
    double lon;
    double alt;
    double dist;
    double vel;
} gpsPoints;

typedef struct {
    char data_inizio[64];
    char data_fine[64];
    double minTot;
    double minMov;
    double disTot;
    double avgSpd;
    double maxSpd;
    double altPos;
    double altNeg;
    double calorie;
    double avgTemp;
    double maxTemp;
    double altAvg;
    double altMax;
} trackData;

int numRecord;
int numPointRec;
Vector2 puntiPixel[32768]; // todo : convert to diynamic
gpsPoints puntiGps[32768]; // see above
trackData infoTraccia;

// per tooltip su grafico
static float chartMaxAlt = 10.0f;
static float chartMaxVel = 1.0f;
static int selectedPointIdx = -1;

// library & files management
#define MAX_FILES 2048
#define MAX_NAME 512
#define FILTER_EXT      ".fit"

// Struttura per memorizzare i metadati di ogni file senza rileggere il disco
typedef struct {
    char path[MAX_NAME];
    int year;
    int month; // 0 - 12 (0 uso speciale tutti i mesi)
    int day; // 0-6 lun dom
} fileEntry;

static fileEntry allFiles[MAX_FILES];
static int totalFilesCount = 0;

// Array che contiene gli indici dei soli file che passano il filtro
static int filteredIndices[MAX_FILES];
static int filteredCount = 0;

// variabili per gestire doppio click su filename
static int lastClickedIndex = -1;
static double lastClickTime = 0.0;
const double DOUBLE_CLICK_DELAY = 0.28; // 280 ms

char fName[MAX_NAME] = "";
int selectedIndex = 0;
int rowHeight = 21;
int visibleRows;
int maxRows = 16;
int scrollOffset = 0;     // primo file visualizzato
bool isTrackLoaded = false;
bool btnQuit = false;

// Variabili di stato per gli spinner
int selectedMonth;
int selectedYear;
bool filterEnabled = false;

// Tabella di conversione da 1-12 a nome del mese (indice 0 = Gennaio)
const char *mesi[] = {
    "Tutti i mesi", "Gennaio", "Febbraio", "Marzo", "Aprile",
    "Maggio", "Giugno", "Luglio", "Agosto",
    "Settembre", "Ottobre", "Novembre", "Dicembre"
};

// --- CONFIGURAZIONE ---
#define TILE_SIZE 256
#define MIN_ZOOM 5
#define MAX_ZOOM 17          // OpenTopoMap supporta fino a zoom 17
#define MAX_RAM_TILES 256    // Cache GPU estesa (~64 MB VRAM) per mantenere i tile padre
#define MAX_QUEUE_SIZE 64    // Coda massima richieste download
#define CACHE_DIR ".tile_cache"

// --- STRUTTURE DATI ---
typedef struct {
    int z, x, y;
    Texture2D tex;
    bool inUse;
    unsigned long lastUsedFrame;
} CachedTile;

typedef struct {
    int z, x, y;
    bool active;
} DownloadJob;

// --- STATO GLOBALE CACHE E THREADING ---
static CachedTile ramCache[MAX_RAM_TILES] = {0};
static DownloadJob downloadQueue[MAX_QUEUE_SIZE] = {0};
static int queueCount = 0;

static pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;
static bool workerRunning = true;

// --- FUNZIONI MATEMATICHE WEB MERCATOR ---
void LatLonToWorldPixel(double lat, double lon, int zoom, double *px, double *py) {
    double n = pow(2.0, zoom);
    double tileX = (lon + 180.0) / 360.0 * n;
    double latRad = lat * M_PI / 180.0;
    double tileY = (1.0 - asinh(tan(latRad)) / M_PI) / 2.0 * n;

    *px = tileX * TILE_SIZE;
    *py = tileY * TILE_SIZE;
}

// Calcola i limiti (bounding box) di un array di punti GPS
void ComputeTrackBounds(const Vector2 *points, int count, double *minLat, double *maxLat, double *minLon, double *maxLon) {
    if (count <= 0) return;
    *minLat = *maxLat = points[0].x;
    *minLon = *maxLon = points[0].y;

    for (int i = 1; i < count; i++) {
        if (points[i].x < *minLat) *minLat = points[i].x;
        if (points[i].x > *maxLat) *maxLat = points[i].x;
        if (points[i].y < *minLon) *minLon = points[i].y;
        if (points[i].y > *maxLon) *maxLon = points[i].y;
    }
}

// Calcola lo zoom massimo in cui rientra l'intera traccia e centra la telecamera
void FitTrackToScreen(double minLat, double maxLat, double minLon, double maxLon,
                      int screenW, int screenH, int paddingPx,
                      int *outZoom, double *outCamX, double *outCamY) {
    int availW = screenW - paddingPx * 2;
    int availH = screenH - paddingPx * 2;
    if (availW < 64) availW = 64;
    if (availH < 64) availH = 64;

    int bestZoom = MIN_ZOOM;

    for (int z = MAX_ZOOM; z >= MIN_ZOOM; z--) {
        double x1, y1, x2, y2;
        LatLonToWorldPixel(maxLat, minLon, z, &x1, &y1);
        LatLonToWorldPixel(minLat, maxLon, z, &x2, &y2);

        double widthPx = fabs(x2 - x1);
        double heightPx = fabs(y2 - y1);

        if (widthPx <= availW && heightPx <= availH) {
            bestZoom = z;
            break;
        }
    }

    double xMin, yMin, xMax, yMax;
    LatLonToWorldPixel(maxLat, minLon, bestZoom, &xMin, &yMin);
    LatLonToWorldPixel(minLat, maxLon, bestZoom, &xMax, &yMax);

    *outZoom = bestZoom;
    *outCamX = (xMin + xMax) / 2.0;
    *outCamY = (yMin + yMax) / 2.0;
}

// --- GESTIONE DOWNLOAD ASINCRONO (LIBCURL) ---
static size_t WriteFileCallback(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

static bool IsTileQueued(int z, int x, int y) {
    for (int i = 0; i < queueCount; i++) {
        if (downloadQueue[i].z == z && downloadQueue[i].x == x && downloadQueue[i].y == y) {
            return true;
        }
    }
    return false;
}

static void EnqueueTileDownload(int z, int x, int y) {
    pthread_mutex_lock(&queueMutex);
    if (!IsTileQueued(z, x, y) && queueCount < MAX_QUEUE_SIZE) {
        downloadQueue[queueCount++] = (DownloadJob){z, x, y, true};
        pthread_cond_signal(&queueCond);
    }
    pthread_mutex_unlock(&queueMutex);
}

static void *DownloadWorker(void *arg) {
    (void)arg;
    CURL *curl = curl_easy_init();
    if (!curl) return NULL;

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "RaylibGPSViewer/1.0 (Linux; C99)");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    const char subdomains[3] = {'a', 'b', 'c'};

    while (workerRunning) {
        pthread_mutex_lock(&queueMutex);
        while (queueCount == 0 && workerRunning) {
            pthread_cond_wait(&queueCond, &queueMutex);
        }
        if (!workerRunning) {
            pthread_mutex_unlock(&queueMutex);
            break;
        }

        DownloadJob job = downloadQueue[0];
        for (int i = 1; i < queueCount; i++) {
            downloadQueue[i - 1] = downloadQueue[i];
        }
        queueCount--;
        pthread_mutex_unlock(&queueMutex);

        char finalPath[256];
        char tempPath[256];
        snprintf(finalPath, sizeof(finalPath), "%s/otm_%d_%d_%d.png", CACHE_DIR, job.z, job.x, job.y);
        snprintf(tempPath, sizeof(tempPath), "%s/tmp_%d_%d_%d.png", CACHE_DIR, job.z, job.x, job.y);

        if (access(finalPath, F_OK) == 0) continue;

        char url[256];
        char sub = subdomains[(job.x + job.y) % 3];
        snprintf(url, sizeof(url), "https://%c.tile.opentopomap.org/%d/%d/%d.png", sub, job.z, job.x, job.y);
        //snprintf(url, sizeof(url), "https://%c.tile-cyclosm.openstreetmap.fr/cyclosm/%d/%d/%d.png", sub, job.z, job.x, job.y);


        FILE *fp = fopen(tempPath, "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            CURLcode res = curl_easy_perform(curl);

            long httpCode = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
            fclose(fp);

            if (res == CURLE_OK && httpCode == 200) {
                rename(tempPath, finalPath);
            } else {
                remove(tempPath);
            }
        }
    }

    curl_easy_cleanup(curl);
    return NULL;
}

// --- GESTIONE CACHE TEXTURE IN GPU & FALLBACK SCALING ---
static Texture2D *FindTileInRamOnly(int z, int x, int y, unsigned long currentFrame) {
    for (int i = 0; i < MAX_RAM_TILES; i++) {
        if (ramCache[i].inUse && ramCache[i].z == z && ramCache[i].x == x && ramCache[i].y == y) {
            ramCache[i].lastUsedFrame = currentFrame;
            return &ramCache[i].tex;
        }
    }
    return NULL;
}

static Texture2D *GetTileTexture(int z, int x, int y, unsigned long currentFrame) {
    // 1. Cerca nella cache RAM/GPU
    Texture2D *cached = FindTileInRamOnly(z, x, y, currentFrame);
    if (cached) return cached;

    // 2. Controlla se il file esiste su disco
    char path[256];
    snprintf(path, sizeof(path), "%s/otm_%d_%d_%d.png", CACHE_DIR, z, x, y);

    if (FileExists(path)) {
        int targetIdx = -1;
        unsigned long oldestFrame = currentFrame + 1;

        for (int i = 0; i < MAX_RAM_TILES; i++) {
            if (!ramCache[i].inUse) {
                targetIdx = i;
                break;
            }
            // Protegge i tile usati nel frame corrente o appena precedente (inclusi i tile padre)
            if (ramCache[i].lastUsedFrame < oldestFrame && (currentFrame - ramCache[i].lastUsedFrame) > 2) {
                oldestFrame = ramCache[i].lastUsedFrame;
                targetIdx = i;
            }
        }

        // Fallback se tutti gli slot sono stati usati negli ultimi 2 frame
        if (targetIdx == -1) {
            for (int i = 0; i < MAX_RAM_TILES; i++) {
                if (ramCache[i].lastUsedFrame < oldestFrame) {
                    oldestFrame = ramCache[i].lastUsedFrame;
                    targetIdx = i;
                }
            }
        }

        if (targetIdx != -1) {
            if (ramCache[targetIdx].inUse) {
                UnloadTexture(ramCache[targetIdx].tex);
            }
            Texture2D tex = LoadTexture(path);
            if (tex.id > 0) {
                SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
                ramCache[targetIdx] = (CachedTile){z, x, y, tex, true, currentFrame};
                return &ramCache[targetIdx].tex;
            }
        }
    } else {
        // 3. Accoda per il download in background
        EnqueueTileDownload(z, x, y);
    }

    return NULL;
}

// Disegna una porzione scalata di un tile antenato (zoom inferiore) mentre quello nuovo carica
static bool DrawFallbackTile(int z, int tx, int ty, float screenX, float screenY, unsigned long currentFrame) {
    for (int dz = 1; dz <= 4; dz++) {
        int parentZ = z - dz;
        if (parentZ < MIN_ZOOM) break;

        int factor = 1 << dz; // 2, 4, 8, 16
        int parentX = tx / factor;
        int parentY = ty / factor;

        Texture2D *parentTex = FindTileInRamOnly(parentZ, parentX, parentY, currentFrame);
        if (parentTex) {
            float subSize = (float)TILE_SIZE / (float)factor;
            float srcX = (tx % factor) * subSize;
            float srcY = (ty % factor) * subSize;

            Rectangle sourceRec = { srcX, srcY, subSize, subSize };
            Rectangle destRec = { screenX, screenY, (float)TILE_SIZE, (float)TILE_SIZE };

            DrawTexturePro(*parentTex, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
            return true;
        }
    }
    return false;
}


/* -----------------------------------------------------------------------------
                        Function declaration
----------------------------------------------------------------------------- */

bool UpdateKeyRepeater(KeyRepeater *repeater, int key, float deltaTime);
void ConvertFitTimestamp(FIT_UINT32 fit_timestamp, char* output_str, size_t max_size);
int decodeFit(char* fileName);
void getTimeFromMinute (double mins);
void loadLibrary (void);
int compareFiles(const void *a, const void *b);
int compareFilesByModTime(const void *a, const void *b);
int loadFilesRecursive(const char *path);
void applyDateFilter(bool enableFilter, int targetYear, int targetMonth);
void drawChart(RenderTexture2D target);
void ApplyTheme(colorTheme *currentTheme, bool darkMode); 
Color GetReadableTextColor(Color bgColor);
void drawChartTooltip(void);

int main (void)
{
	 // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED |  FLAG_MSAA_4X_HINT); //|  FLAG_MSAA_4X_HINT); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "fitview");
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // carica fonts con tutti i caratteri da 32 (spazio) a 255 (caratteri estesi europei)
    int startGlyph = 32;
    int endGlyph = 255;
    int glyphCount = endGlyph - startGlyph + 1;

    // array per i codepoints
    int *codepoints = (int *)malloc(sizeof(int) * glyphCount);
    for (int i = 0; i < glyphCount; i++) codepoints[i] = startGlyph + i;

    Font textFont = LoadFontEx("fonts/PixelOperator.ttf", fntSize,codepoints, glyphCount); // all other text
    Font boldFont = LoadFontEx("fonts/PixelOperator-Bold.ttf", fntSize,codepoints, glyphCount); // all other text
    Font titleFont = LoadFontEx("fonts/HelveticaNeue-Medium.otf", 36,codepoints, glyphCount); // all other text
    Font subTitleFont = LoadFontEx("fonts/HelveticaNeue.otf", 28,codepoints, glyphCount); // all other text

	RenderTexture targetChart = LoadRenderTexture(chartRect.width, chartRect.height);
    Texture2D gpsIcon= LoadTexture("assets/position.png");          // Image converted to texture, GPU memory (VRAM)

    // shader per cambiare i colori di default delle mappe..
    Shader mapShader = LoadShader(0, "map_theme.fs");

    mkdir(CACHE_DIR, 0755);
    curl_global_init(CURL_GLOBAL_DEFAULT);
    pthread_t workerThread;
    pthread_create(&workerThread, NULL, DownloadWorker, NULL);

    int zoom = MIN_ZOOM;
    double camWorldX = 0.0, camWorldY = 0.0;
        // Bounding box geografico (WGS84 / EPSG:4326)
    double minLon = 10.46546111; // Ovest (Left)
    double maxLon = 11.36674722; // Est (Right)
    double minLat = 45.15988611; // Sud (Bottom)
    double maxLat = 45.91895000; // Nord (Top) 
    FitTrackToScreen(minLat, maxLat, minLon, maxLon, GetScreenWidth(), GetScreenHeight(), 80, &zoom, &camWorldX, &camWorldY);
    unsigned long frameCounter = 0;


    // apply color theme
    ApplyTheme(&currentTheme, isDarkMode);

// load file list and sort by file modification time
    loadLibrary();
    qsort(allFiles, totalFilesCount, sizeof(fileEntry), compareFilesByModTime);  // ordinamento file


    // set key repeater speed (400ms initial delay, 80ms repeat rate)
    KeyRepeater repeater = {
        .currentKey = 0,
        .timer = 0.0f,
        .isRepeating = false,
        .firstDelay = 0.40f,
        .repeatRate = 0.064f
    };

    // GuiStyle common settings
    GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 2);
    GuiSetStyle(VALUEBOX, SPINNER_BUTTON_SPACING, -1);
    GuiSetStyle(VALUEBOX, SPINNER_BUTTON_WIDTH, 20);

    // rileva mese e anno correnti da impostare come valore dei due GuiSpinner
    // usati nel filtro dei file
    time_t now = time (NULL);
    struct tm *t = localtime(&now);
    //int currentMonth = t->tm_mon + 1; // current month
    int currentYear = t->tm_year + 1900; // current year

    selectedMonth = 0; //currentMonth;
    selectedYear = currentYear;

    // Applica il filtro iniziale in base a bool filterEnable...
    applyDateFilter(filterEnabled, selectedYear, selectedMonth);

    // Array di puntatori ai nomi files filtrati
    const char *displayNames[MAX_FILES];

    // fai riapparire finestra dopo caricamento iniziale
    // set FPS
    SetTargetFPS(60);
    //EnableEventWaiting(); //  per app statiche : verificare se porta benefici cmq dipende dalla app
                          // questa praticamente non va quasi mai niente...
    ClearWindowState(FLAG_WINDOW_HIDDEN);

// Main game loop
while (!WindowShouldClose())
{

//----------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------

        frameCounter++;
		float deltaTime = GetFrameTime();
        // gestione zoom su mappa
        Vector2 thisPos = GetMousePosition();

        if (IsKeyPressed(KEY_T)) {
            isDarkMode = !isDarkMode;
            // Applica tema colori
            ApplyTheme(&currentTheme, isDarkMode);
            // aggiorna grafico perche viene disegnato solo quando viene caricato file .fit 
            drawChart(targetChart);
        }
        if (btnQuit || IsKeyPressed(KEY_Q)) break;
        // Applica zoom e pan solo se il mouse è dentro mapRect
        if (CheckCollisionPointRec(thisPos, mapRect)) {
            SetMouseCursor(4);

            // --- INPUT: RICENTRA TRACCIA CON TASTO X ---
            if (IsKeyPressed(KEY_X)) FitTrackToScreen(minLat, maxLat, minLon, maxLon, GetScreenWidth(), GetScreenHeight(), 80, &zoom, &camWorldX, &camWorldY);
            
            if (IsKeyPressed(KEY_K) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) showKM = !showKM;

            // --- INPUT: PAN CON MOUSE ---
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            camWorldX -= delta.x;
            camWorldY -= delta.y;
            }
            // --- INPUT: ZOOM CON ROTELLINA (centrato sul cursore) ---
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
            int newZoom = (zoom + (wheel > 0 ? 1 : -1));
                if (newZoom >= MIN_ZOOM && newZoom <= MAX_ZOOM) {
                    Vector2 mouse = GetMousePosition();
                    double mouseWorldX = camWorldX - (GetScreenWidth() / 2.0) + mouse.x;
                    double mouseWorldY = camWorldY - (GetScreenHeight() / 2.0) + mouse.y;
                    double scale = pow(2.0, newZoom - zoom);
                    mouseWorldX *= scale;
                    mouseWorldY *= scale;
                    zoom = newZoom;
                    camWorldX = mouseWorldX + (GetScreenWidth() / 2.0) - mouse.x;
                    camWorldY = mouseWorldY + (GetScreenHeight() / 2.0) - mouse.y;
                }
            }
        } else SetMouseCursor(0); //restore os cursor visibility outside matrix

        float spessoreLinea = 0.36f * zoom; 
        // float diametroPunto = 0.72f * zoom;


    //----------------------------------------------------------------------
    //  file LIbrary management
    //----------------------------------------------------------------------
                // Scroll con tastiera per spostarsi tra i files (solo se si è con il mouse sulla zona della libreria)
                int maxOffset = filteredCount - visibleRows;
                if (maxOffset < 0) maxOffset = 0;
                int activatedFileIndex = -1;

                // mouse dentro l'area della lista files...
                if (CheckCollisionPointRec(thisPos, filesRect))
                {
                    // Scroll libero con la rotellina (sposta la vista senza cambiare selectedIndex)
                    float wheel = GetMouseWheelMove();
                    if (wheel != 0.0f) {
                        scrollOffset -= (int)wheel * 1; // velocità scroll in righe 
                        if (scrollOffset < 0) scrollOffset = 0;
                        if (scrollOffset > maxOffset) scrollOffset = maxOffset;
                    }

                    // Click sinistro per selezione e doppio click 
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        int clickedRow = (int)((thisPos.y - filesRect.y) / rowHeight);
                        int clickedIndex = scrollOffset + clickedRow;

                        if (clickedRow >= 0 && clickedRow < visibleRows && clickedIndex < filteredCount) {
                            double now = GetTime();
                            // Doppio click sullo stesso identico elemento
                            if (clickedIndex == lastClickedIndex && (now - lastClickTime) <= DOUBLE_CLICK_DELAY)
                            {
                                activatedFileIndex = clickedIndex; // Segna il file da aprire
                                lastClickedIndex = -1;
                                lastClickTime = 0.0;
                            } else {
                                // Click singolo: seleziona soltanto, senza toccare scrollOffset
                                selectedIndex = clickedIndex;
                                lastClickedIndex = clickedIndex;
                                lastClickTime = now;
                            }
                        }
                    }
                }

                if (filteredCount > 0) {
                    bool selectionChangedByKey = false;
                    if (UpdateKeyRepeater(&repeater, KEY_DOWN, deltaTime) && selectedIndex < filteredCount - 1) {
                        selectedIndex++;
                        selectionChangedByKey = true;
                    }

                    if (UpdateKeyRepeater(&repeater, KEY_UP, deltaTime) && selectedIndex > 0) {
                        selectedIndex--;
                        selectionChangedByKey = true;
                    }

                    if (UpdateKeyRepeater(&repeater, KEY_PAGE_DOWN, deltaTime)) {
                        selectedIndex += visibleRows;
                        if (selectedIndex >= filteredCount) selectedIndex = filteredCount - 1;
                        selectionChangedByKey = true;
                    }

                    if (UpdateKeyRepeater(&repeater, KEY_PAGE_UP, deltaTime)) {
                        selectedIndex -= visibleRows;
                        if (selectedIndex < 0) selectedIndex = 0;
                        selectionChangedByKey = true;
                    }

                    // 5. HOME (Primo file della lista)
                    if (IsKeyPressed(KEY_HOME)) {
                        selectedIndex = 0;
                        selectionChangedByKey = true;
                    }

                    // 6. FINE (Ultimo file della lista)
                    if (IsKeyPressed(KEY_END)) {
                        selectedIndex = filteredCount - 1;
                        selectionChangedByKey = true;
                    }

                    // Riporta `selectedIndex` dentro l'area visibile SOLO se l'utente ha premuto un tasto di navigazione
                    if (selectionChangedByKey)
                    {
                        // Se la selezione è finita sopra la prima riga visibile
                        if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
                        // Se la selezione è finita sotto l'ultima riga visibile
                        else if (selectedIndex >= scrollOffset + visibleRows) scrollOffset = selectedIndex - visibleRows + 1;
                    }

                    // Controllo di sicurezza finale sui limiti di scrollOffset
                    int maxOffset = filteredCount - visibleRows;
                    if (maxOffset < 0) maxOffset = 0;
                    if (scrollOffset > maxOffset) scrollOffset = maxOffset;
                    if (scrollOffset < 0) scrollOffset = 0;
                }

                    if (filteredCount > 0 && selectedIndex >= 0 && selectedIndex < filteredCount) {
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) activatedFileIndex = selectedIndex; // Segna il file attualmente selezionato
                    }

                    // Esecuzione tasks sia per doppio click che per Invio
                    if (activatedFileIndex != -1)
                    {
                        int realIndex = filteredIndices[selectedIndex];
                        strcpy(fName,allFiles[realIndex].path);

                        // decodifica il file .FIT
                        decodeFit (fName);
                        // disegna grafico altimetria
                        selectedPointIdx = -1;
                        drawChart(targetChart);
                        isTrackLoaded = true;
                        
                        // Array di supporto per salvare i pixel convertiti
                        for (int i = 0; i < numPointRec; i++) puntiPixel[i] = (Vector2){puntiGps[i].lat, puntiGps[i].lon};
                    }

    // -------------------------------------------------------------------------
    // color GuiStyle widgets
    //--------------------------------------------------------------------------
        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(currentTheme.bgColor));

        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(currentTheme.gridColor));
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,   ColorToInt(currentTheme.gridBgColor));
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,   ColorToInt(currentTheme.fgColor));

        GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(currentTheme.gridColor));
        GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,   ColorToInt(currentTheme.gridBgColor));
        GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,   ColorToInt(currentTheme.accentColor));

        GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(currentTheme.gridColor));
        GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,   ColorToInt(currentTheme.gridBgColor));
        GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,   ColorToInt(currentTheme.fgColor));

        GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, ColorToInt(currentTheme.gridColor));
        GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED,   ColorToInt(currentTheme.gridBgColor));
        GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED,   ColorToInt(currentTheme.gridColor));

    // Filtro files: variabili  per capire se l'utente ha cambiato filtro..
        bool prevFilter = filterEnabled;
        int prevYear = selectedYear;
        int prevMonth = selectedMonth;


//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------
        
        BeginTextureMode(target);
            ClearBackground(currentTheme.bgColor);
        EndTextureMode();
		
         BeginDrawing();
            ClearBackground(currentTheme.bgColor);

            // -----------------------------------------------------------------
            // disegna traccia sulla mappa
            //------------------------------------------------------------------


        BeginScissorMode(mapRect.x, mapRect.y, mapRect.width, mapRect.height);
                double topLeftWorldX = camWorldX - mapRect.width / 2.0;
                double topLeftWorldY = camWorldY - mapRect.height / 2.0;

                int maxTileIndex = (1 << zoom) - 1;
                int startTileX = (int)floor(topLeftWorldX / TILE_SIZE);
                int startTileY = (int)floor(topLeftWorldY / TILE_SIZE);
                int endTileX   = (int)floor((topLeftWorldX + mapRect.width) / TILE_SIZE);
                int endTileY   = (int)floor((topLeftWorldY + mapRect.height) / TILE_SIZE);

                // 1. Disegna i tile visibili (aggiungendo mapRect.x e mapRect.y)
                for (int ty = startTileY; ty <= endTileY; ty++) {
                    for (int tx = startTileX; tx <= endTileX; tx++) {
                        if (tx < 0 || tx > maxTileIndex || ty < 0 || ty > maxTileIndex) continue;

                        float screenX = mapRect.x + roundf((float)(tx * TILE_SIZE - topLeftWorldX));
                        float screenY = mapRect.y + roundf((float)(ty * TILE_SIZE - topLeftWorldY));

                        Texture2D *tex = GetTileTexture(zoom, tx, ty, frameCounter);
                        if (tex) {
                            BeginShaderMode(mapShader);
                                DrawTexture(*tex, (int)screenX, (int)screenY, WHITE);
                            EndShaderMode();
                        } else if (!DrawFallbackTile(zoom, tx, ty, screenX, screenY, frameCounter)) {
                            DrawRectangleLines((int)screenX, (int)screenY, TILE_SIZE, TILE_SIZE, currentTheme.gridBgColor);
                        }
                    }
                }

        if (isTrackLoaded) {
                    // 2. Disegna la traccia GPS (anch'essa traslata di mapRect.x / mapRect.y e tagliata ai bordi)
                        double prevX, prevY;
                        //punto GPS iniziale
                        LatLonToWorldPixel(puntiPixel[1].x, puntiPixel[1].y, zoom, &prevX, &prevY);
                        Vector2 pPrev = { mapRect.x + (float)(prevX - topLeftWorldX), mapRect.y + (float)(prevY - topLeftWorldY)};
                        // punti GPS successivi
                        for (int i = 1; i < numPointRec-1 ; i++) {
                            double nextX, nextY;
                            LatLonToWorldPixel(puntiPixel[i + 1].x, puntiPixel[i + 1].y, zoom, &nextX, &nextY);
                            Vector2 pNext = { mapRect.x + (float)(nextX - topLeftWorldX), mapRect.y + (float)(nextY - topLeftWorldY) };
                            DrawLineEx(pPrev, pNext, spessoreLinea, MAROON);
                            //DrawCircleV(pPrev, spessoreLinea, MAROON);
                            pPrev = pNext;
                        }

                         if (showKM && zoom > 11) {
                            for (int i = 0; i < numPointRec - 1; i+=zoom*10) {
                                // disegna label KM : KEY_K
                                double labelX, labelY;
                                LatLonToWorldPixel(puntiPixel[i + 1].x, puntiPixel[i + 1].y, zoom, &labelX, &labelY);
                                Vector2 pLabel = { mapRect.x + (float)(labelX - topLeftWorldX), mapRect.y + (float)(labelY - topLeftWorldY) };

                                snprintf(tempBuffer, sizeof(tempBuffer), " %.f ", puntiGps[i].dist);
                                Vector2 labelPos = MeasureTextEx(textFont, tempBuffer, textFont.baseSize, 0);
                                DrawRectangleRounded((Rectangle){pLabel.x,pLabel.y ,labelPos.x,labelPos.y},0.24f,4,YELLOW);
                                DrawRectangleRoundedLinesEx((Rectangle){pLabel.x,pLabel.y, labelPos.x,labelPos.y},0.24f,4,1.0f,darkTheme.borderColor);
                                DrawTextEx(textFont, tempBuffer, pLabel, (float)textFont.baseSize, 0,BLACK);
                            }
                        }   

                        // punto/posizione sulla traccia che scorre in base alla posizione del mous sul grafico altimetria/velocita
                        double pointX, pointY;
                        LatLonToWorldPixel(puntiPixel[selectedPointIdx].x, puntiPixel[selectedPointIdx].y, zoom, &pointX, &pointY);
                        Vector2 pPoint = { mapRect.x + (float)(pointX - topLeftWorldX), mapRect.y + (float)(pointY - topLeftWorldY)};
                        Vector2 statusOrigin = { gpsIcon.width, gpsIcon.height };
                         DrawTexturePro(gpsIcon, (Rectangle){ 0, 0, gpsIcon.width, gpsIcon.height }, (Rectangle){ pPoint.x, pPoint.y, 16, 16 },  statusOrigin, 0.0f, DARKBLUE );
    
                }
            // copyright mappa
            const char *attr = "Map data: (c) OpenStreetMap contributors, SRTM | Style: (c) OpenTopoMap (CC-BY-SA)";
            int attrWidth = MeasureText(attr, 10);
            DrawRectangle((int)(mapRect.x + mapRect.width) - attrWidth - 14, (int)(mapRect.y + mapRect.height) - 20, attrWidth + 10, 16, Fade(BLACK, 0.50f));
            DrawText(attr, (int)(mapRect.x + mapRect.width) - attrWidth - 9, (int)(mapRect.y + mapRect.height) - 17, 10, WHITE);
        EndScissorMode();
            // bordo attorno alla mappa 
            DrawRectangleLinesEx(mapRect, 1,currentTheme.gridColor);

            // -----------------------------------------------------------------
            // informazioni traccia
            //------------------------------------------------------------------
            DrawRectangleRec(dataRect, currentTheme.gridBgColor);
            DrawRectangleLinesEx(dataRect, 1,currentTheme.gridColor);
            DrawLine(dataRect.x, dataRect.y +44 ,dataRect.x + dataRect.width,dataRect.y +44,currentTheme.gridColor);

            if (isTrackLoaded) {

                // dividi la stringa con separatore ogni stringa separata diventa elemento di un array..
                int count=0;
                char **splitStr = TextSplit(infoTraccia.data_inizio, '/' , &count); 
                Vector2 headerPos = MeasureTextEx(titleFont, splitStr[0], titleFont.baseSize, 0);
                DrawTextEx(titleFont,TextFormat("%s", splitStr[0]) ,(Vector2){dataRect.x + (dataRect.width - headerPos.x)/2, dataRect.y+2},(float)titleFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Ora inizio attività:",(Vector2){dataRect.x+8,dataRect.y +48},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",splitStr[1]) ,(Vector2){dataRect.x + 204, dataRect.y+48},(float)boldFont.baseSize,0,currentTheme.fgColor);

                splitStr = TextSplit(infoTraccia.data_fine, '/' , &count); 
                DrawTextEx(textFont,"Ora fine attività:",(Vector2){dataRect.x+8,dataRect.y + 64},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",splitStr[1]) ,(Vector2){dataRect.x + 204, dataRect.y + 64},(float)boldFont.baseSize,0,currentTheme.fgColor);

                getTimeFromMinute(infoTraccia.minMov);
                DrawTextEx(textFont,"Tempo in movimento:",(Vector2){dataRect.x+8,dataRect.y + 80},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",tempBuffer) ,(Vector2){dataRect.x + 208, dataRect.y + 80},(float)boldFont.baseSize,0,currentTheme.fgColor);

                getTimeFromMinute(infoTraccia.minTot);
                DrawTextEx(textFont,"Tempo totale sessione:",(Vector2){dataRect.x+8,dataRect.y + 96},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",tempBuffer) ,(Vector2){dataRect.x + 208, dataRect.y + 96},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Distanza totale:",(Vector2){dataRect.x+8,dataRect.y + 112},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.2f km",infoTraccia.disTot) ,(Vector2){dataRect.x + 208, dataRect.y + 112},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Velocità media:",(Vector2){dataRect.x+8,dataRect.y + 128},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f km/h",infoTraccia.avgSpd) ,(Vector2){dataRect.x + 208, dataRect.y + 128},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Velocità massima:",(Vector2){dataRect.x+8,dataRect.y + 144},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f km/h",infoTraccia.maxSpd) ,(Vector2){dataRect.x + 208, dataRect.y + 144},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Dislivello positivo (salita):",(Vector2){dataRect.x+8,dataRect.y + 160},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f metri",infoTraccia.altPos) ,(Vector2){dataRect.x + 208, dataRect.y + 160},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Dislivello negativo (discesa):",(Vector2){dataRect.x+8,dataRect.y + 176},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f metri",infoTraccia.altNeg) ,(Vector2){dataRect.x + 208, dataRect.y + 176},(float)boldFont.baseSize,0,currentTheme.fgColor);
                
                DrawTextEx(textFont,"Calorie consumate:",(Vector2){dataRect.x+8,dataRect.y + 192},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f kcal",infoTraccia.calorie) ,(Vector2){dataRect.x + 208, dataRect.y + 192},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Temperatura media:",(Vector2){dataRect.x+8,dataRect.y + 208},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f° C.",infoTraccia.avgTemp) ,(Vector2){dataRect.x + 208, dataRect.y + 208},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Temperatura massima:",(Vector2){dataRect.x+8,dataRect.y + 224},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f° C.",infoTraccia.maxTemp) ,(Vector2){dataRect.x + 208, dataRect.y + 224},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Altitudine di partenza:",(Vector2){dataRect.x+8,dataRect.y + 240},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f metri",puntiGps[0].alt) ,(Vector2){dataRect.x + 208, dataRect.y + 240},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Altitudine media:",(Vector2){dataRect.x+8,dataRect.y + 256},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f metri",infoTraccia.altAvg) ,(Vector2){dataRect.x + 208, dataRect.y + 256},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Altitudine massima:",(Vector2){dataRect.x+8,dataRect.y + 272},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f metri",infoTraccia.altMax) ,(Vector2){dataRect.x + 208, dataRect.y + 272},(float)boldFont.baseSize,0,currentTheme.fgColor);
            } 
            else
             DrawTextEx(subTitleFont,"Seleziona un file dalla lista...",(Vector2){dataRect.x + 28, (dataRect.y + 8)},(float)subTitleFont.baseSize,0,currentTheme.gridColor);


            // -----------------------------------------------------------------
            // Grafico altimetria
            //------------------------------------------------------------------

            DrawRectangleRec(chartRect,currentTheme.gridBgColor);
            DrawRectangleLinesEx(chartRect, 1,currentTheme.gridColor);

            BeginScissorMode(chartRect.x, chartRect.y, chartRect.width, chartRect.height);
                if (isTrackLoaded) {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)targetChart.texture.width, - (float)targetChart.texture.height };
                    // disegna texture grafico alt / spped
                    DrawTextureRec(targetChart.texture, sourceRec, (Vector2){ chartRect.x, chartRect.y }, WHITE);
                     // tooltip interattivo in tempo reale
                    drawChartTooltip();
                } else DrawTextEx(subTitleFont,"Seleziona un file dalla lista...",(Vector2){chartRect.x + 256, (chartRect.y +88)},(float)subTitleFont.baseSize,0,currentTheme.gridColor);
            EndScissorMode();

            // -----------------------------------------------------------------
            //  file list / library 
            //------------------------------------------------------------------q
            DrawRectangleRec(filesRect,currentTheme.gridBgColor);
            DrawRectangleLinesEx(filesRect,1,currentTheme.gridColor);
                    
            for (int i = 0; i < visibleRows; ++i)
            {
                int fileIndex = scrollOffset + i;
                if (fileIndex >= filteredCount) break;

                // even/odd row background
                if (i % 2)
                    DrawRectangleRec((Rectangle){ filesRect.x + 1, filesRect.y + (i * rowHeight) + 1, filesRect.width - 2, rowHeight - 1 }, Fade(currentTheme.bgColor, 0.50f));

                if (fileIndex == selectedIndex)
                    DrawRectangle(filesRect.x + 1, filesRect.y + (i * rowHeight) + 1, filesRect.width - 2, rowHeight - 1, currentTheme.accentColor);

                Color textColor = (fileIndex == selectedIndex) ? GetReadableTextColor(currentTheme.accentColor) : currentTheme.fgColor;

                DrawTextEx(boldFont, TextFormat("%05i", fileIndex + 1), (Vector2){ filesRect.x + 6, filesRect.y + (i * rowHeight) + 3 }, (float)boldFont.baseSize, -1, textColor);
                const char *fileNameTrunc = TextSubtext(GetFileName(displayNames[fileIndex]), 0, 44);
                DrawTextEx(textFont, TextFormat("%s", fileNameTrunc), (Vector2){ filesRect.x + 54, filesRect.y + (i * rowHeight) + 2 }, (float)textFont.baseSize, 0, textColor);
            }   

            //vertical divider between index and filename 
            DrawLine(filesRect.x+48,filesRect.y,filesRect.x+48,filesRect.y + (rowHeight * maxRows),currentTheme.gridColor);
            DrawRectangleLinesEx((Rectangle){filesRect.x,filesRect.y,filesRect.width,1 + (rowHeight * maxRows)},1.0f,currentTheme.gridColor);

            // -----------------------------------------------------------------
            // FILTRO x files list
            //----------------------------------------------------------------

            // la ricerca va fatta una sola volta
            GuiCheckBox((Rectangle){ 14, 678, 16, 16 }, "Filtro ", &filterEnabled);
            if (!filterEnabled) {
                 selectedMonth = 0;//currentMonth;
                 selectedYear = currentYear;
                 GuiDisable();
            }

            GuiSpinner((Rectangle){ 134, 674, 72, 22 }, "Mese ", &selectedMonth, 0, 12, 0);
            GuiSpinner((Rectangle){ 246, 674, 96, 22 }, "Anno ", &selectedYear, 2020, 2040, 0);
            GuiEnable();            

            // Se un controllo è cambiato, ricalcola gli indici filtrati (istantaneo!)
            if (prevFilter != filterEnabled || prevYear != selectedYear || prevMonth != selectedMonth) {
                applyDateFilter(filterEnabled, selectedYear, selectedMonth);
            // setta variabile di visualizzazione libreria in base al numero di files presenti...
                selectedIndex = 0;
                if (filteredCount<maxRows) visibleRows = filteredCount;
                else visibleRows=maxRows;
            }

            //  list files filtrati ---
            for (int i = 0; i < filteredCount; i++) {
                int realIndex = filteredIndices[i];
                displayNames[i] = allFiles[realIndex].path;
            }


            // -----------------------------------------------------------------
            //  STATUS BARS
            //------------------------------------------------------------------
            char infoText[1024];
            if (filterEnabled) snprintf(infoText, sizeof(infoText), "Filtro attivo: %s %04d, %d files presenti.", mesi[selectedMonth], selectedYear, filteredCount);
            else snprintf(infoText, sizeof(infoText), "Filtro non attivo! %d files presenti.", filteredCount);
            GuiStatusBar((Rectangle){ 8, screenHeight-28, filesRect.width, 22 }, infoText);
            // titolo finestra
            DrawText(TextFormat("%s", TOOL_NAME), 40, 14,10,currentTheme.fgColor); 
            DrawText(TextFormat("vers. %s", TOOL_VERSION), 290, 14,10,currentTheme.fgColor); 
            // //DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);
            snprintf(infoText, sizeof(infoText), "[ File in uso: %s | Record letti: %d | Record campionati: %d ] [ Map zoom: %d / %d ]",fName,numRecord, numPointRec, zoom, MAX_ZOOM );
            GuiStatusBar((Rectangle){ chartRect.x, screenHeight-28, chartRect.width, 22 }, infoText);

        
         btnQuit = GuiButton((Rectangle){8,8,20,20}, "#128#");
        EndDrawing();
	}

    UnloadRenderTexture(targetChart);
    UnloadTexture(gpsIcon);
    UnloadRenderTexture(target);
    UnloadFont(boldFont);
    UnloadFont(textFont);
    UnloadFont(titleFont);
    UnloadFont(subTitleFont);
      UnloadShader(mapShader);

    // --- PULIZIA RISORSE ---
    pthread_mutex_lock(&queueMutex);
    workerRunning = false;
    pthread_cond_broadcast(&queueCond);
    pthread_mutex_unlock(&queueMutex);
    pthread_join(workerThread, NULL);
    for (int i = 0; i < MAX_RAM_TILES; i++) {
        if (ramCache[i].inUse) {
        UnloadTexture(ramCache[i].tex);
        }
    }
CloseWindow();
curl_global_cleanup();
return 0;
}


// Funzione che aggiorna lo stato delle pressione di un tasto
// e restituisce true se l'azione deve essere eseguita
bool UpdateKeyRepeater(KeyRepeater *repeater, int key, float deltaTime) {
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
    repeater->timer += deltaTime;

    if (!repeater->isRepeating) {
        // ritardo iniziale
        if (repeater->timer >= repeater->firstDelay) {
            repeater->timer = 0.0f;
            repeater->isRepeating = true;
            return true;
        }
    } else {
        // ripetizione continua e rate
        if (repeater->timer >= repeater->repeatRate) {
            repeater->timer -= repeater->repeatRate;
            return true;
        }
    }
    return false;
}

// Funzione di supporto per convertire il timestamp FIT in una stringa leggibile
void ConvertFitTimestamp(FIT_UINT32 fit_timestamp, char* output_str, size_t max_size) 
{
    if (fit_timestamp == FIT_UINT32_INVALID) {
        snprintf(output_str, max_size, "Dato non valido");
        return;
    }
    
    // Converte il tempo Garmin in tempo Unix standard
    time_t unix_time = (time_t)fit_timestamp + GARMIN_TIME_OFFSET;
    
    // Usa gmtime se vuoi l'orario UTC, oppure localtime per l'orario del tuo computer
    struct tm* time_info = localtime(&unix_time);
    strftime(output_str, max_size, "%a, %d %b %Y / %H:%M:%S", time_info);
}

int decodeFit(char* fileName) {

   FILE *file;
   FIT_UINT8 buf[8];
   FIT_CONVERT_RETURN convert_return = FIT_CONVERT_CONTINUE;
   FIT_UINT32 buf_size;
   #if defined(FIT_CONVERT_MULTI_THREAD)
      FIT_CONVERT_STATE state;
   #endif

   #if defined(FIT_CONVERT_MULTI_THREAD)
      FitConvert_Init(&state, FIT_TRUE);
   #else
      FitConvert_Init(FIT_TRUE);
   #endif

   if((file = fopen(fileName, "rb")) == NULL)
   {
      printf("Error opening file %s.\n", fileName);
      return FIT_FALSE;
   }
    numRecord = 0;
    numPointRec = 0;
    double lat = 0;
    double lon = 0;
    double alt = 0 ;
    double dist = 0;
    double vel = 0;

    while(!feof(file) && (convert_return == FIT_CONVERT_CONTINUE))
    {
          for(buf_size=0;(buf_size < sizeof(buf)) && !feof(file); buf_size++)
          {
             buf[buf_size] = (FIT_UINT8)getc(file);
          }
          
          do 
          {
             #if defined(FIT_CONVERT_MULTI_THREAD)
                convert_return = FitConvert_Read(&state, buf, buf_size);
             #else
                convert_return = FitConvert_Read(buf, buf_size);
             #endif
             switch (convert_return)
             {
                case FIT_CONVERT_MESSAGE_AVAILABLE:
                {
                   #if defined(FIT_CONVERT_MULTI_THREAD)
                      const FIT_UINT8 *mesg = FitConvert_GetMessageData(&state);
                      FIT_UINT16 mesg_num = FitConvert_GetMessageNumber(&state);
                   #else
                      const FIT_UINT8 *mesg = FitConvert_GetMessageData();
                      FIT_UINT16 mesg_num = FitConvert_GetMessageNumber();
                   #endif
                   
                   switch(mesg_num)
                   {
                      case FIT_MESG_NUM_FILE_ID:
                      {
                         const FIT_FILE_ID_MESG *id = (FIT_FILE_ID_MESG *) mesg;
                         printf("File ID: type=%u, number=%u\n", id->type, id->number);
                         break;
                      }

                      case FIT_MESG_NUM_RECORD:
                      {
                        // il fit registra un record ogni secondo.. sono anche troppi per la risoluzione 
                        // la traccia sarebbe perfetta ma per il mio pc la cpu va oltre il 20%
                        // forse su un pc del 2027 se ne fa un baffo ma sul mio T480 devo limitare
                        // metto che registra 1 record ogni 5 per un compromesso tra performance e qualita traccia   
                        // per restare intorno al 10% di cpu..ovvio che se metto tipo 10 consumo ancora meno risorse
                        // ma la traccia specialmente al livello di zoom massimo verraa disegnata a linee distanti...
                        // specialmente quando si va veloce .. in montagna invece andando piano i punti saranna cmq
                        // piu ravvicinati
                            if (numRecord % 5 == 0) { 
                                 const FIT_RECORD_MESG *record = (FIT_RECORD_MESG *) mesg;
                                 // 1. Data e Ora Leggibile
                                  ConvertFitTimestamp(record->timestamp, tempBuffer, sizeof(tempBuffer));

                                // 2 & 3. Latitudine e Longitudine
                                  if (record->position_lat != FIT_SINT32_INVALID && record->position_long != FIT_SINT32_INVALID) {
                                      lat = (double)record->position_lat * SEMICIRCLES_TO_DEGREES;
                                      lon = (double)record->position_long * SEMICIRCLES_TO_DEGREES;
                                  }
                                // 4. Altitudine
                                if (record->altitude != FIT_UINT16_INVALID) alt = ((double)record->altitude / 5.0) - 500.0;
                                
                                // 5. Distanza percorsa accumulata
                                if (record->distance != FIT_UINT32_INVALID) dist = (double)record->distance / 100.0 / 1000.0;
                                
                                // 6. Velocità istantanea
                                if (record->enhanced_speed != FIT_UINT32_INVALID) vel = ((double)record->enhanced_speed/ 1000.0) * 3.6;
                                else if (record->speed != FIT_UINT16_INVALID) vel = ((double)record->speed / 1000.0) * 3.6;

                                // assegnazione valori
                                puntiGps[numPointRec].record = numRecord;
                                strcpy( puntiGps[numPointRec].data, tempBuffer);
                                puntiGps[numPointRec].lat = lat;
                                puntiGps[numPointRec].lon = lon;
                                puntiGps[numPointRec].alt = alt;
                                puntiGps[numPointRec].dist = dist;
                                puntiGps[numPointRec].vel= vel;
                                
                                numPointRec++;
                            }                            
                            numRecord++;
                            break;
                      }

                      case FIT_MESG_NUM_SESSION:
                      {
                         const FIT_SESSION_MESG *session = (FIT_SESSION_MESG *) mesg;
                            ConvertFitTimestamp(session->start_time, tempBuffer, sizeof(tempBuffer));
                            strcpy(infoTraccia.data_inizio, tempBuffer);
                            ConvertFitTimestamp(session->timestamp, tempBuffer, sizeof(tempBuffer));
                            strcpy(infoTraccia.data_fine, tempBuffer);
                            if (session->total_elapsed_time != FIT_UINT32_INVALID) infoTraccia.minTot = (double)session->total_elapsed_time / 1000.0 / 60.0;
                            if (session->total_moving_time != FIT_UINT32_INVALID) infoTraccia.minMov = (double)session->total_moving_time / 1000.0 / 60.0;
                            if (session->total_distance != FIT_UINT32_INVALID) infoTraccia.disTot = (double)session->total_distance / 100.0 / 1000.0;
                            if (session->avg_speed != FIT_UINT16_INVALID) infoTraccia.avgSpd = ((double)session->avg_speed / 1000.0) * 3.6;
                            if (session->max_speed != FIT_UINT16_INVALID) infoTraccia.maxSpd = ((double)session->max_speed / 1000.0) * 3.6;
                            if (session->total_ascent != FIT_UINT16_INVALID) infoTraccia.altPos = session->total_ascent;
                            if (session->total_descent != FIT_UINT16_INVALID) infoTraccia.altNeg =session->total_descent;
                            if (session->total_calories != FIT_UINT16_INVALID) infoTraccia.calorie = session->total_calories;
                            if (session->avg_temperature != FIT_SINT8_INVALID) infoTraccia.avgTemp = session->avg_temperature;
                            if (session->max_temperature != FIT_SINT8_INVALID) infoTraccia.maxTemp = session->max_temperature;
                            if (session->avg_altitude!= FIT_UINT16_INVALID) infoTraccia.altAvg = ((double)session->avg_altitude/ 5.0) - 500.0;
                            if (session->max_altitude!= FIT_UINT16_INVALID) infoTraccia.altMax = ((double)session->max_altitude/ 5.0) - 500.0;
                       break;
                      }
                      default:
                      break;
                   }
                   break;
                }
                default: break;
            }
        } while (convert_return == FIT_CONVERT_MESSAGE_AVAILABLE);
    }

   if (convert_return == FIT_CONVERT_ERROR)
   {
      printf("Error decoding file.\n");
      fclose(file);
      return 1;
   }

   if (convert_return == FIT_CONVERT_CONTINUE)
   {
      printf("Unexpected end of file.\n");
      fclose(file);
      return 1;
   }

   if (convert_return == FIT_CONVERT_DATA_TYPE_NOT_SUPPORTED)
   {
      printf("File is not FIT.\n");
      fclose(file);
      return 1;
   }

   if (convert_return == FIT_CONVERT_PROTOCOL_VERSION_NOT_SUPPORTED)
   {
      printf("Protocol version not supported.\n");
      fclose(file);
      return 1;
   }

   if (convert_return == FIT_CONVERT_END_OF_FILE){
      // output in console some infos related to loaded track
      printf("%s\n",fName );
      printf("File converted successfully.\n");
      printf("\n");
   }
   fclose(file);
   return 0;
}

void loadLibrary (void) {
    // Carica tutti i file dalla cartella desiderata
    totalFilesCount = loadFilesRecursive("."); // carica files .fit
    // setta variabile di visualizzazione libreria in base al numero di files presenti...
    if (totalFilesCount<maxRows) visibleRows = totalFilesCount;
    else visibleRows=maxRows;
}

int compareFiles(const void *a, const void *b) {
    const char *path_a = (const char *)a;
    const char *path_b = (const char *)b;

    // estrapola nome del file dalla path completo, usando raylib
    const char *filename_a = GetFileName(path_a);
    const char *filename_b = GetFileName(path_b);
    
    // Confronto case sensitive...A <> a
    // return strcmp(filename_a, filename_b);
    
    // Confronto case insensitive A = a
    return strcasecmp(filename_a, filename_b); //strcasecmp non esiste in C99
}

int compareFilesByModTime(const void *a, const void *b) {
    const char *path_a = (const char *)a;
    const char *path_b = (const char *)b;

    long time_a = GetFileModTime(path_a);
    long time_b = GetFileModTime(path_b);

    if (time_a > time_b) return -1;
    if (time_a < time_b) return 1;
    return 0;
}


// Carica i file e ne estrae subito anno e mese
int loadFilesRecursive(const char *path) {
    if (!DirectoryExists(path)) {
        TraceLog(LOG_ERROR, "Directory non trovata: %s", path);
        return 0;
    }

    FilePathList list = LoadDirectoryFilesEx(path, FILTER_EXT, true);
    int count = 0;

    for (unsigned int i = 0; i < list.count && count < MAX_FILES; i++) {
        int written = snprintf(allFiles[count].path, MAX_NAME, "%s", list.paths[i]);
        if (written >= 0 && written < MAX_NAME) {
            // Ottiene il timestamp di modifica del file tramite raylib
            long modTime = GetFileModTime(allFiles[count].path);
            time_t t = (time_t)modTime;
            struct tm *tm_info = localtime(&t);

            if (tm_info != NULL) {
                allFiles[count].year = tm_info->tm_year + 1900;
                allFiles[count].month = tm_info->tm_mon + 1; // tm_mon parte da 0
                allFiles[count].day = tm_info->tm_wday; 
            } else {
                allFiles[count].year = 0;
                allFiles[count].month = 0;
                allFiles[count].day = 0;
            }
            count++;
        }
    }

    UnloadDirectoryFiles(list);
    return count;
}

// Aggiorna l'array filteredIndices in base ai parametri scelti nella UI
void applyDateFilter(bool enableFilter, int targetYear, int targetMonth) {

    filteredCount = 0;

    for (int i = 0; i < totalFilesCount; i++) {
        if (!enableFilter) {
            // Se il filtro è disattivato, includi tutti i file
            filteredIndices[filteredCount++] = i;
        } else {
            // Controlla corrispondenza anno (se targetYear > 0) e mese (se targetMonth > 0)
            bool matchYear = (targetYear == 0) || (allFiles[i].year == targetYear);
            bool matchMonth = (targetMonth == 0) || (allFiles[i].month == targetMonth);

            if (matchYear && matchMonth) {
                filteredIndices[filteredCount++] = i;
            }
        }
    }
}

void getTimeFromMinute (double mins)
{
    long totalSec = mins*60;
    long hours = totalSec / 3600;
    long minutes = totalSec % 3600 / 60;
    long seconds = totalSec % 60;
    snprintf(tempBuffer, sizeof(tempBuffer),"%ldore %ldmin %ldsec",hours,minutes,seconds );
}

void drawChart(RenderTexture2D target) {

    if (numPointRec < 2) return;

    // 1. Margini
    float margineSinistro = 36.0f;
    float margineDestro   = 10.0f;
    float margineAlto     = 12.0f;
    float margineBasso    = 18.0f;

    float larghezzaUtile = chartRect.width - margineSinistro - margineDestro;
    float altezzaUtile   = chartRect.height - margineAlto - margineBasso;

    float distMax = (infoTraccia.disTot > 0.0f) ? infoTraccia.disTot : 1.0f;

    // 2. Calcolo altitudine con base fissa a 0.0f
    float minAlt = 0.0f;
    float rawMaxAlt = 0.0f;
    float rawMaxVel = 0.0f;

    for (int i = 0; i < numPointRec; i++) {
        if (puntiGps[i].alt > rawMaxAlt) rawMaxAlt = puntiGps[i].alt;
        if (puntiGps[i].vel > rawMaxVel) rawMaxVel = puntiGps[i].vel;
    }

    if (rawMaxAlt < 10.0f) rawMaxAlt = 10.0f; // Altezza minima del grafico per tracce piatte
      if (rawMaxVel < 1.0f)  rawMaxVel = 1.0f;

    // Calcolo step griglia altitudine (Asse Y)
    float stepAlt = 10.0f;
    if (rawMaxAlt > 2000.0f) stepAlt = 500.0f;
    else if (rawMaxAlt > 1000.0f) stepAlt = 250.0f;
    else if (rawMaxAlt > 400.0f)  stepAlt = 100.0f;
    else if (rawMaxAlt > 150.0f)  stepAlt = 50.0f;
    else if (rawMaxAlt > 50.0f)   stepAlt = 20.0f;

    // Arrotonda maxAlt al multiplo superiore di stepAlt (evita che il picco tocchi il bordo alto)
    float maxAlt = ceilf(rawMaxAlt / stepAlt) * stepAlt;
    float deltaAlt = maxAlt - minAlt;

   // Margine superiore del 10% sulla velocità così il picco non tocca il bordo alto
    float maxVel = rawMaxVel * 1.20f;

    // per interattivita / tooltip alt/vel
    chartMaxAlt = maxAlt;
    chartMaxVel = maxVel;

    float startX = margineSinistro;
    float baseY  = chartRect.height - margineBasso;
    float topY   = margineAlto;

    // 3. Calcolo step griglia distanza (Asse X) in ordine decrescente corretto
    float distMaxKm = distMax; // Dividi per 1000.0f se distMax è in metri

    float stepKm = 1.0f;
    if (distMaxKm > 50.0f)      stepKm = 5.0f;
    else if (distMaxKm > 25.0f) stepKm = 2.0f;
    else if (distMaxKm > 10.0f) stepKm = 1.0f;
    else if (distMaxKm <= 2.0f) stepKm = 0.5f;

  
    // Ogni quante linee verticali stampare l'etichetta (1 = tutte, 2 = una sì e una no)
    int labelStepKm = (distMaxKm <= 2.0f) ? 1 : 2;

        // Allocazione array vertici per riempimento (2 vertici per ogni punto GPS)
    Vector2 *stripPoints = (Vector2 *)MemAlloc(sizeof(Vector2) * numPointRec * 2);
    if (stripPoints == NULL) return;

    BeginTextureMode(target);
        ClearBackground(BLANK);

        // --- GRIGLIA ORIZZONTALE (ALTITUDINE) ---
        float halfStepAlt = stepAlt * 0.5f;
        int totalStepsY = (int)roundf(maxAlt / halfStepAlt);

        for (int s = 0; s <= totalStepsY; s++) {
            float alt = s * halfStepAlt;
            float normY = (alt - minAlt) / deltaAlt;
            float y = baseY - (normY * altezzaUtile);

            if (s > 0) DrawLineDashed((Vector2){ startX, y }, (Vector2){ startX + larghezzaUtile, y }, 1, 1, currentTheme.gridColor);

            // Stampa etichetta ogni 2 mezzi-step (ovvero sui multipli interi di stepAlt)
            if (s % 2 == 0) {
                const char *labelAlt = TextFormat("%.0fm", alt);
                int textW = MeasureText(labelAlt, 10);
                DrawText(labelAlt, (int)(startX - textW - 4), (int)(y - 5), 10, currentTheme.fgColor);
            }
        }

        // --- GRIGLIA VERTICALE (DISTANZA) ---
        int totalStepsX = (int)floorf(distMaxKm / stepKm);

        for (int s = 0; s <= totalStepsX; s++) {
            float km = s * stepKm;
            float normX = km / distMaxKm;
            float x = startX + (normX * larghezzaUtile);

            if (s > 0) DrawLineDashed((Vector2){ x, topY }, (Vector2){ x, baseY }, 1, 1, currentTheme.gridColor);

            // Stampa etichetta usando il contatore intero
            if (s % labelStepKm == 0) {
                const char *labelKm = (stepKm < 1.0f) ? TextFormat("%.1fkm", km) : TextFormat("%.0fkm", km);
                int textW = MeasureText(labelKm, 10);
                int textX = (s == 0) ? (int)x : (int)(x - textW / 2);
                DrawText(labelKm, textX, (int)(baseY + 4), 10, currentTheme.fgColor);
            }
        }


        // Bordi principali degli assi
        DrawLineV((Vector2){ startX, topY }, (Vector2){ startX, baseY }, currentTheme.borderColor);
        DrawLineV((Vector2){ startX, baseY }, (Vector2){ startX + larghezzaUtile, baseY }, currentTheme.borderColor);

       // disegno altimetria---
        for (int i = 0; i < numPointRec; i++) {
            float alt = (puntiGps[i].alt < 0.0f) ? 0.0f : puntiGps[i].alt;

            float normX = puntiGps[i].dist / distMax;
            float normY = (alt - minAlt) / deltaAlt;

            float x = startX + (normX * larghezzaUtile);
            float y = baseY - (normY * altezzaUtile);

            // Ordine antiorario per TriangleStrip: punto superiore (quota), poi punto inferiore (base)
            stripPoints[i * 2]     = (Vector2){ x, y };
            stripPoints[i * 2 + 1] = (Vector2){ x, baseY };
        }

        // 1. Disegno riempimento in un'unica chiamata
        Color fillCol = Fade(currentTheme.accentColor, 0.50f);
        DrawTriangleStrip(stripPoints, numPointRec * 2, fillCol);

        // 2. Disegno linea superiore con spessore 2.0f
        for (int i = 0; i < numPointRec - 1; i++) DrawLineEx(stripPoints[i * 2], stripPoints[(i + 1) * 2], 2.0f, currentTheme.accentColor);

        // disegna grafico velocità
        for (int i = 0; i < numPointRec - 1; i++) {
            float vel1 = (puntiGps[i].vel < 0.0f) ? 0.0f : puntiGps[i].vel;
            float vel2 = (puntiGps[i + 1].vel < 0.0f) ? 0.0f : puntiGps[i + 1].vel;

            float normX1 = puntiGps[i].dist / distMax;
            float normX2 = puntiGps[i + 1].dist / distMax;

            float normY1 = vel1 / maxVel;
            float normY2 = vel2 / maxVel;

            float x1 = startX + (normX1 * larghezzaUtile);
            float y1 = baseY - (normY1 * altezzaUtile);

            float x2 = startX + (normX2 * larghezzaUtile);
            float y2 = baseY - (normY2 * altezzaUtile);

            DrawLineEx((Vector2){ x1, y1 }, (Vector2){ x2, y2 }, 2.0f, isDarkMode?Fade(lightTheme.accentColor, 0.82f):Fade(darkTheme.accentColor, 0.82f));

            // linea vert asse Y sul lato dx per chiudere il grafico
            DrawLineV((Vector2){ startX + larghezzaUtile + 1, topY }, (Vector2){ startX + larghezzaUtile + 1, baseY }, currentTheme.borderColor);
        }
    
    EndTextureMode();
  MemFree(stripPoints);
}

void ApplyTheme(colorTheme *currentTheme, bool darkMode) {
    if (darkMode) *currentTheme = darkTheme;
    else *currentTheme = lightTheme;
}

// Restituisce un colore leggibile (BIANCO o NERO) in contrasto al colore di sfondo
Color GetReadableTextColor(Color bgColor) {
    
    // intercetta colori con trasparenza quasi a zero (che visivamente si fa fatica a distinguere)
    if (bgColor.a <= 24) return BLACK;

    // Formula della luminanza percepita dall'occhio umano
    float luminance = 0.299f * bgColor.r + 0.587f * bgColor.g + 0.114f * bgColor.b;
    return (luminance > 127.5f) ? BLACK : WHITE;
}

void drawChartTooltip(void) {
    if (numPointRec < 2) {
        selectedPointIdx = -1;
        return;
    }

    // Stessi margini usati in drawChart
    float margineSinistro = 36.0f;
    float margineDestro   = 10.0f;
    float margineAlto     = 12.0f;
    float margineBasso    = 18.0f;

    Rectangle plotArea = {
        chartRect.x + margineSinistro,
        chartRect.y + margineAlto,
        chartRect.width - margineSinistro - margineDestro,
        chartRect.height - margineAlto - margineBasso
    };

    float distMax = (infoTraccia.disTot > 0.0f) ? infoTraccia.disTot : 1.0f;
    Vector2 mouse = GetMousePosition();
    Vector2 mouseDelta = GetMouseDelta();
    bool mouseInPlot = CheckCollisionPointRec(mouse, plotArea);

    // 1. Aggiornamento tramite MOUSE (solo se dentro il grafico e in movimento, o al clic)
    if (mouseInPlot && (fabsf(mouseDelta.x) > 0.0f || fabsf(mouseDelta.y) > 0.0f || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        float normMouseX = (mouse.x - plotArea.x) / plotArea.width;
        float targetDist = normMouseX * distMax;

        int left = 0;
        int right = numPointRec - 1;
        while (left < right) {
            int mid = left + (right - left) / 2;
            if (puntiGps[mid].dist < targetDist) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }

        int idx = left;
        if (idx > 0 && fabs(puntiGps[idx - 1].dist - targetDist) < fabs(puntiGps[idx].dist - targetDist)) {
            idx = idx - 1;
        }
        selectedPointIdx = idx;
    }

    // 2. Aggiornamento tramite TASTIERA (Frecce Sinistra / Destra)
    if (mouseInPlot && (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT))) {
        if (selectedPointIdx < 0) selectedPointIdx = 0;
        else if (selectedPointIdx < numPointRec - 1) selectedPointIdx++;
    }
    if (mouseInPlot && (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))) {
        if (selectedPointIdx < 0) selectedPointIdx = numPointRec - 1;
        else if (selectedPointIdx > 0) selectedPointIdx--;
    }

    // 3. Deselezione con ESC o clic fuori dal grafico
    if (IsKeyPressed(KEY_ESCAPE) || (!mouseInPlot && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        selectedPointIdx = -1;
    }

    // Se non c'è nessun punto selezionato (o indice fuori range), esci senza disegnare
    if (selectedPointIdx < 0 || selectedPointIdx >= numPointRec) return;

    int idx = selectedPointIdx;

    // --- CALCOLO PENDENZA SU FINESTRA LOCALE (anti-rumore GPS) ---
    int iStart = idx;
    int iEnd   = idx;
    float minWindowKm = 0.025f;

    while (iStart > 0 && (puntiGps[idx].dist - puntiGps[iStart].dist) < (minWindowKm * 0.5f)) iStart--;
    while (iEnd < numPointRec - 1 && (puntiGps[iEnd].dist - puntiGps[idx].dist) < (minWindowKm * 0.5f)) iEnd++;

    float deltaDistM = (puntiGps[iEnd].dist - puntiGps[iStart].dist) * 1000.0f;
    float deltaAltM  = puntiGps[iEnd].alt - puntiGps[iStart].alt;
    float pendenza   = (deltaDistM > 0.5f) ? (deltaAltM / deltaDistM) * 100.0f : 0.0f;

    Color slopeColor = currentTheme.fgColor;
    if (pendenza > 1.0f)       slopeColor = RED;
    else if (pendenza < -1.0f) slopeColor = GREEN;

    // Coordinate esatte del punto selezionato sullo schermo
    float alt = (puntiGps[idx].alt < 0.0f) ? 0.0f : puntiGps[idx].alt;
    float vel = (puntiGps[idx].vel < 0.0f) ? 0.0f : puntiGps[idx].vel;
    float safeMaxAlt = (chartMaxAlt > 0.001f) ? chartMaxAlt : 1.0f;
    float safeMaxVel = (chartMaxVel > 0.001f) ? chartMaxVel : 1.0f;

    float px = plotArea.x + (puntiGps[idx].dist / distMax) * plotArea.width;
    float pyAlt = (plotArea.y + plotArea.height) - (alt / safeMaxAlt) * plotArea.height;
    float pyVel = (plotArea.y + plotArea.height) - (vel / safeMaxVel) * plotArea.height;

    // Linea verticale guida
    DrawLineV((Vector2){ px, plotArea.y }, (Vector2){ px, plotArea.y + plotArea.height }, Fade(currentTheme.fgColor, 0.5f));
    
    Color velColor = Fade(isDarkMode?SKYBLUE:MAROON,0.72f);
    
    // disegno punti sulle due curve
    DrawCircleV((Vector2){ px, pyAlt }, 4.0f, currentTheme.accentColor);
    DrawCircleLinesV((Vector2){ px, pyAlt }, 4.5f, currentTheme.borderColor);

    DrawCircleV((Vector2){ px, pyVel }, 4.0f, velColor);
    DrawCircleLinesV((Vector2){ px, pyVel }, 4.5f, currentTheme.borderColor);

    char txtData[32], txtDist[32], txtAlt[32], txtVel[32], txtSlope[32];
    snprintf(txtData,  sizeof(txtData),  "%s", puntiGps[idx].data);
    snprintf(txtDist,  sizeof(txtDist),  "Distanza: %.2f km", puntiGps[idx].dist);
    snprintf(txtAlt,   sizeof(txtAlt),   "Altitudine: %.0f m", alt);
    snprintf(txtVel,   sizeof(txtVel),   "Velocità: %.1f km/h", vel);
    snprintf(txtSlope, sizeof(txtSlope), "Pendenza: %+.1f%%", pendenza);

    int fontSize = 10;
    int widths[5] = {
        MeasureText(txtData, fontSize),
        MeasureText(txtDist, fontSize),
        MeasureText(txtAlt, fontSize),
        MeasureText(txtVel, fontSize),
        MeasureText(txtSlope, fontSize)
    };

    // trova testo piu lungo in px...
    int maxW = widths[0];
    for (int i = 1; i < 5; i++) {
        if (widths[i] > maxW) maxW = widths[i];
    }

    float boxW = (float)maxW + 14.0f;
    float boxH = 70.0f;

    // Posizionamento orizzontale
    float boxX = px + 10.0f;
    if (boxX + boxW > plotArea.x + plotArea.width) boxX = px - boxW - 10.0f;

    // Posizionamento verticale: segue il mouse se è dentro il grafico, altrimenti segue il punto altimetrico
    float refY = mouseInPlot ? mouse.y : pyAlt;
    float boxY = refY - (boxH / 2.0f);
    if (boxY < plotArea.y) boxY = plotArea.y;
    if (boxY + boxH > plotArea.y + plotArea.height) boxY = plotArea.y + plotArea.height - boxH;

    Rectangle tooltipRect = { boxX, boxY, boxW, boxH };

    // Disegno sfondo e bordo del tooltip
    DrawRectangleRounded(tooltipRect, 0.2f, 4, Fade(currentTheme.bgColor, 0.90f));
    DrawRectangleRoundedLinesEx(tooltipRect, 0.2f, 4, 1.0f, currentTheme.borderColor);

    // Disegno etichette
    DrawText(txtData,  (int)(boxX + 7), (int)(boxY + 5),  fontSize, GRAY);
    DrawText(txtDist,  (int)(boxX + 7), (int)(boxY + 17),  fontSize, currentTheme.fgColor);
    DrawText(txtAlt,   (int)(boxX + 7), (int)(boxY + 29), fontSize, currentTheme.accentColor);
    DrawText(txtVel,   (int)(boxX + 7), (int)(boxY + 41), fontSize, velColor);
    DrawText(txtSlope, (int)(boxX + 7), (int)(boxY + 53), fontSize, slopeColor);

}
