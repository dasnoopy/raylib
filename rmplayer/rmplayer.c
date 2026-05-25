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
#define TOOL_VERSION            "0.5.1"

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


// ARDUINO Matrix tool colors (light)
#define FG_COLOR    CLITERAL(Color){ 0, 255, 0,255}      // Green
#define TEXT_COLOR  CLITERAL(Color){ 0, 128, 0, 255}      // Dark Green /verde bandiera
#define BG_COLOR      CLITERAL(Color){ 10, 20, 30, 255 }         // background Black
#define BORDER_COLOR CLITERAL(Color){ 55, 65, 70, 255}  //grid color
#define ON_COLOR CLITERAL(Color){ 0, 255, 0, 255}
#define OFF_COLOR CLITERAL(Color){ 0,80, 0,255}
#define VIS_COLOR CLITERAL(Color){ 0, 128, 0, 255}

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

    // Set UI style
    // Custom GUI font loading
    Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetIconScale(1);

    // define TOGGLE style
    GuiSetStyle(SLIDER, BASE_COLOR_NORMAL,0x0A141EFF);
    GuiSetStyle(SLIDER, BASE_COLOR_PRESSED,0x00FF00FF);
    GuiSetStyle(SLIDER, TEXT_COLOR_NORMAL,0x00FF00FF);
    GuiSetStyle(SLIDER, TEXT_COLOR_FOCUSED,0x00FF00FF);
    GuiSetStyle(SLIDER, TEXT_COLOR_PRESSED,0x00FF00FF);
    GuiSetStyle(SLIDER, BORDER_WIDTH,0);

    // init Audio
    InitAudioDevice();
    AttachAudioMixedProcessor(ProcessAudio);
    Music music = LoadMusicStream("/home/andrea/Music/test.mp3");
    
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
    
    // Define button position and button size for every texture loaded
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
    bool isPlay = true;
    bool isStop = !isPlay;
    bool isPause = false;  
    bool isMute = false;
    bool isPan = false;
    float pan = 0.0f;               // Default audio pan center [-1.0f..1.0f]
    float volume = 0.50f;            // Default audio volume [0.0f..1.0f]
    float prev_volume = volume;

    if (isPlay) PlayMusicStream(music);  // autoplay at start
    SetMusicPan(music, pan);
    SetMusicVolume(music, volume);

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);

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
        // if (IsKeyPressed(KEY_SPACE))
        // {

        //     if (!isStop) {
        //         StopMusicStream(music);
        //         isStop=true;
        //         isPlay=false;
        //         isPause=false;
        //     }
        //     else {
        //          isStop=false;
        //          isPlay=true;
        //          isPause=false;
        //          PlayMusicStream(music);
        //     }
        // }

        // // Pause/Resume music playing
        // if (IsKeyPressed(KEY_P))
        // {
        //     if (!isPause) PauseMusicStream(music), isPause = true, isPlay = false, isStop = false;
        //     else {
        //         ResumeMusicStream(music), isPause=false, isStop=false, isPlay=true;
        //     }
        // }

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

            DrawLine(434,8,434,81,BORDER_COLOR);
            DrawLine(367,26,500,26,BORDER_COLOR);
            DrawLine(367,45,500,45,BORDER_COLOR);
            DrawLine(367,64,500,64,BORDER_COLOR);
            
            //volume slider
            DrawRectangle(507,12,25,99, BG_COLOR);
            DrawRectangleLinesEx((Rectangle){508,(int)104-(volume*94),23,6},2,FG_COLOR);

            DrawTextEx(font,"VOL",(Vector2){509,8},16,0, TEXT_COLOR);
            DrawTextEx(font,TextFormat("%03.f",volume*100),(Vector2){509,94},16,0,TEXT_COLOR);

            // pan slider
            DrawRectangle(367, 89, 132, 21, BG_COLOR);
            DrawRectangleLinesEx((Rectangle){(int)(368 + (pan + 1.0f)/2.0f*124), 90, 6, 18},2,FG_COLOR);
            DrawTextEx(font,"Left",(Vector2){370,90},16,0, TEXT_COLOR);
            DrawTextEx(font,"Right",(Vector2){464,90},16,0, TEXT_COLOR);

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
            DrawTextEx(font,"Hour   Min    Sec",(Vector2){16,42},16,0, TEXT_COLOR);
            char timeStr[32];
            int hour   = (int)GetMusicTimePlayed(music) / 3600;
            int minute = ((int)GetMusicTimePlayed(music) / 60) % 60;
            int second = (int)GetMusicTimePlayed(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d %02d %02d", hour , minute, second);
            DrawTextEx(font,timeStr,(Vector2){16,52},32,0, FG_COLOR);
            int hours   = (int)GetMusicTimeLength(music) / 3600;
            int minutes = ((int)GetMusicTimeLength(music) / 60) % 60;
            int seconds = (int)GetMusicTimeLength(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d:%02d:%02d", hours, minutes, seconds);
            DrawTextEx(font,timeStr,(Vector2){160,64},16,0, FG_COLOR);



            // una specie di visualizer : giusto per vivacizzare....
            for (int i = 0; i < 133; i++) //cambiare questo valore anche nelle varbi
            {
                DrawLine(225 + i, 79 - (int)(averageVolume[i]*32), 225 + i, 79, VIS_COLOR);
            }



            // show Play / Stop /Pause status
            DrawTextEx(font,"Play",(Vector2){370,8},16,0, isPlay ? ON_COLOR : OFF_COLOR);
            DrawTextEx(font,"Stop",(Vector2){370,26},16,0, isStop ? ON_COLOR : OFF_COLOR);
            DrawTextEx(font,"Pause",(Vector2){370,45},16,0, isPause ? ON_COLOR : OFF_COLOR);
            // Shuffle flag
            DrawTextEx(font,"Shuffle",(Vector2){370,64},16,0, OFF_COLOR);
            // Mute flag
            DrawTextEx(font,"Mute",(Vector2){436,8},16,0, isMute ? ON_COLOR:OFF_COLOR);
            // PAN flag
            DrawTextEx(font,"(< PAN >)",(Vector2){436,26},16,0, isPan ? ON_COLOR : OFF_COLOR);
            // Repeat flag
            DrawTextEx(font,"Repeat",(Vector2){436,64},16,0, OFF_COLOR);

                  // song of songs
            DrawTextEx(font,"Titolo della canzone.mp3",(Vector2){16,4},32,0, FG_COLOR);
            
            DrawTextEx(font,"0001 ",(Vector2){136,42},16,0, TEXT_COLOR);
            DrawTextEx(font,"of 0001",(Vector2){168,42},16,0, TEXT_COLOR);





            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, BLACK); 
            DrawText(TextFormat("version %s", TOOL_VERSION), 64, screenHeight-16, 10, GRAY); 
            DrawText("[Q] exit program.",screenWidth-96, screenHeight-16,10,BLACK);

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
