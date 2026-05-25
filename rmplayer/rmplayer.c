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
* libreria file mp3 
* random / repeat once / all
* ricontrollare logicaa bottoni plaay stop pause : tengo barra e [p]ause?
* 
*******************************************************************************************/

#define TOOL_NAME               "Mod4win Reborn"
#define TOOL_SHORT_NAME         "rMPlayer"
#define TOOL_VERSION            "0.5.0"

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
#define myBLACK      CLITERAL(Color){ 10, 20, 30, 255 }         // Black
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

static float exponent = 0.72f;                 // Audio exponentiation value
static float averageVolume[133] = { 0.0f };   // Average volume history

#define NUM_BUTTONS 7
#define SEEK_TIME 10.0f
#define NUM_FRAMES  3       // Number of frames (rectangles) for the button sprite texture

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
    //SetConfigFlags (FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(screenWidth, screenHeight, "Mod4win Reborn");
    Image image = LoadImage("assets/background.png");     // Loaded in CPU memory (RAM)
    Texture2D backGround = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
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

    // define TOGGLE style
    GuiSetStyle(SLIDER, BASE_COLOR_NORMAL,0x0A141EFF);
    GuiSetStyle(SLIDER, BASE_COLOR_FOCUSED,0x00FF01FF);
    GuiSetStyle(SLIDER, BASE_COLOR_PRESSED,0x00FF01FF);
    GuiSetStyle(SLIDER, TEXT_COLOR_NORMAL,0x00FF01FF);
    GuiSetStyle(SLIDER, TEXT_COLOR_FOCUSED,0x00FF01FF);
    GuiSetStyle(SLIDER, TEXT_COLOR_PRESSED,0x00FF01FF);
    GuiSetStyle(SLIDER, BORDER_WIDTH,0);

    // init Audio
    InitAudioDevice();
    AttachAudioMixedProcessor(ProcessAudio);
    Music music = LoadMusicStream("/home/andrea/Music/test.mp3");
    //PlayMusicStream(music);
    
    // Load texture for toolbar buttons
    Texture2D btnTexture[NUM_BUTTONS]; //  immagine e' 49 x 69 e contiene 3 stati ; ogni stato (FRAME) è quindi  49x23
    btnTexture[0] = LoadTexture("assets/btnStop.png"); // Load button texture for Stop
    btnTexture[1] = LoadTexture("assets/btnPlay.png"); // Load button texture for Play
    btnTexture[2] = LoadTexture("assets/btnPause.png"); // Load button texture for Pause
    btnTexture[3] = LoadTexture("assets/btnPrev.png"); // Load button texture for previous song but
    btnTexture[4] = LoadTexture("assets/btnMinus.png"); // Load button texture for seek -10 sec
    btnTexture[5] = LoadTexture("assets/btnPlus.png"); // Load button texture for seek +10 sec
    btnTexture[6] = LoadTexture("assets/btnNext.png"); // Load button texture for next song in the list
    float frameHeight = btnTexture[0].height / NUM_FRAMES; // altezza immagine / nr. FRAMES
    // Define button position and button size (for every button loaded)
    Rectangle btnRect[NUM_BUTTONS] = { 0 };
    Rectangle srcRect[NUM_BUTTONS] = { 0 };

    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        btnRect[i].x = 9 + (50.0f*i);
        btnRect[i].y = 88;
        btnRect[i].width = 49;
        btnRect[i].height = frameHeight;

        srcRect[i].x = 0;
        srcRect[i].y = 0;
        srcRect[i].width = 49;
        srcRect[i].height = frameHeight;
    }

    int btnState[NUM_BUTTONS] = { 0 };               // Button state: 0-NORMAL, 1-MOUSE_HOVER, 2-PRESSED
    bool btnAction[NUM_BUTTONS] = { false };         // Button action should be activated

    float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
    float current_pos = 0.0f;
    bool isPlay = false;
    bool isStop = true;
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
        Vector2 mousePos = GetMousePosition();
        UpdateMusicStream(music);   // Update music buffer with new stream data
        // Get normalized time played for current music stream
        timePlayed = GetMusicTimePlayed(music)/GetMusicTimeLength(music);
        current_pos = GetMusicTimePlayed(music); //just to simplify some checks

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
            if (!isPause) PauseMusicStream(music), isPause = true, isPlay = false, isStop = false;
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

        // rileva e memorizza stato per ogni bottone della toolbar
        for (int i = 0; i < NUM_BUTTONS; ++i)
        {
            btnAction[i] = false;
            // Check button state (base on mouse position and mouse action)
            if (CheckCollisionPointRec(mousePos, btnRect[i])) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) btnState[i] = 2;
                else btnState[i] = 1;
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) btnAction[i] = true;
            }
            else btnState[i] = 0;
            // Calculate button frame rectangle to draw depending on button state
            srcRect[i].y = btnState[i]*frameHeight;
        }

        if (btnAction[4] && !isStop && !isPause) // seek -10sec
        {
                    if (current_pos < 10.0f) {
                        current_pos = 0.0f; 
                        PauseMusicStream(music);
                        SeekMusicStream(music, 0.0f);
                        PlayMusicStream(music);
                        }
                    else SeekMusicStream(music, current_pos - SEEK_TIME);
        }
        if (btnAction[5] && !isStop && !isPause) // seek +10sec
        {
                    if (current_pos + SEEK_TIME >= GetMusicTimeLength(music)) 
                    {
                        current_pos = 0.0f;
                        PauseMusicStream(music);
                        SeekMusicStream(music, 0.0f);
                        PlayMusicStream(music);
                        }
                    else SeekMusicStream(music, current_pos + SEEK_TIME);
        }

        if (btnAction[0]) { // Stop button
                StopMusicStream(music);
                isStop=true;
                isPlay=false;
                isPause=false;
                }

        if (btnAction[1]) { // play button
                 isStop=false;
                 isPlay=true;
                 isPause=false;
                 PlayMusicStream(music);
            }
        
        if (btnAction[2]) { // pause
            if (!isPause) PauseMusicStream(music), isPause = true, isPlay = false, isStop = false;
            else {
                ResumeMusicStream(music);
                isPause=false;
                isStop=false;
                isPlay=true;
            }
        }

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(BLANK);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLANK);
            // Draw Player background
            DrawTexture(backGround, screenWidth/2 - backGround.width/2, screenHeight/2 - backGround.height/2, WHITE);

            DrawLine(434,8,434,81,myDARKGRAY);
            DrawLine(367,26,500,26,myDARKGRAY);
            DrawLine(367,45,500,45,myDARKGRAY);
            DrawLine(367,64,500,64,myDARKGRAY);
            
            //volume slider
            DrawRectangle(507,12,25,99, myBLACK);
            DrawRectangleLinesEx((Rectangle){508,(int)104-(volume*94),23,6},2,myGREEN);

            DrawTextEx(font,"VOL",(Vector2){509,8},16,0, myDARKGREEN);
            DrawTextEx(font,TextFormat("%03.f",volume*100),(Vector2){509,94},16,0,myDARKGREEN);

            // pan slider
            DrawRectangle(367, 89, 132, 21, myBLACK);
            DrawRectangleLinesEx((Rectangle){(int)(368 + (pan + 1.0f)/2.0f*124), 90, 6, 18},2,myGREEN);
            DrawTextEx(font,"Left",(Vector2){370,90},16,0, myDARKGREEN);
            DrawTextEx(font,"Right",(Vector2){464,90},16,0, myDARKGREEN);

            // seek slider bar    
                float songLength = GetMusicTimeLength(music);
                float sliderSeek = GetMusicTimePlayed(music)/songLength;

                if (isStop < GuiSlider((Rectangle){10,screenHeight-37,screenWidth-20,14},"",NULL, &sliderSeek,0,1.0f)) {
                    SeekMusicStream(music, sliderSeek * songLength);
                }

             // Draw buttons bar
                for (int i = 0; i < NUM_BUTTONS; ++i)
                    DrawTextureRec(btnTexture[i], srcRect[i], (Vector2){ btnRect[i].x, btnRect[i].y }, WHITE); // Draw button frame

            // tempo attuale brano e durata totale brano
            DrawTextEx(font,"Hour   Min    Sec",(Vector2){16,42},16,0, myDARKGREEN);
            char timeStr[32];
            int hour   = (int)GetMusicTimePlayed(music) / 3600;
            int minute = ((int)GetMusicTimePlayed(music) / 60) % 60;
            int second = (int)GetMusicTimePlayed(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d %02d %02d", hour , minute, second);
            DrawTextEx(font,timeStr,(Vector2){16,52},32,0, myGREEN);
            int hours   = (int)GetMusicTimeLength(music) / 3600;
            int minutes = ((int)GetMusicTimeLength(music) / 60) % 60;
            int seconds = (int)GetMusicTimeLength(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d:%02d:%02d", hours, minutes, seconds);
            DrawTextEx(font,timeStr,(Vector2){160,64},16,0, myGREEN);



            // una specie di visualizer : giusto per vivacizzare....
            for (int i = 0; i < 133; i++) //cambiare questo valore anche nelle varbi
            {
                DrawLine(225 + i, 79 - (int)(averageVolume[i]*32), 225 + i, 79, myGREEN);
            }



            // show Play / Stop /Pause status
            DrawTextEx(font,"Play",(Vector2){374,8},16,0, isPlay ? myGREEN : myDARKGREEN);
            DrawTextEx(font,"Stop",(Vector2){374,26},16,0, isStop ? myGREEN : myDARKGREEN);
            DrawTextEx(font,"Pause",(Vector2){374,45},16,0, isPause ? myGREEN : myDARKGREEN);
            // Random flag
            DrawTextEx(font,"Random",(Vector2){374,64},16,0, myDARKGREEN);
            // Mute flag
            DrawTextEx(font,"Mute",(Vector2){440,8},16,0, isMute ? myGREEN:myDARKGREEN);
            // PAN flag
            DrawTextEx(font,"(< PAN >)",(Vector2){440,26},16,0, isPan ? myGREEN : myDARKGREEN);
            // Repeat flag
            DrawTextEx(font,"Repeat",(Vector2){440,64},16,0, myDARKGREEN);

                  // song of songs
            DrawTextEx(font,"Titolo della canzone.mp3",(Vector2){16,4},32,0, myGREEN);
            
            DrawTextEx(font,"0001 ",(Vector2){136,42},16,0, myDARKGREEN);
            DrawTextEx(font,"of 0001",(Vector2){168,42},16,0, myDARKGREEN);





            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, myBLACK); 
            DrawText(TextFormat("version %s", TOOL_VERSION), 64, screenHeight-16, 10, myGRAY); 
            DrawText("[Q] exit program.",screenWidth-96, screenHeight-16,10,myBLACK);

        EndDrawing();
    }
    UnloadTexture(backGround);
    UnloadRenderTexture(target);
    UnloadFont(font);
    DetachAudioMixedProcessor(ProcessAudio);  // Disconnect audio processor
    UnloadMusicStream(music); // Unloaad music stream
    // unload buttons texture 
    for (int i = 0; i < NUM_BUTTONS; i++) UnloadTexture(btnTexture[i]);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
