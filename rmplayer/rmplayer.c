/*******************************************************************************************
*
* 
*   raylib MPlaayer
*   Small utility to plaay mp3 files
*   A simple app to learn C using raylib library

*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
* 
* array di brani no? si?
* funzione preview
* slider veri al posto dei fake racthgle
* visualizer
* libreria file mp3 
* random / repeat once / all
* 
*******************************************************************************************/

#define TOOL_NAME               "Music Player"
#define TOOL_SHORT_NAME         "rMPlayer"
#define TOOL_VERSION            "0.4.0"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

// raygui integration
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"


// window initial size
const int screenWidth = 540;
const int screenHeight = 156;


// custom Colors
#define myWHITE      CLITERAL(Color){ 255, 255, 255, 255 }   // White
#define myBLACK      CLITERAL(Color){ 14, 35, 46, 255 }         // Black
#define myBLANK      CLITERAL(Color){ 0, 0, 0, 0 }           // Blank (Transparent)
#define myYELLOW     CLITERAL(Color){ 255, 233, 3, 255 }     // Yellow / Giallo Modena Ferrari
#define myGOLD       CLITERAL(Color){ 239,191,4, 255 }     // Gold
#define myORANGE     CLITERAL(Color){ 255, 128, 0, 255 }     //  Orange Mclaren Papaya
#define myPINK       CLITERAL(Color){ 255, 192, 203, 255 }     //  Pink Panther
#define myRED        CLITERAL(Color){ 205, 33, 42, 255 }     //  Red /Rosso bandiera
#define myMAROON     CLITERAL(Color){ 148,34,34, 255 }     //  Maroon / Granata
#define myGREEN      CLITERAL(Color){ 0, 255, 1, 255 }      // Green
#define myLIME       CLITERAL(Color){ 70, 163, 41, 255 }      // Lime
#define myDARKGREEN  CLITERAL(Color){ 32, 104, 17, 255 }      // Dark Green /verde bandiera
#define mySKYBLUE    CLITERAL(Color){ 25, 174, 255, 255 }   // Sky Blue
#define myBLUE       CLITERAL(Color){ 0, 132, 200, 255 }     // Blue
#define myDARKBLUE   CLITERAL(Color){ 0, 92, 148, 255 }      // Dark Blue
#define myPURPLE     CLITERAL(Color){ 144,99,205, 255 }   // Purple
#define myVIOLET     CLITERAL(Color){ 112,74,191, 255 }    // Violet
#define myDARKPURPLE CLITERAL(Color){ 66,49,137, 255 }    // Dark Purple
#define myBEIGE      CLITERAL(Color){ 217,182,154, 255 }   // Beige
#define myBROWN      CLITERAL(Color){ 121,85,61, 255 }    // Brown
#define myDARKBROWN  CLITERAL(Color){ 73,55,43, 255 }      // Dark Brown
#define myLIGHTGRAY  CLITERAL(Color){ 189, 205, 212,255}   // Light Gray
#define myGRAY       CLITERAL(Color){ 111, 131, 136, 255 }   // Gray
#define myDARKGRAY   CLITERAL(Color){ 54, 78, 89, 255 }      // Dark Gray

static float exponent = 1.0f;                 // Audio exponentiation value
static float averageVolume[133] = { 0.0f };   // Average volume history


// 'fake' background
void drawRectangleRounded (int x, int y, int w, int h, float radius, Color color)  
{
  Rectangle  rect = { x, y, w, h};   // toplx, toply, width, height
  int segs = 6; // non segments
  DrawRectangleRounded ( rect, radius, segs, color );
}

//------------------------------------------------------------------------------------
// Audio processing function
//------------------------------------------------------------------------------------
void ProcessAudio(void *buffer, unsigned int frames)
{
    float *samples = (float *)buffer;   // Samples internally stored as <float>s
    float average = 0.0f;               // Temporary average volume

    for (unsigned int frame = 0; frame < frames; frame++)
    {
        float *left = &samples[frame*2 + 0], *right = &samples[frame*2 + 1];

        *left = powf(fabsf(*left), exponent)*( (*left < 0.0f)? -1.0f : 1.0f );
        *right = powf(fabsf(*right), exponent)*( (*right < 0.0f)? -1.0f : 1.0f );

        average += fabsf(*left)/frames;   // accumulating average volume
        average += fabsf(*right)/frames;
    }

    // Moving history to the left
    for (int i = 0; i < 132; i++) averageVolume[i] = averageVolume[i + 1];

    averageVolume[132] = average;         // Adding last average value
}

int main (int argc, char *argv[])
{
    //nascondi finestra durante caricamento iniziale
    SetWindowState(FLAG_WINDOW_HIDDEN);
    SetConfigFlags (FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(screenWidth, screenHeight, "Raylib Music Player");
    Image image = LoadImage("assets/background.png");     // Loaded in CPU memory (RAM)
    Texture2D texture = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
    UnloadImage(image);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    //SetWindowState(FLAG_WINDOW_TOPMOST);
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  
    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);
    // Set UI style
    // Custom GUI font loading
    Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetIconScale(1);

    // init Audio
    InitAudioDevice();
    AttachAudioMixedProcessor(ProcessAudio);
    Music music = LoadMusicStream("Music/test.mp3");
    PlayMusicStream(music);
    
    float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
    bool isPlay = true;
    bool isStop = false;
    bool isPause = false;  
    bool isMute = false;
    bool isPan = false;
    float pan = 0.0f;               // Default audio pan center [-1.0f..1.0f]
    SetMusicPan(music, pan);
    float volume = 0.50f;            // Default audio volume [0.0f..1.0f]
    float prev_volume = volume;

    SetMusicVolume(music, volume);

    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);

    while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        UpdateMusicStream(music);   // Update music buffer with new stream data

        // Get normalized time played for current music stream
        timePlayed = GetMusicTimePlayed(music)/GetMusicTimeLength(music);
        if (timePlayed > 1.0f) timePlayed = 1.0f;   // Make sure time played is no longer than music
        
        // Restart music playing (stop and play)
        if (IsKeyPressed(KEY_SPACE))
        {

            if (!isStop) {
                StopMusicStream(music);
                isStop=true;
                isPlay=false;
                isPause=false;
            }
            else {
             isStop=false;
             isPlay=true;
             isPause=false;
             PlayMusicStream(music);
            }
            
        }

        // Pause/Resume music playing
        if (IsKeyPressed(KEY_P))
        {
           
            if (!isPause) { 
                PauseMusicStream(music);
                isPause = true;
                isPlay = false;
                isStop = false;
            }

            else {
                ResumeMusicStream(music);
                isPause=false;
                isStop=false;
                isPlay=true;
            }
        }

        // Set audio pan
        if (IsKeyPressed(KEY_LEFT))
        {
            isPan = true;
            pan -= 0.02f;
            if (pan < -1.0f) pan = -1.0f;
            SetMusicPan(music, pan);
        }
        else if (IsKeyPressed(KEY_RIGHT))
        {
            isPan = true;
            pan += 0.02f;
            if (pan > 1.0f) pan = 1.0f;
            SetMusicPan(music, pan);
        }
        else if (IsKeyPressed(KEY_C))
        {
            isPan = false;
            pan = 0.0f;
            SetMusicPan(music, pan);
        }

        // Set audio volume
        if (IsKeyDown(KEY_DOWN))
        {
            volume -= 0.02f;
            if (volume < 0.0f) volume = 0.0f;
            SetMusicVolume(music, volume);
        }
        else if (IsKeyDown(KEY_UP))
        {
            isMute = false;
            volume += 0.02f;
            if (volume > 1.0f) volume = 1.0f;
            SetMusicVolume(music, volume);
        }
        else if (IsKeyPressed(KEY_M)) // MUTE
        {
            isMute = !isMute;
            if (isMute) {
                prev_volume = volume;
                volume = 0.0f;

            }
            else volume = prev_volume;

        SetMusicVolume(music, volume);
        }


        //----------------------------------------------------------------------------------

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(BLANK);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLANK);
            // Draw Player background
            DrawTexture(texture, screenWidth/2 - texture.width/2, screenHeight/2 - texture.height/2, WHITE);

            //volume
            DrawRectangle(507,12,25,99, BLACK);
            DrawRectangle(508,(int)104-(volume*94),23,6,myLIME);
            DrawTextEx(font,"VOL",(Vector2){509,8},16,0, myGREEN);
            DrawTextEx(font,TextFormat("%03.f",volume*100),(Vector2){509,94},16,0,myGREEN);

            // pan slider
            DrawRectangle(367, 89, 132, 21, BLACK);
            DrawRectangle((int)(368 + (pan + 1.0f)/2.0f*124), 90, 6, 18,myLIME);
            DrawTextEx(font,"L                            R",(Vector2){370,90},16,0, myGREEN);
            //DrawTextEx(font,"Right",(Vector2){464,86},16,0, myLIME);


            // seek slider bar    
                float songLength = GetMusicTimeLength(music);
                float sliderSeek = GetMusicTimePlayed(music)/songLength;
                // define TOGGLE style
                GuiSetStyle(SLIDER, BORDER_COLOR_NORMAL,0x848285FF);
                GuiSetStyle(SLIDER, BORDER_COLOR_FOCUSED,0x848285FF);
                GuiSetStyle(SLIDER, BORDER_COLOR_PRESSED,0x848285FF);
                GuiSetStyle(SLIDER, BASE_COLOR_NORMAL,0x000000FF);
                GuiSetStyle(SLIDER, BASE_COLOR_FOCUSED,0x00FF01FF);
                GuiSetStyle(SLIDER, BASE_COLOR_PRESSED,0x00FF01FF);
                GuiSetStyle(SLIDER, TEXT_COLOR_NORMAL,0x00FF01FF);
                GuiSetStyle(SLIDER, TEXT_COLOR_FOCUSED,0x00FF01FF);
                GuiSetStyle(SLIDER, TEXT_COLOR_PRESSED,0x00FF01FF);
                GuiSetStyle(SLIDER, BORDER_WIDTH,0);
                if (!isPlay < GuiSlider((Rectangle){10,screenHeight-37,screenWidth-20,14},"",NULL, &sliderSeek,0,1.0f))
                    SeekMusicStream(music, sliderSeek * songLength);

            // tempo da inizio brano e durata totale
            DrawTextEx(font,"Hour  Min    Sec",(Vector2){18,42},16,0, myLIME);
            char timeStr[32];
            int hour = 0; //(int)GetMusicTimeLength(music) / .....
            int minute = (int)GetMusicTimePlayed(music) / 60;
            int second = (int)GetMusicTimePlayed(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d %02d %02d", hour , minute, second);
            DrawTextEx(font,timeStr,(Vector2){16,52},32,0, myGREEN);
            int hours = 0; //(int)GetMusicTimeLength(music) / .....
            int minutes = (int)GetMusicTimeLength(music) / 60;
            int seconds = (int)GetMusicTimeLength(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d:%02d:%02d", hours, minutes, seconds);
            DrawTextEx(font,timeStr,(Vector2){160,64},16,0, myGREEN);

            // una specie di visualizer
            for (int i = 0; i < 133; i++) //cambiare questo valore anche nelle varbi
            {
                DrawLine(225 + i, 79 - (int)(averageVolume[i]*32), 225 + i, 79, myGREEN);
            }


            // Play / Stop /Pause flag
            DrawTextEx(font,"Play",(Vector2){370,8},16,0, isPlay ? myGREEN : myBLACK);
            DrawTextEx(font,"Stop",(Vector2){370,22},16,0, isStop ? myGREEN : myBLACK);
            DrawTextEx(font,"Pause",(Vector2){370,36},16,0, isPause ? myGREEN : myBLACK);
            // Random flag
            DrawTextEx(font,"Random",(Vector2){370,50},16,0, myBLACK);
            // Mute flag
            DrawTextEx(font,"Mute",(Vector2){468,8},16,0, isMute ? myGREEN:myBLACK);
            // Repeat flag
            DrawTextEx(font,"Repeat",(Vector2){456,22},16,0, myBLACK);
            // PAN flag
            DrawTextEx(font,"(< PAN >)",(Vector2){446,36},16,0, isPan ? myGREEN : myBLACK);

                  // song of songs
            DrawTextEx(font,"Titolo della canzone.mp3",(Vector2){16,4},32,0, myLIME);
            
            DrawTextEx(font,"0001 ",(Vector2){136,42},16,0, myLIME);
            DrawTextEx(font,"of 0001",(Vector2){168,42},16,0, myLIME);





            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, myBLACK); 
            DrawText(TextFormat("version %s", TOOL_VERSION), 64, screenHeight-16, 10, myGRAY); 
            DrawText("[Q] exit program.",screenWidth-96, screenHeight-16,10,myBLACK);

        EndDrawing();
    }
    UnloadTexture(texture);
    UnloadRenderTexture(target);
    UnloadFont(font);
    DetachAudioMixedProcessor(ProcessAudio);  // Disconnect audio processor
    UnloadMusicStream(music); // Unloaad music stream
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
