/*******************************************************************************************
*
*   COLOR PICKER
*   A simple png viewer with pixel color picker, made in C99 using raylib v6.0
*
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************/

#define TOOL_NAME               "rColor Picker"
#define TOOL_SHORT_NAME         "rcolpick"
#define TOOL_VERSION            "1.6.5"

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

// window size (16:9)
const int screenWidth = 1280;
const int screenHeight = 720;

float minZoomX;
float minZoomY;
float minZoom;
const float stepZoom = 0.20f;
const float maxZoom = 24.0f;
const int txtOffset = 32;
const int fntSize = 18;
char buffer[64];
const char *clipboardText = NULL;
Color pixelCol;
Color clickCol = CLITERAL(Color){ 0, 0, 0, 255};
Color *pixels;
char fName[384] = { '\0' };
bool showGrid = false;
bool showMinimap = true;
bool showHist = true;

int histR[256] = {0};
int histG[256] = {0};
int histB[256] = {0};
int histA[256] = {0};

int maxR=1;
int maxG=1;
int maxB=1;
int maxA=1;

Vector2 barPos = {0,screenHeight-txtOffset};
//Rectangle scissorArea = { 0,0,screenWidth,screenHeight - txtOffset };
Rectangle rectPixel = { 8,8,272,152 }; // 1002 - right  / 8 left , height 208 with shaaded/tinted color
Rectangle rectHist = { 8,570,272,110 };

//mimimap variables
const float maxMapSize = 196.0f; 
float mapWidth;
float mapHeight;
float aspectRatio;
float mapPosX ;
float mapPosY ;
float scaleX;
float scaleY;

// color conversion variables
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

typedef struct hsl {
  float h, s, l;
} HSL;


 // * Converts an RGB color value to HSL. Conversion formula
 // * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 // * Assumes r, g, and b are contained in the set [0, 255] and
 // * returns HSL in the set [0, 1].

HSL rgb2hsl(float r, float g, float b) {
  
  HSL result;
  
  r /= 255;
  g /= 255;
  b /= 255;
  
  float max = MAX(MAX(r,g),b);
  float min = MIN(MIN(r,g),b);
  
  result.h = result.s = result.l = (max + min) / 2;

  if (max == min) {
    result.h = result.s = 0; // achromatic
  }
  else {
    float d = max - min;
    result.s = (result.l > 0.5) ? d / (2 - max - min) : d / (max + min);
    
    if (max == r) {
      result.h = (g - b) / d + (g < b ? 6 : 0);
    }
    else if (max == g) {
      result.h = (b - r) / d + 2;
    }
    else if (max == b) {
      result.h = (r - g) / d + 4;
    }
    
    result.h /= 6;
  }

  return result;
  
}

void drawRectangleRounded (Rectangle recSize, Color color)  
{
  float radius = 0.064f; // no radius
  int   segs   = 12; // non segments
  DrawRectangleRounded ( recSize, radius, segs, color );
}

Color darkenColor(Color color, float factor)
{
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    return (Color){
        (unsigned char)(color.r * (1 - factor)),
        (unsigned char)(color.g * (1 - factor)),
        (unsigned char)(color.b * (1 - factor)),
        color.a
    };
}

Color lightenColor(Color color, float factor)
{
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    return (Color){
        (unsigned char) ( color.r + (255-color.r) * factor ),
        (unsigned char) ( color.g + (255-color.g) * factor ),
        (unsigned char) ( color.b + (255-color.b) * factor ),
        color.a
    };
}

void getHistogram (Image image, Color *pixels)
{
       //read pixels color e populate color histogram values -----------
	        for (int i = 0; i < 256; i++) {
            histR[i]=0;
            histG[i]=0;
            histB[i]=0;
            histA[i]=0;
        }
        
        maxR = 1;
        maxG = 1;
        maxB = 1;
        maxA = 1;

       // leggi tutti i pixel dell' immagine
       for (int i = 0; i < image.width * image.height; i++)  {
            histR[pixels[i].r]++;
            histG[pixels[i].g]++;
            histB[pixels[i].b]++;
            histA[pixels[i].a]++;
        }

        // trova il valore 
        for (int i = 0; i < 256; i++) {
          if (histR[i] > maxR) maxR = histR[i];
          if (histG[i] > maxG) maxG = histG[i];
          if (histB[i] > maxB) maxB = histB[i];
          if (histA[i] > maxA) maxA = histA[i];
          }
}

void calcMinimap (Texture2D background) {

    // dimensione minimap 
    mapWidth = maxMapSize;
    mapHeight = maxMapSize;

    //  aspect ratio (Es: 4000.0 / 3000.0 = 1.333)
    aspectRatio = (float)background.width / (float)background.height;

    if (aspectRatio >= 1.0f) {
        // Immagine Orizzontale (Landscape): larghezza massima, altezza ridotta proporzionalmente
        mapHeight = maxMapSize / aspectRatio;
    } else {
        // Immagine Verticale (Portrait) o Quadrata: altezza massima, larghezza aumentata proporzionalmente
        mapWidth = maxMapSize * aspectRatio;
    }

    // posizione nell'angolo in basso a destra 
    mapPosX = screenWidth - mapWidth - 10.0f;   
    mapPosY = screenHeight - mapHeight - 40.0f; 
    
    // Fattore di scala 
    scaleX = mapWidth / background.width;
    scaleY = mapHeight / background.height;
}

int main (int argc, char *argv[])
{
    if ( (IsPathFile(argv[1])) && FileExists(argv[1]))  strcpy(fName , argv[1]);
    else {
        TraceLog(LOG_INFO, "Invalid argument (wrong path/filename? Default image will be loaded.");
        strcpy(fName,"./resources/default.png");
    }

	 // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_MSAA_4X_HINT ); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "rcolpick");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    Font txtFont = LoadFontEx("fonts/computer-says-no.otf", fntSize,NULL, 0); // all other text

    Image img = LoadImage(fName);
    Image img1 = GenImageChecked((int)img.width, (int)img.height, 8,8, WHITE, RAYWHITE);
    Texture2D background = LoadTextureFromImage(img);
    Texture2D checkerBoard = LoadTextureFromImage(img1);

	Color *pixels = LoadImageColors(img);
    //read pixels color e populate color histogram values
    getHistogram(img,pixels);
	
	RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // setup a camera
    Camera2D cam = { 0 };
    cam.target = (Vector2){ 0.0f, 0.0f };
    cam.offset = (Vector2){ 0.0f, 0.0f };
    cam.rotation = 0.0f;
    // Limite minimo di zoom per non rimpicciolire l'immagine più della finestra stessa
    minZoomX = (float)screenWidth / background.width;
    minZoomY = (float)screenHeight / background.height;
    minZoom = (minZoomX > minZoomY) ? minZoomX : minZoomY; 
    cam.zoom = minZoom;

    //generate mini-map
    calcMinimap(background);

    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);

// Main game loop
while (!WindowShouldClose())    // Detect window close button or ESC key
	{
        // Load a dropped TTF file dynamically (at current fontSize)
        if (IsFileDropped()) {
            FilePathList droppedFiles = LoadDroppedFiles();
        
         if (droppedFiles.count == 1) // Only support one file dropped
            {
                // NOTE: We only support first ttf file dropped
                if (IsFileExtension(droppedFiles.paths[0], ".png"))
                
                {
                    UnloadTexture(background);
                    UnloadTexture(checkerBoard);
                    UnloadImageColors(pixels);
                    UnloadImage(img);
                    UnloadImage(img1);
                    
                    strcpy(fName, droppedFiles.paths[0]);
                    img = LoadImage(droppedFiles.paths[0]);
                    img1 = GenImageChecked((int)img.width, (int)img.height, 8,8, WHITE, RAYWHITE);
                    background = LoadTextureFromImage(img);
                    checkerBoard = LoadTextureFromImage(img1);
                    //carica  pixels color in ram
                    pixels = LoadImageColors(img);
                    //read pixels color e populate color histogram values
                    getHistogram(img,pixels);

                    // Camera reset
                    cam.target = (Vector2){ 0.0f, 0.0f };
                    cam.offset = (Vector2){ 0.0f, 0.0f };
                    cam.rotation = 0.0f;
                    // Limite minimo di zoom per non rimpicciolire l'immagine più della finestra stessa
                    minZoomX = (float)screenWidth / background.width;
                    minZoomY = (float)screenHeight / background.height;
                    minZoom = (minZoomX > minZoomY) ? minZoomX : minZoomY; 
                    cam.zoom = minZoom;
                    // ricalcola minimap
                    calcMinimap(background);
                }
            }
            UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
        }

//----------------------------------------------------------------------------------
// Update
//----------------------------------------------------------------------------------
      if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V)) {
            if (IsImageValid(GetClipboardImage()))  {

                    UnloadTexture(background);
                    UnloadTexture(checkerBoard);
                    UnloadImageColors(pixels);
                    UnloadImage(img);
                    UnloadImage(img1);
                    
                    strcpy(fName, "(Image pasted from clipboard)");
                    img = GetClipboardImage();
                    img1 = GenImageChecked((int)img.width, (int)img.height, 8,8, WHITE, RAYWHITE);
                    background = LoadTextureFromImage(img);
                    checkerBoard = LoadTextureFromImage(img1);
                    //carica  pixels color in ram
                    pixels = LoadImageColors(img);
                    //read pixels color e populate color histogram values
                    getHistogram(img,pixels);

                    // Camera reset
                    cam.target = (Vector2){ 0.0f, 0.0f };
                    cam.offset = (Vector2){ 0.0f, 0.0f };
                    cam.rotation = 0.0f;
                    // Limite minimo di zoom per non rimpicciolire l'immagine più della finestra stessa
                    minZoomX = (float)screenWidth / background.width;
                    minZoomY = (float)screenHeight / background.height;
                    minZoom = (minZoomX > minZoomY) ? minZoomX : minZoomY; 
                    cam.zoom = minZoom;
                    // ricalcola minimap
                    calcMinimap(background);
                }
                else TraceLog(LOG_INFO, "IMAGE: Could not retrieve image from clipboard");
            }
            
        Vector2 fnameSize = MeasureTextEx(txtFont, fName, fntSize, 0);
        Vector2 mousePos = GetMousePosition();   // abilitare se cursore libero   
        Vector2 world  = GetScreenToWorld2D(GetMousePosition(), cam);

                if (IsKeyPressed(KEY_M)) showMinimap = !showMinimap;
                if (IsKeyPressed(KEY_H)) showHist = !showHist;
                
                // if (IsKeyPressed(KEY_ONE)) {
                //      // Camera reset
                //     cam.target = (Vector2){ 0.0f, 0.0f };
                //     cam.offset = (Vector2){ 0.0f, 0.0f };
                //     cam.rotation = 0.0f;
                //     cam.zoom = 1.0f;
                // }


                if (IsKeyPressed(KEY_X)) {
                     // Camera reset
                    cam.target = (Vector2){ 0.0f, 0.0f };
                    cam.offset = (Vector2){ 0.0f, 0.0f };
                    cam.rotation = 0.0f;
                    // Limite minimo di zoom per non rimpicciolire l'immagine più della finestra stessa
                    minZoomX = (float)screenWidth / background.width;
                    minZoomY = (float)screenHeight / background.height;
                    minZoom = (minZoomX > minZoomY) ? minZoomX : minZoomY; 
                    cam.zoom = minZoom;
                }

                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)) {
                          snprintf(buffer, sizeof(buffer), "%i,%i,%i,%i //RGBA color", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a);
                          SetClipboardText(buffer); // Copy text to clipboard
                          clipboardText = GetClipboardText(); // Get text from clipboard
                      }
                      
                if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_C)) {
                          snprintf(buffer, sizeof(buffer), "#%X%X%X%X //HEXA color", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a);
                          SetClipboardText(buffer); // Copy text to clipboard
                          clipboardText = GetClipboardText(); // Get text from clipboard
                      }
                      
        // --- GESTIONE ZOOM (Rotella del Mouse) ---
        float wheel = GetMouseWheelMove();
            int x = (int)world.x;
            int y = (int)world.y;
            
            // leggi colore pixel (on hove mouse color) 
            if (x >= 0 && x < background.width && y >= 0 && y < background.height) pixelCol = pixels[y * background.width + x];
                
            if (wheel != 0.0f) {
                    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), cam);
                    cam.offset = GetMousePosition();
                    cam.target = mouseWorldPos;

                    const float zoomIncrement = stepZoom;
                    if (wheel > 0) cam.zoom += cam.zoom * zoomIncrement;
                    if (wheel < 0) cam.zoom -= cam.zoom * zoomIncrement;

                    // Limite minimo di zoom per non rimpicciolire l'immagine più della finestra stessa
                    minZoomX = (float)screenWidth / background.width;
                    minZoomY = (float)screenHeight / background.height;
                    minZoom = (minZoomX > minZoomY) ? minZoomX : minZoomY; 
                    
                    if (cam.zoom < minZoom) cam.zoom = minZoom;
                    if (cam.zoom > maxZoom) cam.zoom = maxZoom;
                }

                if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    clickCol = pixelCol;
                }

                // --- GESTIONE TRASCINAMENTO (Click sinistro) ---
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    Vector2 delta = GetMouseDelta();
                    cam.target.x -= delta.x / cam.zoom;
                    cam.target.y -= delta.y / cam.zoom;
                }



                // --- CONTROLLO DEI LIMITI DELL'IMMAGINE ---
                Vector2 topLeft = GetScreenToWorld2D((Vector2){ 0, 0 }, cam);
                Vector2 bottomRight = GetScreenToWorld2D((Vector2){ (float)screenWidth, (float)screenHeight }, cam);

                if (topLeft.x < 0.0f) cam.target.x += (0.0f - topLeft.x);
                else if (bottomRight.x > (float)background.width) cam.target.x -= (bottomRight.x - (float)background.width);

                if (topLeft.y < 0.0f) cam.target.y += (0.0f - topLeft.y);
                else if (bottomRight.y > (float)background.height) cam.target.y -= (bottomRight.y - (float)background.height);

                // Ricalcolo coordinate reali per sincronizzare il rettangolo rosso della mappa
                topLeft = GetScreenToWorld2D((Vector2){ 0, 0 }, cam);
                bottomRight = GetScreenToWorld2D((Vector2){ (float)screenWidth, (float)screenHeight }, cam);

                //cursor centered
                Vector2 cursor = { x + 0.5f, y + 0.5f };
                Vector2 center = GetWorldToScreen2D(cursor, cam);
               
                // funzione di conversione HSV di raylib
                Vector3 hsv = ColorToHSV(pixelCol);
                // funzione di conversione HSL da RGB color
                HSL res = rgb2hsl(pixelCol.r, pixelCol.g, pixelCol.b);

                // modified time convert
               time_t rawtime = GetFileModTime(fName);
               struct tm  ts;
               char timeMod[80];
                // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
                ts = *localtime(&rawtime);
                strftime(timeMod, sizeof(timeMod), "%a %d-%m-%Y %H:%M:%S %Z", &ts);

//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(LIGHTGRAY);
        EndTextureMode();
        
		BeginDrawing();
            HideCursor();
            // sfondo fuori dall' immagine
            ClearBackground(LIGHTGRAY);
                  
            //BeginScissorMode((int)scissorArea.x, (int)scissorArea.y, (int)scissorArea.width, (int)scissorArea.height);   
    		    BeginMode2D(cam);
	           	// draw the entire background image for the entire world. The camera will clip it to the screen
                DrawTexture(checkerBoard, 0, 0, WHITE);
		        DrawTexture(background, 0, 0, WHITE);
		          //DrawRectangleLinesEx(screenInWorldRect, 4 / cam.zoom, Fade(BLACK,0.8f));
		        
                      //if (!showGrid) DrawRectangleLines(0,0,background.width, background.height,Fade(BLACK,0.5f)); 
		      
		      if (cam.zoom >=10.0f) {
		        showGrid=true;
                          // linee verticali
                          for (int x = 0; x <= background.width; x++) DrawLine(x, 0, x, background.height, Fade(BLACK, 0.5f));
                          // linee orizzontali
                         for (int y = 0; y <= background.height; y++) DrawLine(0, y, background.width, y, Fade(BLACK, 0.5f));
                      }
                      else showGrid = false;
          
		          EndMode2D();
		      
		      if (cam.zoom >= 10.0f) {
          	      // mirino cursore centerd
                        DrawRectangle(center.x-16, center.y, 33, 2, RED);
                        DrawRectangle(center.x, center.y-16, 2, 33, RED);
                      }
                      else  {
		       // mirino cursore libero
                        DrawRectangle(mousePos.x-16, mousePos.y, 33 , 2, RED);
                        DrawRectangle(mousePos.x, mousePos.y-16 , 2,33, RED);
                      }                      
            //EndScissorMode();	      

            
            // pixel panel ("floating")
            drawRectangleRounded (rectPixel, Fade(BLACK,0.7f));
            // cursor x,y
            DrawTextEx(txtFont,TextFormat("X,Y,Zoom: %i, %i, %02.f%%", x, y,cam.zoom*100),(Vector2){rectPixel.x + 32, rectPixel.y + 4},fntSize,0,WHITE);
            // RGBA
            DrawRectangleRounded ((Rectangle){rectPixel.x + 6 ,rectPixel.y + 25 ,260, 108},0.096f,12, pixelCol);
            DrawTextEx(txtFont,TextFormat("RGBA: %03i, %03i, %03i, %03i", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a),(Vector2){ rectPixel.x + 10, rectPixel.y + 28},fntSize,0,(pixelCol.r<=128 && pixelCol.g<=128 && pixelCol.b<=128)? WHITE : BLACK);
            // HEXA 
            DrawTextEx(txtFont,TextFormat("HEXA: #%02X%02X%02X%02X", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a),(Vector2){rectPixel.x + 10, rectPixel.y + 52},fntSize,0,(pixelCol.r<=128 && pixelCol.g<=128 && pixelCol.b<=128)? WHITE : BLACK);
           // HSVLighter variations created by adding white to your base color.
           DrawTextEx(txtFont,TextFormat("HSV: %3.02f, %3.02f%%, %3.02f%%", hsv.x,hsv.y*100.0f,hsv.z*100.0f),(Vector2){rectPixel.x + 10, rectPixel.y + 76},fntSize,0,(pixelCol.r<=128 && pixelCol.g<=128 && pixelCol.b<=128)? WHITE : BLACK);
           // HSL
           DrawTextEx(txtFont,TextFormat("HSL: %3.02f, %3.02f%%, %3.02f%%", res.h*360, res.s*100, res.l*100),(Vector2){rectPixel.x + 10, rectPixel.y + 100},fntSize,0,(pixelCol.r<=128 && pixelCol.g<=128 && pixelCol.b<=128)? WHITE : BLACK);   
            
            //  stored color (right click mouse)
            DrawRectangle (rectPixel.x + 6,rectPixel.height - 22 ,260, 24, clickCol);
            DrawTextEx(txtFont,TextFormat("RGBA: %03i, %03i, %03i, %03i",clickCol.r, clickCol.g,clickCol.b,clickCol.a),(Vector2){rectPixel.x+34,rectPixel.y + 125},fntSize,0,(clickCol.r<=128 && clickCol.g<=128 && clickCol.b<=128)? WHITE : BLACK);
            
            // shaded and tinted colors (starting from hover color)
            // for (int i = 1; i < 10; ++i) {
            //     // shaded -10% / -90%
            //     DrawRectangle(rectPixel.x - 21 + (i * 29) ,rectPixel.height - 49, 26, 24, darkenColor(pixelCol,( i*0.1f )));
            //     //DrawText(TextFormat("-%i%%",i*10),rectPixel.x-20+(i*29),rectPixel.height-41,10,WHITE);
            //     // tinted +10% / +90%                
            //     DrawRectangle(rectPixel.x - 21 + (i * 29) ,rectPixel.height - 22, 26, 24, lightenColor(pixelCol,( i * 0.1f )));
            //     //DrawText(TextFormat("+%i%%",i*10),rectPixel.x-20+(i*29),rectPixel.height-14,10,BLACK);
            // }

        	  // disegna istogramma RGBA 
              // sfondo 
            if (showHist) {
                   drawRectangleRounded(rectHist, Fade(BLACK,0.7f));
                    //for (int h = 1; h<10 ; h++) DrawLine(rectHist.x, rectHist.y + (h*12), rectHist.x + rectHist.width, rectHist.y + (h*12), Fade(DARKGRAY,0.6f));
                    //for (int v = 1; v < 22; v++) DrawLine(rectHist.x + (v*12), rectHist.y, rectHist.x + (v*12), rectHist.y + rectHist.height, Fade(DARKGRAY,0.6f));
                      // disegno istrogramma
                      for (int i = 0; i < 256; i++) {
                       	  float hR = (float)histR[i] / maxR;
                          float hG = (float)histG[i] / maxG;
                          float hB = (float)histB[i] / maxB;
                          float hA = (float)histA[i] / maxA;
                          
                            DrawLine((rectHist.x + 8) + i, (rectHist.y + 107), (rectHist.x + 8) + i, (rectHist.y + 107) - (int)(hR*100), Fade(RED, 0.5f));
                            DrawLine( (rectHist.x + 8) + i, (rectHist.y + 107), (rectHist.x + 8) + i, (rectHist.y + 107) - (int)(hG*100), Fade(LIME, 0.5f));
                            DrawLine( (rectHist.x + 8) + i, (rectHist.y + 107), (rectHist.x + 8) + i, (rectHist.y + 107) - (int)(hB*100), Fade(SKYBLUE, 0.5f));  
                            DrawLine( (rectHist.x + 8) + i, (rectHist.y + 107), (rectHist.x + 8) + i, (rectHist.y + 107) - (int)(hA*100), Fade(WHITE, 0.5f));    
                      }
            }

                // --- Minimap show/hide with M key
            if (showMinimap) {
                // Sfondo/Bordo
                DrawRectangleRec((Rectangle){mapPosX - 2, mapPosY - 2, mapWidth + 4, mapHeight + 4}, Fade(BLACK,0.7f));
                
                // Disegno della mini-map
                DrawTexturePro(
                    background, 
                    (Rectangle){ 0, 0, background.width, background.height }, 
                    (Rectangle){ mapPosX, mapPosY, mapWidth, mapHeight },                 
                    (Vector2){ 0, 0 }, 0.0f, WHITE
                );

                // Coordinate e sizing del rettangolo di evidenziazione sulla minimap
                float rectX = mapPosX + (topLeft.x * scaleX);
                float rectY = mapPosY + (topLeft.y * scaleY);
                float rectW = (bottomRight.x - topLeft.x) * scaleX;
                float rectH = (bottomRight.y - topLeft.y) * scaleY;

                // Disegno del rettangolo rosso (con limiti visivi per non uscire dalla mini-mappa)
                DrawRectangle(rectX, rectY, rectW, rectH, Fade(SKYBLUE, 0.25f));
                DrawRectangleLinesEx((Rectangle){ rectX, rectY, rectW, rectH }, 1, SKYBLUE);
                }
                //--------------------------------------------------------------------------------------------------------------


                //file info (bottom panel)
                DrawRectangle(barPos.x,barPos.y,screenWidth,txtOffset,Fade(BLACK,0.7f));
                //filename and size
                DrawTextEx(txtFont,TextFormat("File: %s",fName),(Vector2){8,barPos.y + 6}, fntSize,0,WHITE);
                DrawTextEx(txtFont,TextFormat("| %ix%i (%i pixels) | %d Bytes | %s",img.width,img.height,img.width * img.height, GetFileLength(fName),timeMod),(Vector2){64+fnameSize.x,barPos.y + 6}, fntSize,0,LIGHTGRAY);

            //statusbar with some info
            DrawText(TextFormat("%s", TOOL_SHORT_NAME), screenWidth-124, screenHeight-22, 10, WHITE); 
            DrawText(TextFormat("version %s", TOOL_VERSION), screenWidth-72, screenHeight-22, 10, LIGHTGRAY); 
        EndDrawing();
	}
	// De-Initialization
	//--------------------------------------------------------------------------------------
	UnloadTexture(background);
    UnloadTexture(checkerBoard);
	UnloadImageColors(pixels);
	UnloadImage(img);
    UnloadImage(img1);
    UnloadFont(txtFont);
	CloseWindow();        // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

    return 0;
}
