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
#define TOOL_VERSION            "0.6.0"

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
const int screenWidth = 1104;
const int screenHeight = 736;

Rectangle mapArea = { 8,8,540,720};
Rectangle dataArea = { 548,8,540,360};
Rectangle libraryPos = { 556,370,540,358};

const int fntSize = 16;
char buffer[64];
bool showMinimap = false;

float minZoomX;
float minZoomY;
float minZoom;
const float stepZoom = 0.20f;
const float maxZoom = 5.0f;


//mimimap variables
const float maxMapSize = 196.0f; 
float miniMapWidth;
float miniMapHeight;
float aspectRatio;
float miniMapX ;
float miniMapY ;
float scaleX;
float scaleY;

// ARDUINO Matrix tool colors (light)
#define FG_COLOR CLITERAL(Color){ 55, 65, 70, 255}
#define BG_COLOR CLITERAL(Color){ 218, 227, 227, 255} 
// grid and checkerboard
#define GRID_COLOR CLITERAL(Color){ 155, 160, 163, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
// some funs
#define TRACK_COLOR CLITERAL(Color){ 247, 120, 16, 255}
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}
#define BORDER_COLOR CLITERAL(Color){ 20, 20,25, 255} 

// Key repeat structure
typedef struct {
    int currentKey;      // Il tasto attualmente premuto
    float timer;         // Timer per calcolare i delay
    bool isRepeating;    // true se ha superato il delay iniziale
    float firstDelay;    // Tempo di attesa iniziale 
    float repeatRate;    // Intervallo di ripetizione 
} KeyRepeater;

// Struttura per le coordinate geografiche reali
typedef struct {
    int record;
    char* time;
    double lat;
    double lon;
    double alt;
    double dist;
    double speed;
} tracciaGps;

size_t numRecord;
size_t numPointRec;
Vector2 puntiPixel[4096];
tracciaGps puntiGps[32768];

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
int maxRows = 17;
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

void loadLibrary (void);
int compareFiles(const void *a, const void *b);
int loadFilesRecursive(const char *path, char files[MAX_FILES][MAX_NAME], int count);

int main (int argc, char *argv[])
{
	 // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED ); //|  FLAG_MSAA_4X_HINT); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "fitview");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    Font textFont = LoadFontEx("fonts/PixelOperator.ttf", fntSize,NULL, 0); // all other text
    Font BoldFont = LoadFontEx("fonts/PixelOperator-Bold.ttf", fntSize,NULL, 0); // all other text
    
      // Caricamento texture (reale o di test)
     // fonte immagine definitiva 
     // https://www.esa.int/media/earth-from-space-olympic-view-en-378359
    Texture2D mapTexture;
    if (FileExists("assets/map.png")) {
        mapTexture = LoadTexture("assets/map.png");
    } else {
        Image img = GenImageChecked(mapArea.width, mapArea.height, 12, 12, WHITE, RAYWHITE);
        mapTexture = LoadTextureFromImage(img);
        UnloadImage(img);
    }

	RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // setup initial camera settings
    Camera2D cam = { 0 };
    //float minZoom = 1.0f;
    ResetCameraView(&cam, mapTexture, mapArea, 0.0f, &minZoom);

// load file list and set file library variables
    loadLibrary();

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


        if (IsKeyPressed(KEY_M)) showMinimap = !showMinimap;

        if (IsKeyPressed(KEY_X)) ResetCameraView(&cam, mapTexture, mapArea, 0.0f, &minZoom);
        
        if (IsKeyPressed(KEY_ONE)) ResetCameraView(&cam, mapTexture, mapArea, 1.0f, &minZoom);    
                      
        // Applica zoom e pan solo se il mouse è dentro mapArea
        if (CheckCollisionPointRec(thisPos, mapArea)) {
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

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                cam.target = Vector2Add(cam.target, Vector2Scale(delta, 1.0f / cam.zoom));
            }
        
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
        }


        // Controllo dei limiti usando i bordi reali di mapArea sullo schermo
        Vector2 topLeft = GetScreenToWorld2D((Vector2){ mapArea.x, mapArea.y }, cam);
        Vector2 bottomRight = GetScreenToWorld2D((Vector2){ mapArea.x + mapArea.width, mapArea.y + mapArea.height }, cam);

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


                //---------------------------------------------------------------------
                //  file LIbrary management
                //----------------------------------------------------------------------
                // Scroll con tastiera per sopstarsi tra i files (solo se si è con il mouse sulla zona della libreria)
                 if (CheckCollisionPointRec(thisPos, libraryPos)) {
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
                                decodeFit (fName);

                                    printf("%s\n",fName );
                                    printf("Record totali: %d, record registrati: %d\n",numRecord, numPointRec );
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
            ClearBackground(BG_COLOR);
        EndTextureMode();
        
		BeginDrawing();
            ClearBackground(BG_COLOR);
            DrawRectangleRec((Rectangle){mapArea.x - 1, mapArea.y - 1, mapArea.width + 2, mapArea.height + 2}, BORDER_COLOR);

            BeginScissorMode(mapArea.x, mapArea.y, mapArea.width, mapArea.height);
    		    BeginMode2D(cam);
                    DrawTexture(mapTexture,0,0, WHITE);

                    if (isTrackLoaded) {
                        // 2. Disegno le linee rosse di unione tra i punti
                        // Usiamo uno spessore proporzionale allo zoom per non farla sparire o diventare enorme
                        float spessoreLinea = 4.0f / cam.zoom; 
                        if (spessoreLinea < 1.0f) spessoreLinea = 1.0f;

                        for (int i = 1; i < numPointRec - 1; i++) {
                            DrawLineEx(puntiPixel[i], puntiPixel[i + 1], spessoreLinea, MAROON);
                        }

                        // // 3. Disegno i nodi/punti del percorso
                        // float raggioPunto = 2.0f / cam.zoom;
                        // if (raggioPunto < 2.0f) raggioPunto = 2.0f;

                        // for (int i = 1; i < numPointRec; i++) {
                        //     DrawCircleV(puntiPixel[i], raggioPunto, Fade (YELLOW,0.50f));
                        // }
                    }

                EndMode2D();
		    EndScissorMode();	      


            snprintf(buffer, sizeof(buffer), "Zoom: %02.f%%", cam.zoom*100);
            DrawTextEx(textFont,buffer,(Vector2){screenWidth-100, 24},(float)textFont.baseSize,0,BLACK);


        // -----------------------------------------------------------------
        //  print file list / library 
        //------------------------------------------------------------------
        DrawRectangleRec(libraryPos,GRID_BG_COLOR);
        DrawRectangleLinesEx(libraryPos,1,GRID_COLOR);

            scrollOffset = selectedIndex - centerRow;
            if (scrollOffset < 0) scrollOffset = 0;
            int maxOffset = fileCount - visibleRows;
            if (maxOffset < 0) maxOffset = 0;
            if (scrollOffset > maxOffset) scrollOffset = maxOffset;

                for (int i = 0; i < visibleRows; ++i) {

                    if (i!=0) DrawLine(libraryPos.x+1, libraryPos.y + (1+i*rowHeight), libraryPos.x + libraryPos.width-1, libraryPos.y +(1+i*rowHeight),GRID_COLOR);

                    int fileIndex = scrollOffset + i;
                    if (fileIndex > fileCount) break;

                    if (fileIndex == selectedIndex) DrawRectangle(libraryPos.x+1,libraryPos.y +(i*rowHeight) +1 ,libraryPos.width-2,rowHeight-1, ON_COLOR);
                        DrawText(TextFormat("%04i",fileIndex + 1),libraryPos.x + 6, libraryPos.y +(i*rowHeight)+6,10,FG_COLOR);
                        // print truncate too long filenames (max. 88 chars)
                        const char *fileNameTrunc = TextSubtext(files[fileIndex],0,124); 
                        DrawText(TextFormat("%s",fileNameTrunc), libraryPos.x+38, libraryPos.y +(i*rowHeight)+6,10,BLACK);
                    
                    }   
            //vertical divider between index and filename 
            DrawLine(libraryPos.x+34,libraryPos.y,libraryPos.x+34,libraryPos.y + libraryPos.height,GRID_COLOR);
        



            // Minimap show/hide with M key ------------------------------------
            if (showMinimap) {
                // bordo minimap
                DrawRectangleRec((Rectangle){miniMapX - 1, miniMapY - 1, miniMapWidth + 2, miniMapHeight + 2}, BORDER_COLOR);
                
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
                DrawRectangle(rectX, rectY, rectW, rectH, Fade(SKYBLUE, 0.25f));
                DrawRectangleLinesEx((Rectangle){ rectX, rectY, rectW, rectH }, 1, SKYBLUE);
            }
            //------------------------------------------------------------------

        EndDrawing();
	}

    UnloadRenderTexture(target);
	UnloadTexture(mapTexture);
    UnloadFont(BoldFont);
    UnloadFont(textFont);
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

    // posizione in alto a sx
    miniMapX = 12; 
    miniMapY = 12; 

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
    const double minLat = 45.15818611; // Sud (Bottom)
    const double maxLat = 45.91795000; // Nord (Top)

    Vector2 pixelPos = { 0 };
    pixelPos.x = ((lon - minLon) / (maxLon - minLon)) * tex.width;
    pixelPos.y = (1.0f - ((lat - minLat) / (maxLat - minLat))) * tex.height;
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
    double altimetro;
    double dist_km;
    double vel_kmh;

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

                        // registra 6 record al minuti invece di 60...(la cpu e gnome ringraziano!!!)
                            if (numRecord % 15 == 0) { 
                                 const FIT_RECORD_MESG *record = (FIT_RECORD_MESG *) mesg;
                                 // 1. Data e Ora Leggibile
                                  ConvertFitTimestamp(record->timestamp, data_ora, sizeof(data_ora));

                                // 2 & 3. Latitudine e Longitudine
                                  if (record->position_lat != FIT_SINT32_INVALID && record->position_long != FIT_SINT32_INVALID) {
                                      lat = (double)record->position_lat * SEMICIRCLES_TO_DEGREES;
                                      lon = (double)record->position_long * SEMICIRCLES_TO_DEGREES;
                                  }
                                // 4. Altitudine
                                if (record->altitude != FIT_UINT16_INVALID) altimetro = ((double)record->altitude / 5.0) - 500.0;
                                
                                // 5. Distanza percorsa accumulata
                                if (record->distance != FIT_UINT32_INVALID) dist_km = (double)record->distance / 100.0 / 1000.0;
                                
                                // 6. Velocità istantanea
                                if (record->enhanced_speed != FIT_UINT32_INVALID) vel_kmh = ((double)record->enhanced_speed/ 1000.0) * 3.6;
                                else if (record->speed != FIT_UINT16_INVALID) vel_kmh = ((double)record->speed / 1000.0) * 3.6;

                                puntiGps[numPointRec] =(tracciaGps) {numRecord, data_ora, lat, lon, altimetro, dist_km, vel_kmh };
                                numPointRec++;
                            }                            
                            numRecord++;
                            break;
                      }

                      case FIT_MESG_NUM_SESSION:
                      {
                         const FIT_SESSION_MESG *session = (FIT_SESSION_MESG *) mesg;
                         printf("\n");

                              char data_inizio[64];
                                  ConvertFitTimestamp(session->start_time, data_inizio, sizeof(data_inizio));
                                  printf("Inizio sessione              : %s\n", data_inizio);
                              char data_fine[64];
                                  ConvertFitTimestamp(session->timestamp, data_fine, sizeof(data_fine));
                                  printf("Fine sessione                : %s\n", data_fine);
                              if (session->total_elapsed_time != FIT_UINT32_INVALID) {
                                  double min = (double)session->total_elapsed_time / 1000.0 / 60.0;
                                  printf("Tempo totale                 : %.2f minuti\n", min);
                              }
                              if (session->total_moving_time != FIT_UINT32_INVALID) {
                                  double min = (double)session->total_moving_time / 1000.0 / 60.0;
                                  printf("Tempo in movimento           : %.2f minuti\n", min);
                              }
                              if (session->total_distance != FIT_UINT32_INVALID) {
                                  double dist_tot = (double)session->total_distance / 100.0 / 1000.0;
                                  printf("Distanza totale              : %.2f km\n", dist_tot);
                              }
                              if (session->avg_speed != FIT_UINT16_INVALID) {
                                  double avg_vel = ((double)session->avg_speed / 1000.0) * 3.6;
                                  printf("Velocità media               : %.1f km/h\n", avg_vel);
                              }
                              if (session->max_speed != FIT_UINT16_INVALID) {
                                  double max_vel = ((double)session->max_speed / 1000.0) * 3.6;
                                  printf("Velocità massima             : %.1f km/h\n", max_vel);
                              }
                              if (session->total_ascent != FIT_UINT16_INVALID) {
                                  printf("Dislivello positivo (salita) : %d metri\n", session->total_ascent);
                              }
                              if (session->total_descent != FIT_UINT16_INVALID) {
                                  printf("Dislivello negativo (discesa): %d metri\n", session->total_descent);
                              }
                              if (session->total_calories != FIT_UINT16_INVALID) {
                                  printf("Calorie bruciate             : %d kcal\n", session->total_calories);
                              }
                              if (session->avg_temperature != FIT_UINT16_INVALID) {
                                  printf("Temperatura media            : %d gradi celsius\n", session->avg_temperature);
                              }
                              if (session->max_temperature != FIT_UINT16_INVALID) {
                                  printf("Temperatura massima          : %d gradi celsius\n", session->max_temperature);
                              }
                              if (session->avg_altitude!= FIT_UINT16_INVALID) {
                                  double alt_AVG = ((double)session->avg_altitude/ 5.0) - 500.0;
                                  printf("Altitudine media             : %.1f metri\n", alt_AVG);
                              }
                               if (session->max_altitude!= FIT_UINT16_INVALID) {
                                  double alt_MAX = ((double)session->max_altitude/ 5.0) - 500.0;
                                  printf("Altitudine massima           : %.1f metri\n", alt_MAX);
                              }
                              printf("\n");
                           break;
                      }
                      default:
                         //printf("Unknown\n");
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
      printf("File converted successfully.\n");
                               
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
    qsort(files, fileCount, MAX_NAME, compareFiles);  // ordinamento file
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
