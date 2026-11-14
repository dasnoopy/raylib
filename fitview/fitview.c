/*******************************************************************************************
*
*   FIT VIEWER
*   A simple .fit decoder/viewer with map and track drawing on a map, made in C99 using raylib v6.0
*
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************/

#define TOOL_NAME               "FIT Viewer"
#define TOOL_SHORT_NAME         "fitview"
#define TOOL_VERSION            "1.4.8"

#include <raylib.h>
#include <raymath.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "fit_convert.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>

#define _CRT_SECURE_NO_WARNINGS
// Costante per convertire i semicerchi in gradi decimali
#define SEMICIRCLES_TO_DEGREES (180.0 / 2147483648.0)
// Epoca Garmin in formato timestamp Unix (31 Dicembre 1989 00:00:00 UTC)
#define GARMIN_TIME_OFFSET 631065600

// window size (16:9)
const int screenWidth = 1096;
const int screenHeight = 736;

Rectangle datatRect = { 8,8,340,295};     // top/left
Rectangle mapRect = { 354,8.0,734,520};  // top/right
Rectangle filesRect = { 8,309,340,421}; // bottom/left
Rectangle chartRect = {354,534,734,196};  // bottom/right

const int fntSize = 16;
char tempBuffer[64];
bool showMinimap = false;
bool showKM=false;

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
    .accentColor = (Color){191, 97, 106, 255}, 
    .fgColor = (Color){55, 65, 70, 255},
    .bgColor = (Color){252, 252, 252, 255},
    .borderColor = (Color){76, 86, 106, 255},
    .gridColor = (Color){180, 185, 185, 255},
    .gridBgColor = (Color){236, 241, 241, 255},
};

static const colorTheme darkTheme = {
    .accentColor = (Color){94, 129, 172, 255},
    .fgColor = (Color){216, 222, 233, 255},
    .bgColor = (Color){61, 68, 83, 255},
    .borderColor = (Color){76, 86, 106, 255},
    .gridColor = (Color){77, 87, 106, 255},
    .gridBgColor = (Color){50, 55, 66, 255},
};

bool isDarkMode = true;
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
    char* time;
    double lat;
    double lon;
    double alt;
    double dist;
    double speed;
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

// library & files management
size_t fileCount;
#define MAX_FILES 4096
#define MAX_NAME 1024
#define FILTER_EXT      ".fit"
char files[MAX_FILES][MAX_NAME];
char fName[MAX_NAME] = "";
int selectedIndex = 0;
int rowHeight = 21;
int visibleRows;
int maxRows = 20;
int centerRow;
int scrollOffset = 0;     // primo file visualizzato
bool isTrackLoaded = false;

// barra path su Linux oppure su WIN
#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

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
int loadFilesRecursive(const char *path, char files[MAX_FILES][MAX_NAME], int count);
void drawChart(RenderTexture2D target);
void ApplyTheme(colorTheme *currentTheme, bool darkMode); 
Color GetReadableTextColor(Color bgColor);

int main (int argc, char *argv[])
{
	 // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED ); //|  FLAG_MSAA_4X_HINT); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "fitview");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    
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
    Font titleFont = LoadFontEx("fonts/UnDotumBold.ttf", 40,codepoints, glyphCount); // all other text
    Font subTitleFont = LoadFontEx("fonts/UnDotum.ttf", 32,codepoints, glyphCount); // all other text

      // Caricamento texture (reale o di test)
     // fonte immagine definitiva 
     // https://www.esa.int/media/earth-from-space-olympic-view-en-378359
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
    qsort(files, fileCount, MAX_NAME, compareFilesByModTime);  // ordinamento file

// tutta la parte di carimento punti, dati della traccia selezionata
//  e' nella parte update

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

    // fai riapparire finestra dopo caricamento iniziale
    // set FPS
    SetTargetFPS(60);
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
                // Scroll con tastiera per sopstarsi tra i files (solo se si è con il mouse sulla zona della libreria)
                 if (CheckCollisionPointRec(thisPos, filesRect)) {
                    if (fileCount >= visibleRows) 
                    {
                        selectedIndex  = -(int)GetMouseWheelMove() + selectedIndex;  

                        if (IsKeyPressed(KEY_HOME)) selectedIndex = 0;
                        if (IsKeyPressed(KEY_END)) selectedIndex = fileCount -1;

                        // tasti con il repeat rate attivo -----------------------------
                        if (UpdateKeyRepeater(&repeater, KEY_DOWN, deltaTime)) selectedIndex++;
                        if (UpdateKeyRepeater(&repeater, KEY_UP, deltaTime)) selectedIndex--;
                        if (UpdateKeyRepeater(&repeater, KEY_PAGE_DOWN, deltaTime)) selectedIndex += 9;
                        if (UpdateKeyRepeater(&repeater, KEY_PAGE_UP, deltaTime)) selectedIndex -= 9;
                        // -------------------------------------------------------------
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                            if (selectedIndex >= 0 && selectedIndex < fileCount) {
                                strcpy(fName,files[selectedIndex]);

                                // decodifica il file .FIT
                                decodeFit (fName);
                                // disegna grafico altimetria
                                drawChart(targetChart);
                                isTrackLoaded = true;
                                
                                // Array di supporto per salvare i pixel convertiti
                                puntiPixel[numPointRec];
                                for (int i = 0; i < numPointRec; i++) puntiPixel[i] = GpsToPixel(puntiGps[i].lat, puntiGps[i].lon, mapTexture);
                            }
                        }
                            // checks
                            if (selectedIndex < 0) selectedIndex=0;
                            if (selectedIndex > fileCount-1) selectedIndex=fileCount-1;
                    }
                }

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
                        if (spessoreLinea < 1.0f) spessoreLinea = 1.0f;
                        for (int i = 1; i < numPointRec - 1; i++) {
                            // disegna traccia
                            DrawLineEx(puntiPixel[i], puntiPixel[i + 1], spessoreLinea, currentTheme.accentColor );

                            // disegna label KM : KEY_K
                            if (i % 40 == 0 && showKM) {
                                snprintf(tempBuffer, sizeof(tempBuffer), " %.f ", puntiGps[i].dist);
                                Vector2 labelPos = MeasureTextEx(boldFont, tempBuffer, boldFont.baseSize, 0);
                                DrawRectangleRounded((Rectangle){puntiPixel[i].x,puntiPixel[i].y+1 ,labelPos.x+2,labelPos.y-1},0.24f,24,isDarkMode?BLACK:WHITE);
                                DrawRectangleRounded((Rectangle){puntiPixel[i].x +1,puntiPixel[i].y+2, labelPos.x,labelPos.y-3},0.24f,24,currentTheme.gridBgColor);
                                DrawTextEx(boldFont, tempBuffer, puntiPixel[i], (float)boldFont.baseSize, 0,currentTheme.fgColor);
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


            // DrawText(TextFormat("%s", TOOL_SHORT_NAME), screenWidth-60,8, 10,currentTheme.fgColor); 
            // DrawText(TextFormat("v%s", TOOL_VERSION),screenWidth - 60,20, 10, currentTheme.gridColor); 
            // //DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);
            DrawText(TextFormat("Zoom: %02.f%%", cam.zoom*100),screenWidth - 68, mapRect.height-6,10,currentTheme.accentColor);

            // -----------------------------------------------------------------
            // informazioni traccia
            //------------------------------------------------------------------
            DrawRectangleRec(datatRect, currentTheme.bgColor);
            DrawRectangleRec((Rectangle){datatRect.x + 4, datatRect.y +44 ,datatRect.width-8,datatRect.height-48},currentTheme.gridBgColor);
            DrawRectangleLinesEx(datatRect, 1,currentTheme.gridColor);
            if (isTrackLoaded) {

                // dividi la stringa con separatore ogni stringa separata diventa elemento di array..
                int count=0;
                char **splitStr = TextSplit(infoTraccia.data_inizio, ',' , &count); 
                Vector2 headerPos = MeasureTextEx(titleFont, splitStr[0], titleFont.baseSize, 0);
                DrawTextEx(titleFont,TextFormat("%s",splitStr[0]) ,(Vector2){datatRect.x + (datatRect.width - headerPos.x)/2, datatRect.y + 4},(float)titleFont.baseSize,0,currentTheme.accentColor);


                DrawTextEx(textFont,"Inizio sessione:",(Vector2){datatRect.x+8,datatRect.y +48},(float)textFont.baseSize,0,currentTheme.fgColor);
                DrawTextEx(boldFont,TextFormat("%s",splitStr[1]) ,(Vector2){datatRect.x + 204, datatRect.y+48},(float)boldFont.baseSize,0,currentTheme.fgColor);

                count=0;
                splitStr = TextSplit(infoTraccia.data_fine, ',' , &count); 
                DrawTextEx(textFont,"Fine sessione:",(Vector2){datatRect.x+8,datatRect.y + 64},(float)textFont.baseSize,0,currentTheme.fgColor);
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

            } else DrawTextEx(subTitleFont,"Choose a file to display data!",(Vector2){datatRect.x + 12, (datatRect.y + 6)},(float)subTitleFont.baseSize,-1,currentTheme.accentColor);


            // -----------------------------------------------------------------
            // Grafico altimetria
            //------------------------------------------------------------------

            DrawRectangleRec(chartRect,currentTheme.gridBgColor);
            DrawRectangleLinesEx(chartRect, 1,currentTheme.gridColor);

            BeginScissorMode(chartRect.x, chartRect.y, chartRect.width, chartRect.height);
                if (isTrackLoaded) {
                    Rectangle sourceRec = { 0.0f, 0.0f, (float)targetChart.texture.width, - (float)targetChart.texture.height };
                    DrawTextureRec(targetChart.texture, sourceRec, (Vector2){ chartRect.x, chartRect.y }, WHITE);
                } else DrawTextEx(subTitleFont,"Choose a file to display data!",(Vector2){chartRect.x + 216, (chartRect.y +88)},(float)subTitleFont.baseSize,-1,currentTheme.accentColor);
            EndScissorMode();

            // -----------------------------------------------------------------
            //  file list / library 
            //------------------------------------------------------------------
            DrawRectangleRec(filesRect,currentTheme.gridBgColor);
            DrawRectangleLinesEx(filesRect,1,currentTheme.gridColor);

                scrollOffset = selectedIndex - centerRow;
                if (scrollOffset < 0) scrollOffset = 0;
                int maxOffset = fileCount - visibleRows;
                if (maxOffset < 0) maxOffset = 0;
                if (scrollOffset > maxOffset) scrollOffset = maxOffset;

                    for (int i = 0; i < visibleRows; ++i) {

                        if (i!=0) DrawLine(filesRect.x+1, filesRect.y + (1+i*rowHeight), filesRect.x + filesRect.width-1, filesRect.y +(1+i*rowHeight),currentTheme.gridColor);

                        int fileIndex = scrollOffset + i;
                        if (fileIndex > fileCount) break;

                        if (fileIndex == selectedIndex) DrawRectangle(filesRect.x+1,filesRect.y +(i*rowHeight) +1 ,filesRect.width-2,rowHeight-1, currentTheme.accentColor);
                            DrawText(TextFormat("%04i",fileIndex + 1),filesRect.x + 6, filesRect.y +(i*rowHeight)+6,10,(fileIndex == selectedIndex)?GetReadableTextColor(currentTheme.accentColor):currentTheme.fgColor);
                            // print truncate too long filenames (max. 88 chars)
                            const char *fileNameTrunc = TextSubtext(files[fileIndex],0,124); 
                            DrawText(TextFormat("%s",fileNameTrunc), filesRect.x+38, filesRect.y +(i*rowHeight)+6,10,(fileIndex == selectedIndex)?GetReadableTextColor(currentTheme.accentColor):currentTheme.fgColor);
                        }   
                //vertical divider between index and filename 
                DrawLine(filesRect.x+34,filesRect.y,filesRect.x+34,filesRect.y + filesRect.height,currentTheme.gridColor);
            

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
    const double minLat = 45.15868611; // Sud (Bottom)// se la traccia e' un pò sfalsata aumenta di poco questo valore tipo 45.1571xx >> 45.1591xx
    const double maxLat = 45.91895000; // Nord (Top) // se la traccia e' un pò sfalsata aumenta di poco questo valore tipo 45.9179xx >> 45.9189xx

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
    strftime(output_str, max_size, "%d %B %Y, %H:%M:%S", time_info);
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
                                  ConvertFitTimestamp(record->timestamp, data_ora, sizeof(data_ora));

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

                                puntiGps[numPointRec] =(gpsPoints) {numRecord, data_ora, lat, lon, alt, dist, vel };
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
    fileCount = loadFilesRecursive(".", files,0); // carica files .png
    // setta variabile di visualizzazione libreria in base al numero di files presenti...
        if (fileCount<maxRows) visibleRows = fileCount;
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


int loadFilesRecursive(const char *path, char files[MAX_FILES][MAX_NAME], int count) {
    // Controllo numero files
    if (count >= MAX_FILES) return count;

    // controllo percorso se esiste o meno...
    if (!DirectoryExists(path)) {
        TraceLog(LOG_ERROR, "Failed to locate: %s directory; exit program!", path);
        exit(1);
    }   

    DIR *dir = opendir(path);
    if (!dir) return count; //...nel caso opendir fallisca per permessi negati

    struct dirent *entry;

    // Controllo sul limite ad ogni iterazione
    while ((entry = readdir(dir)) != NULL) {
        if (count >= MAX_FILES) break; 

        const char *name = entry->d_name;

        // Salta "." e ".."
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        // MAX_NAME lunghezza massima del path+filename
        char fullpath[MAX_NAME];
        snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, SEPARATOR, name);

        struct stat st;
        if (stat(fullpath, &st) == -1) continue;

        // Se directory procedi con ricorsione
        if (S_ISDIR(st.st_mode)) {
            count = loadFilesRecursive(fullpath, files, count);
        }
        // Se è un file controlla l'estensione
        else if (S_ISREG(st.st_mode)) {
            const char *ext = strrchr(name, '.');

            if (ext && strcmp(ext, FILTER_EXT) == 0) {
                int written = snprintf(files[count], MAX_NAME, "%s", fullpath);
                if (written >= 0 && written < MAX_NAME) {
                    count++;
                }
            }
        }
    }
    closedir(dir);
    return count;
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
    float margineSinistro = 38.0f;
    float margineDestro   = 10.0f;
    float margineAlto     = 12.0f;
    float margineBasso    = 18.0f;

    float larghezzaUtile = chartRect.width - margineSinistro - margineDestro;
    float altezzaUtile   = chartRect.height - margineAlto - margineBasso;

    float distMax = (infoTraccia.disTot > 0.0f) ? infoTraccia.disTot : 1.0f;

    // 2. Calcolo altitudine con base fissa a 0.0f
    float minAlt = 0.0f;
    float rawMaxAlt = 0.0f;

    for (int i = 0; i < numPointRec; i++) {
        if (puntiGps[i].alt > rawMaxAlt) rawMaxAlt = puntiGps[i].alt;
    }

    if (rawMaxAlt < 10.0f) rawMaxAlt = 10.0f; // Altezza minima del grafico per tracce piatte

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

    // Intervallo ogni quante linee verticali stampare il testo dei km (evita sovrapposizioni)
    float labelStepKm = stepKm * 2.0f;
    if (distMaxKm <= 2.0f) labelStepKm = 0.5f;

    BeginTextureMode(target);
        ClearBackground(BLANK);

        // --- GRIGLIA ORIZZONTALE (ALTITUDINE) ---
        for (float alt = 0.0f; alt <= maxAlt + 0.1f; alt += stepAlt / 2.0f) {
            float normY = (alt - minAlt) / deltaAlt;
            float y = baseY - (normY * altezzaUtile);

            if (alt > 0.0f) {
                DrawLineDashed((Vector2){ startX, y }, (Vector2){ startX + larghezzaUtile, y }, 1, 1, currentTheme.gridColor);
            }

            // Stampa etichetta sui multipli interi di stepAlt, allineata a destra verso l'asse Y
            if (fmodf(alt, stepAlt) < 0.1f) {
                const char *labelAlt = TextFormat("%.0fm", alt);
                int textW = MeasureText(labelAlt, 10);
                DrawText(labelAlt, (int)(startX - textW - 4), (int)(y - 5), 10, currentTheme.fgColor);
            }
        }

        // --- GRIGLIA VERTICALE (DISTANZA) ---
        for (float km = 0.0f; km <= distMaxKm + 0.01f; km += stepKm) {
            float normX = km / distMaxKm;
            float x = startX + (normX * larghezzaUtile);

            if (km > 0.0f) {
                DrawLineDashed((Vector2){ x, topY }, (Vector2){ x, baseY }, 1, 1, currentTheme.gridColor);
            }

            // Stampa etichetta Km in modo proporzionale alla distanza totale
            if (fmodf(km, labelStepKm) < 0.05f) {
                const char *labelKm = (stepKm < 1.0f) ? TextFormat("%.1fkm", km) : TextFormat("%.0fkm", km);
                int textW = MeasureText(labelKm, 10);
                // Centra il testo sulla linea verticale (tranne lo 0km che parte dall'asse)
                int textX = (km == 0.0f) ? (int)x : (int)(x - textW / 2);
                DrawText(labelKm, textX, (int)(baseY + 4), 10, currentTheme.fgColor);
            }
        }

        // Bordi principali degli assi
        DrawLineV((Vector2){ startX, topY }, (Vector2){ startX, baseY }, currentTheme.borderColor);
        DrawLineV((Vector2){ startX, baseY }, (Vector2){ startX + larghezzaUtile, baseY }, currentTheme.borderColor);

        // --- DISEGNO TRACCIA ALTIMETRICA ---
        Color fillCol = Fade(currentTheme.accentColor, 0.50f);

        for (int i = 0; i < numPointRec - 1; i++) {
            float alt1 = (puntiGps[i].alt < 0.0f) ? 0.0f : puntiGps[i].alt;
            float alt2 = (puntiGps[i + 1].alt < 0.0f) ? 0.0f : puntiGps[i + 1].alt;

            float normX1 = puntiGps[i].dist / distMax;
            float normX2 = puntiGps[i + 1].dist / distMax;

            float normY1 = (alt1 - minAlt) / deltaAlt;
            float normY2 = (alt2 - minAlt) / deltaAlt;

            float x1 = startX + (normX1 * larghezzaUtile);
            float y1 = baseY - (normY1 * altezzaUtile);

            float x2 = startX + (normX2 * larghezzaUtile);
            float y2 = baseY - (normY2 * altezzaUtile);

            Vector2 tLeft  = { x1, y1 };
            Vector2 tRight = { x2, y2 };
            Vector2 bLeft  = { x1, baseY };
            Vector2 bRight = { x2, baseY };

            DrawTriangle(tLeft, bLeft, tRight, fillCol);
            DrawTriangle(tRight, bLeft, bRight, fillCol);

            DrawLineV((Vector2){ x1, y1 }, (Vector2){ x2, y2 }, currentTheme.accentColor);
        }

    EndTextureMode();
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
