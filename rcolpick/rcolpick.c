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
#define TOOL_VERSION            "1.2.1"

#include <raylib.h>
#include <rlgl.h>
#include <raymath.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

    const int screenWidth = 1200;
    const int screenHeight = 720;
    const float minZoom = 1.0f;
    const float stepZoom = 0.2f;
    const float maxZoom = 24.0f;
    const int txtOffset = 32;
    const int fntSize = 18;
    char buffer[64];
      const char *clipboardText = NULL;
    Color pixelCol;
    Color *pixels;
    char fName[384] = { '\0' };
    bool showGrid = false;
    
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
    Rectangle rectPixel = { 932,8,260,160 };
    Rectangle rectHist = { 932,570,260,110 };
            
// 'fake' background
void drawRectangleRounded (Rectangle recSize, Color color)  
{
  float radius = 0.072; // no radius
  int   segs   = 12; // non segments
  DrawRectangleRounded ( recSize, radius, segs, color );
}

Color darkenColor(Color color, float factor)
{
    if (factor < 0.0f) factor = 0.0f;
    if (factor > 1.0f) factor = 1.0f;

    return (Color){
        (unsigned char)(color.r * factor),
        (unsigned char)(color.g * factor),
        (unsigned char)(color.b * factor),
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

int main(void)
{
	 // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "rColorPicker");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    Font txtFont = LoadFontEx("fonts/computer-says-no.otf", fntSize,NULL, 0); // all other text

        strcpy(fName,"./resources/default.png");
        Image img = LoadImage(fName);
        Image img1 = GenImageChecked((int)img.width, (int)img.height, 12,12, WHITE, RAYWHITE);
	Texture2D background = LoadTextureFromImage(img);
	Texture2D checkerBoard = LoadTextureFromImage(img1);
	Color *pixels = LoadImageColors(img);
	
	RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  
	//SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

        // setup a camera
        Camera2D cam = { 0 };
        cam.zoom = minZoom;
        // center the camera on the middle of the screen
        cam.offset = (Vector2){ (screenWidth - img.width)/2,(screenHeight-img.height)/2 };
        cam.target = (Vector2){ 0,0 };
        cam.rotation = 0;        // rotation in deg

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
                        img1 = GenImageChecked((int)img.width, (int)img.height, 12,12, WHITE, RAYWHITE);
        	        background = LoadTextureFromImage(img);
        	        checkerBoard = LoadTextureFromImage(img1);
        	        //carica  pixels color in ram
        	        pixels = LoadImageColors(img);
                   
                   //read pixels color e populate color histogram values
                     getHistogram(img,pixels);

                    // reset camera settings
                    cam.zoom = 1.0f; 
                    cam.offset = (Vector2){ (screenWidth - img.width)/2,(screenHeight-img.height)/2 };
                    cam.target = (Vector2){ 0,0 };
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
    	        
    	        strcpy(fName, "Image pasted from clipboard");
                img = GetClipboardImage();
                img1 = GenImageChecked((int)img.width, (int)img.height, 12,12, WHITE, RAYWHITE);
    	        background = LoadTextureFromImage(img);
    	        checkerBoard = LoadTextureFromImage(img1);
    	        //carica  pixels color in ram
    	        pixels = LoadImageColors(img);
                
                //read pixels color e populate color histogram values
                getHistogram(img,pixels);
                // reset camera settings
                cam.zoom = 1.0f; 
                cam.offset = (Vector2){ (screenWidth - img.width)/2,(screenHeight-img.height)/2 };
                cam.target = (Vector2){ 0,0 };
                }
                else TraceLog(LOG_INFO, "IMAGE: Could not retrieve image from clipboard");
            }
            
        Vector2 fnameSize = MeasureTextEx(txtFont, fName, fntSize, 0);
        Vector2 mousePos = GetMousePosition();   // abilitare se cursore libero   
        // Get the world point that is under the mouse
        Vector2 world = GetScreenToWorld2D(GetMousePosition(), cam);
        
                if (IsKeyPressed(KEY_X)) {
                    cam.zoom = 1.0f; 
                    cam.offset = (Vector2){ (screenWidth - img.width)/2,(screenHeight-img.height)/2 };
                    cam.target = (Vector2){ 0,0 };
                }
                
                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C)) {
                          snprintf(buffer, sizeof(buffer), "%i,%i,%i,%i //RGBA format", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a);
                          SetClipboardText(buffer); // Copy text to clipboard
                          clipboardText = GetClipboardText(); // Get text from clipboard
                      }
                      
                if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_H)) {
                          snprintf(buffer, sizeof(buffer), "#%X%X%X%X //HEXA format", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a);
                          SetClipboardText(buffer); // Copy text to clipboard
                          clipboardText = GetClipboardText(); // Get text from clipboard
                      }
                      
                //pan immagine con tast sx del mouse premuto..
              	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            			Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f / cam.zoom);
            			// set the camera target to follow the player
            			cam.target = Vector2Add(cam.target, delta);
            		}

            float wheel = GetMouseWheelMove();
            int x = (int)world.x;
            int y = (int)world.y;
            
            // leggi colore pixel 
            if (x >= 0 && x < img.width && y >= 0 && y < img.height) pixelCol = pixels[y * img.width + x];
                  
            if (wheel != 0) {
                //https://www.reddit.com/r/raylib/comments/1plbm84/how_to_zoom_a_2d_camera_on_mouse_position/
                // 1. Get mouse position in world coordinates before zoom
                Vector2 mouseScreen = GetMousePosition();
                Vector2 preZoomWorldPos = GetScreenToWorld2D(mouseScreen, cam);
            
        		// 2. Apply zoom change
                //cam.zoom += wheel * 1.0f;           		
        	   // Camera zoom controls
                // Uses log scaling to provide consistent zoom speed
                cam.zoom = expf(logf(cam.zoom) + ((float)wheel*stepZoom));
                if (cam.zoom > maxZoom) cam.zoom = maxZoom;
                else if (cam.zoom < minZoom) cam.zoom = minZoom;

        		// 3. Get mouse position in world coordinates after zoom
		          Vector2 postZoomWorldPos = GetScreenToWorld2D(mouseScreen, cam);

    		// 4. Offset camera.target so the world position under the cursor stays fixed
    		cam.target.x += roundf(preZoomWorldPos.x - postZoomWorldPos.x);
    		cam.target.y += roundf(preZoomWorldPos.y - postZoomWorldPos.y);
    	}	
		
        //cursor centered
        Vector2 cursor = { x + 0.5f, y + 0.5f };
        Vector2 center = GetWorldToScreen2D(cursor, cam);
       
        // funzione di conversione HSV di raylib
        Vector3 hsv = ColorToHSV(pixelCol);

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
            ClearBackground(RAYWHITE);
        EndTextureMode();
        
		BeginDrawing();
            HideCursor();
            // sfondo fuori dall' immagine
            ClearBackground(RAYWHITE);
                  
            //BeginScissorMode((int)scissorArea.x, (int)scissorArea.y, (int)scissorArea.width, (int)scissorArea.height);   
    		    BeginMode2D(cam);
	           	// draw the entire background image for the entire world. The camera will clip it to the screen
		        DrawTexture(checkerBoard, 0, 0, WHITE);
		        DrawTexture(background, 0, 0, WHITE);
		          //DrawRectangleLinesEx(screenInWorldRect, 4 / cam.zoom, Fade(BLACK,0.8f));
		        
                      if (!showGrid) DrawRectangleLines(0,0,background.width, background.height,Fade(GRAY,0.5f)); 
		      
		      if (cam.zoom >=12.0f) {
		        showGrid=true;
                          // linee verticali
                          for (int x = 0; x <= background.width; x++) DrawLine(x, 0, x, background.height, Fade(GRAY, 0.5f));
                          // linee orizzontali
                         for (int y = 0; y <= background.height; y++) DrawLine(0, y, background.width, y, Fade(GRAY, 0.5f));
                      }
                      else showGrid = false;
          
		          EndMode2D();
		      
		      if (cam.zoom >= 12.0f) {
          	      // mirino cursore centerd
                        DrawLine(center.x-16, center.y, center.x + 16, center.y, RED);
                        DrawLine(center.x, center.y-16, center.x, center.y+16, RED);
                      }
                      else  {
		       // mirino cursore libero
                        DrawLine(mousePos.x-16, mousePos.y, mousePos.x+16 , mousePos.y, RED);
                        DrawLine(mousePos.x, mousePos.y-16 , mousePos.x,mousePos.y+16, RED);
                      }                      
            //EndScissorMode();	      

            //file info (bottom panel)
            DrawRectangle(barPos.x,barPos.y,screenWidth,txtOffset,Fade(BLACK,0.7f));
            //filename and size
            DrawTextEx(txtFont,TextFormat("File: %s",fName),(Vector2){8,barPos.y + 6}, fntSize,0,WHITE);
            DrawTextEx(txtFont,TextFormat("| %ix%i (%i pixels) | %d Bytes | %s",img.width,img.height,img.width * img.height, GetFileLength(fName),timeMod),(Vector2){64+fnameSize.x,barPos.y + 6}, fntSize,0,LIGHTGRAY);   
            // pixel panel ("floating")
            drawRectangleRounded (rectPixel, Fade(BLACK,0.7f));
            // cursor x,y
            DrawTextEx(txtFont,TextFormat("X,Y,Zoom: %i, %i, %02.f%%", x, y,cam.zoom*100),(Vector2){rectPixel.x + 8, rectPixel.y + 4},fntSize,0,WHITE);
            // RGBA
            DrawTextEx(txtFont,TextFormat("RGBA: %03i, %03i, %03i, %03i", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a),(Vector2){ rectPixel.x + 8, rectPixel.y + 28},fntSize,0,LIGHTGRAY);
            // HEXA 
            DrawTextEx(txtFont,TextFormat("HEXA: #%02X%02X%02X%02X", pixelCol.r,pixelCol.g,pixelCol.b,pixelCol.a),(Vector2){rectPixel.x + 8, rectPixel.y + 52},fntSize,0,WHITE);
           // HSV
           DrawTextEx(txtFont,TextFormat("HSV: %.1f, %.1f%%, %.1f%%", hsv.x,hsv.y*100.0f,hsv.z*100.0f),(Vector2){rectPixel.x + 8, rectPixel.y + 76},fntSize,0,LIGHTGRAY);   
            
            // riquadro colore 
            for (int i = 0; i < 5; ++i)
                DrawRectangle(rectPixel.x + 8 + (i * 50) ,rectPixel.y + 104,46, 46, darkenColor(pixelCol,(1.0f-(i*0.2f) ) ) );

	  //istogramma RGB      
       drawRectangleRounded (rectHist, Fade(BLACK,0.7f));
        //for (int h = 1; h<10 ; h++) DrawLine(rectHist.x, rectHist.y + (h*12), rectHist.x + rectHist.width, rectHist.y + (h*12), Fade(DARKGRAY,0.6f));
        //for (int v = 1; v < 22; v++) DrawLine(rectHist.x + (v*12), rectHist.y, rectHist.x + (v*12), rectHist.y + rectHist.height, Fade(DARKGRAY,0.6f));
          
      for (int i = 0; i < 256; i++) {
       	  float hR = (float)histR[i] / maxR;
          float hG = (float)histG[i] / maxG;
          float hB = (float)histB[i] / maxB;
          float hA = (float)histA[i] / maxA;
          
            DrawLine((rectHist.x + 3) + i, (rectHist.y + 107), (rectHist.x + 3) + i, (rectHist.y + 107) - (int)(hR*100), Fade(RED, 0.5f));
            DrawLine( (rectHist.x + 3) + i, (rectHist.y + 107), (rectHist.x + 3) + i, (rectHist.y + 107) - (int)(hG*100), Fade(LIME, 0.5f));
            DrawLine( (rectHist.x + 3) + i, (rectHist.y + 107), (rectHist.x + 3) + i, (rectHist.y + 107) - (int)(hB*100), Fade(SKYBLUE, 0.5f));  
            DrawLine( (rectHist.x + 3) + i, (rectHist.y + 107), (rectHist.x + 3) + i, (rectHist.y + 107) - (int)(hA*100), Fade(WHITE, 0.5f));    
          }

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
