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
#define TOOL_VERSION            "2.2.0"

#include <raylib.h>
#include <raymath.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "fit_convert.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define _CRT_SECURE_NO_WARNINGS
// Costante per convertire i semicerchi in gradi decimali
#define SEMICIRCLES_TO_DEGREES (180.0 / 2147483648.0)
// Epoca Garmin in formato timestamp Unix (31 Dicembre 1989 00:00:00 UTC)
#define GARMIN_TIME_OFFSET 631065600

// window size (16:9)
const int screenWidth = 1184; 
const int screenHeight = 736;

Rectangle datatRect = { 8,32,340,295};     // top/left
Rectangle mapRect = { 354,32.0,822,468};  // top/right
Rectangle filesRect = { 8,365,340,337}; // bottom/left
Rectangle chartRect = {354,506,822,196};  // bottom/right

const int fntSize = 16;
char tempBuffer[64];
bool showMinimap = false;
bool showKM=false;
bool isDarkMode = false;

float minZoomX;
float minZoomY;
float minZoom;
const float stepZoom = 0.20f;
const float maxZoom = 5.0f;

//mimimap variables
const float maxMapSize = 172.0f; 
float miniMapX;
float miniMapY;
float miniMapWidth;
float miniMapHeight;
float aspectRatio;

float scaleX;
float scaleY;

typedef struct {
    Color accentColor;
    Color fgColor;
    Color bgColor;
    Color borderColor;
    Color gridColor;
    Color gridBgColor;
} colorTheme;

static const colorTheme lightTheme = {
    .accentColor = (Color){53, 132, 228, 255}, 
    .fgColor = (Color){55, 65, 70, 255},
    .bgColor = (Color){218, 227, 227, 255},
    .borderColor = (Color){76, 86, 106, 255},
    .gridColor = (Color){180, 185, 185, 255},
    .gridBgColor = (Color){239, 246, 246, 255},
};

static const colorTheme darkTheme = {
    .accentColor = (Color){230, 45, 66, 255},
    .fgColor = (Color){216, 222, 233, 255},
    .bgColor = (Color){50, 55, 66, 255},
    .borderColor = (Color){76, 86, 106, 255},
    .gridColor = (Color){77, 87, 106, 255},
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

size_t numRecord;
size_t numPointRec;
Vector2 puntiPixel[32768]; // todo : convert to diynamic
gpsPoints puntiGps[32768]; // see above
trackData infoTraccia;

// per tooltip su grafico
static float chartMaxAlt = 10.0f;
static float chartMaxVel = 1.0f;
static int selectedPointIdx = -1;

// library & files management
#define MAX_FILES 2048
#define MAX_NAME 1024
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


char fName[MAX_NAME] = "";
int selectedIndex = 0;
int rowHeight = 21;
int visibleRows;
int maxRows = 16;
int centerRow;
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

void ResetCameraView(Camera2D *cam, Texture2D mapTexture, Rectangle area, float targetZoom, float *outMinZoom);
void calcMinimap (Texture2D mapTexture);
bool UpdateKeyRepeater(KeyRepeater *repeater, int key, float deltaTime);
Vector2 GpsToPixel(double lat, double lon, Texture2D tex);
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

int main (int argc, char *argv[])
{
	 // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED |  FLAG_MSAA_4X_HINT); //|  FLAG_MSAA_4X_HINT); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "fitview");
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works
    
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // carica fonts con tutti i caraatteri da 32 (spazio) a 255 (caratteri estesi europei)
    int startGlyph = 32;
    int endGlyph = 255;
    int glyphCount = endGlyph - startGlyph + 1;

    // Allocachiamo l'array per i codepoints
    int *codepoints = (int *)malloc(sizeof(int) * glyphCount);
    for (int i = 0; i < glyphCount; i++) {
        codepoints[i] = startGlyph + i;
    }

    Font textFont = LoadFontEx("fonts/PixelOperator.ttf", fntSize,codepoints, glyphCount); // all other text
    Font boldFont = LoadFontEx("fonts/PixelOperator-Bold.ttf", fntSize,codepoints, glyphCount); // all other text
    Font titleFont = LoadFontEx("fonts/HelveticaNeue-Medium.otf", 36,codepoints, glyphCount); // all other text
    Font subTitleFont = LoadFontEx("fonts/HelveticaNeue.otf", 28,codepoints, glyphCount); // all other text

    // Caricamento mappa ( creare TIFF con QGIS)
    Texture2D mapTexture;
    if (FileExists("assets/map.png")) {
        mapTexture = LoadTexture("assets/map.png");
    } else {
        Image img = GenImageChecked(mapRect.width, mapRect.height, 12, 12, WHITE, RAYWHITE);
        mapTexture = LoadTextureFromImage(img);
        UnloadImage(img);
    }

	RenderTexture targetChart = LoadRenderTexture(mapRect.width, mapRect.height);
    Texture2D cityIcon= LoadTexture("assets/position.png");          // Image converted to texture, GPU memory (VRAM)

    // setup initial camera settings
    Camera2D cam = { 0 };
    float minZoom = 1.0f;
    ResetCameraView(&cam, mapTexture, mapRect, 0.0f, &minZoom);
    // apply color theme
    ApplyTheme(&currentTheme, isDarkMode);

// load file list and sort by file modification time
    loadLibrary();
    qsort(allFiles, totalFilesCount, sizeof(fileEntry), compareFilesByModTime);  // ordinamento file

    //generate mini-map
    calcMinimap(mapTexture);

    // set key repeater speed (400ms initial delay, 80ms repeat rate)
    KeyRepeater repeater = {
        .currentKey = 0,
        .timer = 0.0f,
        .isRepeating = false,
        .firstDelay = 0.40f,
        .repeatRate = 0.064f
    };

    Vector2 prevMousePos = GetMousePosition();

    // GuiStyle common settings
    GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 2);
    GuiSetStyle(VALUEBOX, SPINNER_BUTTON_SPACING, -1);
    GuiSetStyle(VALUEBOX, SPINNER_BUTTON_WIDTH, 20);

    // rileva mese e anno correnti da impostare come valore dei due GuiSpinner
    // usati nel filtro dei file
    time_t now = time (NULL);
    struct tm *t = localtime(&now);
    int currentMonth = t->tm_mon + 1; // current month
    int currentYear = t->tm_year + 1900; // current year

    selectedMonth = currentMonth;
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

		float deltaTime = GetFrameTime();
        float speed = 150.0f /  cam.zoom;

        Vector2 thisPos = GetMousePosition();
        Vector2 delta = Vector2Subtract(prevMousePos, thisPos);
        prevMousePos = thisPos;


        if (IsKeyPressed(KEY_T)) {
            isDarkMode = !isDarkMode;
            // Applica tema colori
            ApplyTheme(&currentTheme, isDarkMode);
            // aggiorna grafico perche viene disegnato solo quando viene caricato file .fit 
            drawChart(targetChart);
        }
        if (btnQuit || IsKeyPressed(KEY_Q)) break;
        if (IsKeyPressed(KEY_M)) showMinimap = !showMinimap;

        // Applica zoom e pan solo se il mouse è dentro mapRect
        if (CheckCollisionPointRec(thisPos, mapRect)) {
            SetMouseCursor(3);
            if (IsKeyPressed(KEY_K) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) showKM = !showKM;
            if (IsKeyPressed(KEY_X) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) ResetCameraView(&cam, mapTexture, mapRect, 0.0f, &minZoom);
            if (IsKeyPressed(KEY_ONE)) ResetCameraView(&cam, mapTexture, mapRect, 1.0f, &minZoom);    

            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                Vector2 mouseWorldPos = GetScreenToWorld2D(thisPos, cam);
                cam.offset = thisPos;
                cam.target = mouseWorldPos;

                if (wheel > 0.0f) cam.zoom += cam.zoom * stepZoom;
                if (wheel < 0.0f) cam.zoom -= cam.zoom * stepZoom;

                if (cam.zoom < minZoom) cam.zoom = minZoom;
                if (cam.zoom > maxZoom) cam.zoom = maxZoom;
            }

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) cam.target = Vector2Add(cam.target, Vector2Scale(delta, 1.0f / cam.zoom));
        
            //-------------------------------------------------------------------
            // PAN SPRITE / ZOOM AREA with cursor KEY only if camera.zoom > 1.0f
            //-------------------------------------------------------------------
            if (cam.zoom >= minZoom)
            {
                if (UpdateKeyRepeater(&repeater, KEY_UP, deltaTime)) cam.target.y -= speed;
                if (UpdateKeyRepeater(&repeater, KEY_DOWN, deltaTime))cam.target.y += speed;
                if (UpdateKeyRepeater(&repeater, KEY_LEFT, deltaTime))cam.target.x -= speed;
                if (UpdateKeyRepeater(&repeater, KEY_RIGHT, deltaTime)) cam.target.x += speed;
            }
        } else SetMouseCursor(0); //restore os cursor visibility outside matrix

                // Controllo dei limiti mappa usando i bordi reali di mapRect sullo schermo ( da cambiare anche sotto qui)
                Vector2 topLeft = GetScreenToWorld2D((Vector2){ mapRect.x, mapRect.y }, cam);
                Vector2 bottomRight = GetScreenToWorld2D((Vector2){ mapRect.x + mapRect.width, mapRect.y + mapRect.height }, cam);

                if (topLeft.x < 0.0f) {
                    cam.target.x += (0.0f - topLeft.x);
                } else if (bottomRight.x > (float)mapTexture.width) {
                    cam.target.x -= (bottomRight.x - (float)mapTexture.width);
                }

                if (topLeft.y < 0.0f) {
                    cam.target.y += (0.0f - topLeft.y);
                } else if (bottomRight.y > (float)mapTexture.height) {
                    cam.target.y -= (bottomRight.y - (float)mapTexture.height);
                }

                // Ricalcolo coordinate reali per sincronizzare il rettangolo di evidenziazione della mappa ( come sopra)
                topLeft = GetScreenToWorld2D((Vector2){ mapRect.x, mapRect.y }, cam);
                bottomRight = GetScreenToWorld2D((Vector2){ mapRect.x + mapRect.width, mapRect.y + mapRect.height }, cam);

                //---------------------------------------------------------------------
                //  file LIbrary management
                //----------------------------------------------------------------------
                // Scroll con tastiera per spostarsi tra i files (solo se si è con il mouse sulla zona della libreria)
                 if (CheckCollisionPointRec(thisPos, filesRect)) {
                    if (filteredCount >= visibleRows) 
                    {
                        selectedIndex  = -(int)GetMouseWheelMove() + selectedIndex;  

                        if (IsKeyPressed(KEY_HOME)) selectedIndex = 0;
                        if (IsKeyPressed(KEY_END)) selectedIndex = filteredCount -1;

                        // tasti con il repeat rate attivo -----------------------------
                        if (UpdateKeyRepeater(&repeater, KEY_DOWN, deltaTime)) selectedIndex++;
                        if (UpdateKeyRepeater(&repeater, KEY_UP, deltaTime)) selectedIndex--;
                        if (UpdateKeyRepeater(&repeater, KEY_PAGE_DOWN, deltaTime)) selectedIndex += 9;
                        if (UpdateKeyRepeater(&repeater, KEY_PAGE_UP, deltaTime)) selectedIndex -= 9;
                        // -------------------------------------------------------------
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {

                            if (filteredCount > 0 && selectedIndex >= 0 && selectedIndex < filteredCount) {
                                int realIndex = filteredIndices[selectedIndex];
                                strcpy(fName,allFiles[realIndex].path);

                                // decodifica il file .FIT
                                decodeFit (fName);
                                // disegna grafico altimetria
                                selectedPointIdx = -1;
                                drawChart(targetChart);
                                isTrackLoaded = true;
                                
                                // Array di supporto per salvare i pixel convertiti
                                puntiPixel[numPointRec];
                                for (int i = 0; i < numPointRec; i++) puntiPixel[i] = GpsToPixel(puntiGps[i].lat, puntiGps[i].lon, mapTexture);
                            }
                        }
                            // checks
                            if (selectedIndex < 0) selectedIndex=0;
                            if (selectedIndex > filteredCount -1) selectedIndex = filteredCount-1;
                    }
                }

    // color GuyStyle e setting mese / anno correnti
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

        // Salva i valori precedenti per capire se l'utente ha cambiato filtro
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
    		    BeginMode2D(cam);
                    DrawTexture(mapTexture,0,0, WHITE);

                    if (isTrackLoaded) {
                        float spessoreLinea = 4.0f / cam.zoom; 
                        float diametroPunto = 6.0f / cam.zoom;
                        if (spessoreLinea < 1.0f) spessoreLinea = 1.0f;
                        if (diametroPunto < 3.0f) diametroPunto = 3.0f;

                        // prima disegna traccia e dopo le label : due for separati per evitare glitch grafici
                        for (int i = 1; i < numPointRec - 1; i++) DrawLineEx(puntiPixel[i], puntiPixel[i + 1], spessoreLinea, currentTheme.accentColor );
                        
                            // punto sulla traccia?
                            DrawCircleV(puntiPixel[selectedPointIdx], diametroPunto, isDarkMode?BLUE:RED);
                            DrawCircleLinesV(puntiPixel[selectedPointIdx], diametroPunto, currentTheme.borderColor);
                            
                        if (showKM) {
                        for (int i = 0; i < numPointRec - 1; i+=40) {
                            // disegna label KM : KEY_K
                            snprintf(tempBuffer, sizeof(tempBuffer), " %.f ", puntiGps[i].dist);
                            Vector2 labelPos = MeasureTextEx(textFont, tempBuffer, textFont.baseSize, 0);
                            DrawRectangleRounded((Rectangle){puntiPixel[i].x,puntiPixel[i].y ,labelPos.x,labelPos.y},0.24f,4,Fade(currentTheme.bgColor, 0.90f));
                            DrawRectangleRoundedLinesEx((Rectangle){puntiPixel[i].x,puntiPixel[i].y, labelPos.x,labelPos.y},0.24f,4,1.0f,currentTheme.borderColor);
                            DrawTextEx(textFont, tempBuffer, puntiPixel[i], (float)textFont.baseSize, 0,currentTheme.fgColor);
                            }
                        }



                    // bandierine
                    Vector2 cityOrigin = { (cityIcon.width - 8) , (cityIcon.height -1 )  };
                    Vector2 startPos = puntiPixel[1];
                    Vector2 endPos = puntiPixel[numPointRec-1];
                    DrawTexturePro(cityIcon, (Rectangle){ 0, 0, cityIcon.width, cityIcon.height }, (Rectangle){ startPos.x, startPos.y, 16, 16 },  cityOrigin, 0.0f, GREEN);
                    DrawTexturePro(cityIcon, (Rectangle){ 0, 0, cityIcon.width, cityIcon.height }, (Rectangle){ endPos.x, endPos.y, 16, 16 },  cityOrigin, 0.0f, BLUE);
                 }

                EndMode2D();
		    EndScissorMode();	      

            // bordo attorno alla mappa 
            DrawRectangleLinesEx(mapRect, 1,currentTheme.gridColor);



            // -----------------------------------------------------------------
            // informazioni traccia
            //------------------------------------------------------------------
            DrawRectangleRec(datatRect, currentTheme.gridBgColor);
            DrawRectangleLinesEx(datatRect, 1,currentTheme.gridColor);
            DrawLine(datatRect.x, datatRect.y +44 ,datatRect.x + datatRect.width,datatRect.y +44,currentTheme.gridColor);

            if (isTrackLoaded) {

                // dividi la stringa con separatore ogni stringa separata diventa elemento di un array..
                int count=0;
                char **splitStr = TextSplit(infoTraccia.data_inizio, '/' , &count); 
                Vector2 headerPos = MeasureTextEx(titleFont, splitStr[0], titleFont.baseSize, 0);
                DrawTextEx(titleFont,TextFormat("%s", splitStr[0]) ,(Vector2){datatRect.x + (datatRect.width - headerPos.x)/2, datatRect.y+2},(float)titleFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Ora inizio attività:",(Vector2){datatRect.x+8,datatRect.y +48},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",splitStr[1]) ,(Vector2){datatRect.x + 204, datatRect.y+48},(float)boldFont.baseSize,0,currentTheme.fgColor);

                splitStr = TextSplit(infoTraccia.data_fine, '/' , &count); 
                DrawTextEx(textFont,"Ora fine attività:",(Vector2){datatRect.x+8,datatRect.y + 64},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",splitStr[1]) ,(Vector2){datatRect.x + 204, datatRect.y + 64},(float)boldFont.baseSize,0,currentTheme.fgColor);

                getTimeFromMinute(infoTraccia.minMov);
                DrawTextEx(textFont,"Tempo in movimento:",(Vector2){datatRect.x+8,datatRect.y + 80},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",tempBuffer) ,(Vector2){datatRect.x + 208, datatRect.y + 80},(float)boldFont.baseSize,0,currentTheme.fgColor);

                getTimeFromMinute(infoTraccia.minTot);
                DrawTextEx(textFont,"Tempo totale sessione:",(Vector2){datatRect.x+8,datatRect.y + 96},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",tempBuffer) ,(Vector2){datatRect.x + 208, datatRect.y + 96},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Distanza totale:",(Vector2){datatRect.x+8,datatRect.y + 112},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.2f km",infoTraccia.disTot) ,(Vector2){datatRect.x + 208, datatRect.y + 112},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Velocità media:",(Vector2){datatRect.x+8,datatRect.y + 128},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f km/h",infoTraccia.avgSpd) ,(Vector2){datatRect.x + 208, datatRect.y + 128},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Velocità massima:",(Vector2){datatRect.x+8,datatRect.y + 144},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f km/h",infoTraccia.maxSpd) ,(Vector2){datatRect.x + 208, datatRect.y + 144},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Dislivello positivo (salita):",(Vector2){datatRect.x+8,datatRect.y + 160},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f metri",infoTraccia.altPos) ,(Vector2){datatRect.x + 208, datatRect.y + 160},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Dislivello negativo (discesa):",(Vector2){datatRect.x+8,datatRect.y + 176},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f metri",infoTraccia.altNeg) ,(Vector2){datatRect.x + 208, datatRect.y + 176},(float)boldFont.baseSize,0,currentTheme.fgColor);
                
                DrawTextEx(textFont,"Calorie consumate:",(Vector2){datatRect.x+8,datatRect.y + 192},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f kcal",infoTraccia.calorie) ,(Vector2){datatRect.x + 208, datatRect.y + 192},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Temperatura media:",(Vector2){datatRect.x+8,datatRect.y + 208},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f° C.",infoTraccia.avgTemp) ,(Vector2){datatRect.x + 208, datatRect.y + 208},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Temperatura massima:",(Vector2){datatRect.x+8,datatRect.y + 224},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.f° C.",infoTraccia.maxTemp) ,(Vector2){datatRect.x + 208, datatRect.y + 224},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Altitudine di partenza:",(Vector2){datatRect.x+8,datatRect.y + 240},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f metri",puntiGps[0].alt) ,(Vector2){datatRect.x + 208, datatRect.y + 240},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Altitudine media:",(Vector2){datatRect.x+8,datatRect.y + 256},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f metri",infoTraccia.altAvg) ,(Vector2){datatRect.x + 208, datatRect.y + 256},(float)boldFont.baseSize,0,currentTheme.fgColor);

                DrawTextEx(textFont,"Altitudine massima:",(Vector2){datatRect.x+8,datatRect.y + 272},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%.1f metri",infoTraccia.altMax) ,(Vector2){datatRect.x + 208, datatRect.y + 272},(float)boldFont.baseSize,0,currentTheme.fgColor);

            } else DrawTextEx(subTitleFont,"Seleziona un file dalla lista...",(Vector2){datatRect.x + 16, (datatRect.y + 8)},(float)subTitleFont.baseSize,0,currentTheme.fgColor);


            // -----------------------------------------------------------------
            // Grafico altimetria
            //------------------------------------------------------------------

            DrawRectangleRec(chartRect,currentTheme.gridBgColor);
            DrawRectangleLinesEx(chartRect, 1,currentTheme.gridColor);

            BeginScissorMode(chartRect.x, chartRect.y, chartRect.width, chartRect.height);
                if (isTrackLoaded) {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)targetChart.texture.width, - (float)targetChart.texture.height };
                    // disegna texture graafico alt / spped
                    DrawTextureRec(targetChart.texture, sourceRec, (Vector2){ chartRect.x, chartRect.y }, WHITE);
                     // tooltip interattivo in tempo reale
                    drawChartTooltip();
                } else DrawTextEx(subTitleFont,"Seleziona un file dalla lista...",(Vector2){chartRect.x + 240, (chartRect.y +88)},(float)subTitleFont.baseSize,0,currentTheme.fgColor);
            EndScissorMode();

            // -----------------------------------------------------------------
            // filtro x files list
            //----------------------------------------------------------------

            // la ricerca va fatta una sola volta
            GuiCheckBox((Rectangle){ 8, 337, 18, 18 }, "Filtro ", &filterEnabled);
            if (!filterEnabled) {
                 selectedMonth = currentMonth;
                 selectedYear = currentYear;
                 GuiDisable();
            }

            GuiSpinner((Rectangle){ 134, 335, 72, 22 }, "Mese ", &selectedMonth, 0, 12, 0);
            GuiSpinner((Rectangle){ 252, 335, 96, 22 }, "Anno ", &selectedYear, 2020, 2040, 0);
            GuiEnable();            

            // Se un controllo è cambiato, ricalcola gli indici filtrati (istantaneo!)
            if (prevFilter != filterEnabled || prevYear != selectedYear || prevMonth != selectedMonth) {
                applyDateFilter(filterEnabled, selectedYear, selectedMonth);
            // setta variabile di visualizzazione libreria in base al numero di files presenti...
                selectedIndex = 0;
                if (filteredCount<maxRows) visibleRows = filteredCount;
                else visibleRows=maxRows;
                centerRow = visibleRows / 2;
            }

            //  list files filtrati ---
            for (int i = 0; i < filteredCount; i++) {
                int realIndex = filteredIndices[i];
                displayNames[i] = allFiles[realIndex].path;
            }


            // -----------------------------------------------------------------
            //  file list / library 
            //------------------------------------------------------------------q
            DrawRectangleRec(filesRect,currentTheme.gridBgColor);
            DrawRectangleLinesEx(filesRect,1,currentTheme.gridColor);

                scrollOffset = selectedIndex - centerRow;
                if (scrollOffset < 0) scrollOffset = 0;
                int maxOffset = filteredCount - visibleRows;
                if (maxOffset < 0) maxOffset = 0;
                if (scrollOffset > maxOffset) scrollOffset = maxOffset;

                    for (int i = 0; i < visibleRows; ++i) {

                        if (i!=0) DrawLine(filesRect.x+1, filesRect.y + (1+i*rowHeight), filesRect.x + filesRect.width-1, filesRect.y +(1+i*rowHeight),currentTheme.gridColor);

                        int fileIndex = scrollOffset + i;
                        if (fileIndex > filteredCount) break;

                        // even/odd row background
                        if (i % 2) DrawRectangleRec((Rectangle){filesRect.x+1,filesRect.y +(i*rowHeight)+1,filesRect.width-2,rowHeight-1}, Fade(currentTheme.bgColor, 0.32f));

                        if (fileIndex == selectedIndex) DrawRectangle(filesRect.x+1,filesRect.y +(i*rowHeight) +1 ,filesRect.width-2,rowHeight-1, currentTheme.accentColor);
                            DrawTextEx(boldFont,TextFormat("%05i",fileIndex + 1),(Vector2){filesRect.x + 6, filesRect.y +(i*rowHeight)+3},(float)boldFont.baseSize,-1,(fileIndex == selectedIndex)?GetReadableTextColor(currentTheme.accentColor):currentTheme.fgColor);

                            // print truncate too long filenames (max. 88 chars)
                            const char *fileNameTrunc = TextSubtext(GetFileName(displayNames[fileIndex]),0,44); 
                            DrawTextEx(textFont, TextFormat("%s",fileNameTrunc),(Vector2){filesRect.x+54, filesRect.y +(i*rowHeight)+2},(float)textFont.baseSize,0,(fileIndex == selectedIndex)?GetReadableTextColor(currentTheme.accentColor):currentTheme.fgColor);
                        }   
                //vertical divider between index and filename 
                DrawLine(filesRect.x+48,filesRect.y,filesRect.x+48,filesRect.y + filesRect.height,currentTheme.gridColor);
            
            // -----------------------------------------------------------------
            //  STATUS BARS
            //------------------------------------------------------------------
            char infoText[256];
            if (filterEnabled) snprintf(infoText, sizeof(infoText), "Filtro attivo: %s %04d, %d files presenti.", mesi[selectedMonth], selectedYear, filteredCount);
            else snprintf(infoText, sizeof(infoText), "Filtro non attivo! %d files presenti.", filteredCount);
            GuiStatusBar((Rectangle){ 8, screenHeight-28, filesRect.width, 22 }, infoText);

            DrawText(TextFormat("%s v%s", TOOL_NAME, TOOL_VERSION), 40, 14,10,currentTheme.fgColor); 
            // //DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);
            snprintf(infoText, sizeof(infoText), "[ Selected file: %s ] [ Map zoom: %02.f%% ]",displayNames[selectedIndex], cam.zoom*100.0f);
            GuiStatusBar((Rectangle){ chartRect.x, screenHeight-28, chartRect.width, 22 }, infoText);

 
            // -----------------------------------------------------------------
            //  MINIMAP
            //------------------------------------------------------------------
            if (showMinimap) {
                // bordo minimap
                DrawRectangleRec((Rectangle){miniMapX - 1, miniMapY - 1, miniMapWidth + 2, miniMapHeight + 2}, currentTheme.borderColor);
                
                // Disegno della mini-map
                DrawTexturePro(
                    mapTexture, 
                    (Rectangle){ 0, 0, mapTexture.width, mapTexture.height }, 
                    (Rectangle){ miniMapX, miniMapY, miniMapWidth, miniMapHeight },
                    (Vector2){ 0, 0 }, 0.0f, WHITE
                );

                // Coordinate e sizing del rettangolo di evidenziazione sulla minimap
                float rectX = miniMapX + (topLeft.x * scaleX);
                float rectY = miniMapY + (topLeft.y * scaleY);
                float rectW = (bottomRight.x - topLeft.x) * scaleX;
                float rectH = (bottomRight.y - topLeft.y) * scaleY;

                // Disegno del rettangolo di evidenziazione (con limiti visivi per non uscire dalla mini-mappa)
                DrawRectangleRec((Rectangle){rectX, rectY, rectW, rectH}, Fade(currentTheme.accentColor, 0.25f));
                DrawRectangleLinesEx((Rectangle){ rectX, rectY, rectW, rectH }, 1, currentTheme.accentColor);
                }
            
         btnQuit = GuiButton((Rectangle){8,8,20,20}, "#128#");
        EndDrawing();
	}

    UnloadRenderTexture(targetChart);
	UnloadTexture(mapTexture);
    UnloadTexture(cityIcon);
    UnloadRenderTexture(target);
    UnloadFont(boldFont);
    UnloadFont(textFont);
    UnloadFont(titleFont);
    UnloadFont(subTitleFont);
	CloseWindow();
    return 0;
}


// Resetta lo zoom al valore targetZoom e centra la mappa dentro area
void ResetCameraView(Camera2D *cam, Texture2D mapTexture, Rectangle area, float targetZoom, float *outMinZoom)
{
    if (cam == NULL || mapTexture.width <= 0 || mapTexture.height <= 0) return;

    float minZoomX = area.width / (float)mapTexture.width;
    float minZoomY = area.height / (float)mapTexture.height;
    float minZoom = (minZoomX > minZoomY) ? minZoomX : minZoomY;

    if (outMinZoom != NULL) *outMinZoom = minZoom;

    // Se passi un valore <= 0.0f, usa automaticamente minZoom come fallback
    cam->zoom = (targetZoom > 0.0f) ? targetZoom : minZoom;
    cam->rotation = 0.0f;

    // Allinea il centro dell'area sullo schermo con il centro della texture
    cam->offset = (Vector2){ area.x + area.width * 0.5f, area.y + area.height * 0.5f };
    cam->target = (Vector2){ mapTexture.width * 0.5f, mapTexture.height * 0.5f };
}

void calcMinimap (Texture2D mapTexture) {

    // dimensione minimap 
    miniMapWidth = maxMapSize;
    miniMapHeight = maxMapSize;

    //  aspect ratio (Es: 4000.0 / 3000.0 = 1.333)
    aspectRatio = (float)mapTexture.width / (float)mapTexture.height;

    if (aspectRatio >= 1.0f) {
        // Immagine Orizzontale (Landscape): larghezza massima, altezza ridotta proporzionalmente
        miniMapHeight = maxMapSize / aspectRatio;
    } else {
        // Immagine Verticale (Portrait) o Quadrata: altezza massima, larghezza aumentata proporzionalmente
        miniMapWidth = maxMapSize * aspectRatio;
    }

    // posizione nell'angolo in basso a destra 
    miniMapX = (mapRect.x + mapRect.width) - miniMapWidth - 4.0f;   
    miniMapY = mapRect.y + 4.0f; 
    
    // Fattore di scala 
    scaleX = miniMapWidth / mapTexture.width;
    scaleY = miniMapHeight / mapTexture.height;
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

// Funzione di conversione (Usa i valori reali della tua mappa di riferimento!)
Vector2 GpsToPixel(double lat, double lon, Texture2D tex) {

    //1. QGIS / web services google / export map draw on canvas/ dpi 300 o regola / save tiff / spunta salva dati georeferenziati con il file
    //2. gdalinfo file.tiff e si vedono i limiti lat&lon
    // se si cambia mappa rimettere limiti corretti..file tif lavorato con QGIS
    // I valori forniti contengono già le coordinate geografiche espresse in gradi, minuti e secondi (DMS) 
    //vanno convertiti nel formato standard Gradi Decimali (LAT / LON)

    // Bounding box geografico (WGS84 / EPSG:4326)
    const double minLon = 10.46546111; // Ovest (Left)
    const double maxLon = 11.36574722; // Est (Right)
    const double minLat = 45.15988611; // Sud (Bottom)// se la traccia e' un pò sfalsata aumenta di poco questo valore tipo 45.1571xx >> 45.1591xx
    const double maxLat = 45.91895000; // Nord (Top) // se la traccia e' un pò sfalsata aumenta di poco questo valore tipo 45.9179xx >> 45.9189xx

    // const double minLon = 10.426533; // Ovest (Left)
    // const double maxLon = 11.353342; // Est (Right)
    // const double minLat = 45.208650; // Sud (Bottom)// se la traccia e' un pò sfalsata aumenta di poco questo valore tipo 45.1571xx >> 45.1591xx
    // const double maxLat = 45.921619; // Nord (Top) // se la traccia e' un pò sfalsata aumenta di poco questo valore tipo 45.9179xx >> 45.9189xx


    double normX = (lon - minLon) / (maxLon - minLon);
    double normY = (lat - minLat) /(maxLat - minLat);

    Vector2 pixelPos = { 0 };
    pixelPos.x = (float)( normX * tex.width);
    pixelPos.y = (float)((1.0 - normY) * tex.height);
    return pixelPos;
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
   FIT_UINT32 mesg_index = 0;
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
    char data_ora[64];
    double lat;
    double lon;
    double alt;
    double dist;
    double vel;

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
                        //  qui posso cambiare ogni quanti secondi "registrare" un record 60 = 1 min , 120 = 2 minuti etc etc
                        // registra 4 record al minuto invece di 60...(la cpu e gnome ringraziano!!!)
                        
                            if (numRecord % 12 == 0) { 
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
                            if (session->avg_temperature != FIT_UINT16_INVALID) infoTraccia.avgTemp = session->avg_temperature;
                            if (session->max_temperature != FIT_UINT16_INVALID) infoTraccia.maxTemp = session->max_temperature;
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
      printf("Record totali: %d, record registrati: %d\n",numRecord, numPointRec );
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
        centerRow = visibleRows / 2;

}

int compareFiles(const void *a, const void *b) {
    const char *path_a = (const char *)a;
    const char *path_b = (const char *)b;

    // estrapola nome del file dalla path completo, usando raylib
    const char *filename_a = GetFileName(path_a);
    const char *filename_b = GetFileName(path_b);
    
    // Confronto case sensitive...A <> a
    //return strcmp(filename_a, filename_b);
    
    // Confronto case insensitive A = a
    return strcasecmp(filename_a, filename_b);
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

        // disegnao velocità
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

            DrawLineEx((Vector2){ x1, y1 }, (Vector2){ x2, y2 }, 2.0f, Fade(isDarkMode?SKYBLUE:MAROON,0.72f));

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
        if (idx > 0 && fabsf(puntiGps[idx - 1].dist - targetDist) < fabsf(puntiGps[idx].dist - targetDist)) {
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
