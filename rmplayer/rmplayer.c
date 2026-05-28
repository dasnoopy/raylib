/*******************************************************************************************
*
*   raylib MPlayer (a sort of MOD4WIN revival)
*   Small utility to play mp3 files based on Raylib
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
* 
* repeat all funziona repeat1 ...?? vabbe
* scan 10 sec all songs
* mostra elenco seleziona file
* tag mp3 id3v2?  meglio 
* desktop file con relativa icona
* salvare config :  autoplay si no,  shuffle at start, colori, folder music
*
*******************************************************************************************/

#define TOOL_NAME               "Mod4win Reborn"
#define TOOL_SHORT_NAME         "rMPlayer"
#define TOOL_VERSION            "0.8.1"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// raygui integration
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// window initial size
const int screenWidth = 540;
const int screenHeight = 156;

// some custom colors
#define FG_COLOR      CLITERAL(Color){ 0, 255, 0, 255 }       // Green
#define TEXT_COLOR    CLITERAL(Color){ 0,128, 0, 255 }      // Dark Green /verde bandiera
#define BORDER_COLOR  TEXT_COLOR // CLITERAL(Color){ 128, 130, 133, 255}  //grid color
#define ON_COLOR      FG_COLOR // CLITERAL(Color){ 0, 255, 0, 255}
#define OFF_COLOR     TEXT_COLOR //CLITERAL(Color){ 0,64, 0,255}
#define VIS_COLOR     FG_COLOR //CLITERAL(Color){ 0, 255, 128, 255 }
#define SLI_COLOR     0x008000FF // slider color
#define SLI_BG_COLOR  0x0A141EFF // slider background color


static float exponent = 0.88f;                 // Audio exponentiation value
static float averageVolume[134] = { 0.0f };   // Average volume history

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
Music music;
float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
float current_pos = 0.0f;
bool isPlay = false;
bool isStop = true;
bool isPause = false;  
bool isMute = false;
bool isPan = false;
bool isShuffle = true;
bool isScan = false;
float pan = 0.0f;               // Default audio pan center [-1.0f..1.0f]
float volume = 0.50f;            // Default audio volume [0.0f..1.0f]
float prev_volume = 0.0f;

// Music library

#define MAX_FILEPATH_SIZE       1024
#define FILE_FILTER             ".mp3;.ogg"

const char *dirPath = "/home/andrea/Music";
int  musicFileCount = 0;
int  current_play = 0;
int  selectedFile = -1;

// Funzione che restituisce un FilePathList dei file in basePath con estensioni filter
FilePathList GetMusicFromDirectory(const char *basePath, const char *filter, bool includeSubdirs){
    FilePathList files = LoadDirectoryFilesEx(basePath, filter, includeSubdirs);
    if (files.count == 0) printf("Nessun file trovato in '%s' con filtro '%s'\n", basePath, filter);
    else musicFileCount = files.count;
    return files; 
}

void LoadMusicByIndex(int idx, FilePathList files) {
    if (idx < 0 || idx >= musicFileCount) return;
    music = LoadMusicStream(files.paths[idx]);
    current_play = idx;
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
    for (int i = 0; i < 133; i++) averageVolume[i] = averageVolume[i + 1];

    averageVolume[133] = average;         // Adding last average value
}

int main (int argc, char *argv[])
{
    //nascondi finestra durante caricamento iniziale
    SetWindowState(FLAG_WINDOW_HIDDEN);
    //SetConfigFlags (FLAG_MSAA_4X_HINT); // occhio che sdoppi ale linee e sfalsa un po i colori
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
    Font fontx32 = LoadFontEx("assets/EuroPCMono.ttf", 32, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetFont(font);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    GuiSetIconScale(1);

    // define TOGGLE style
    GuiSetStyle(SLIDER, BASE_COLOR_NORMAL,SLI_BG_COLOR); // slider background color
    GuiSetStyle(SLIDER, BASE_COLOR_PRESSED,SLI_COLOR); // slider fg color
    GuiSetStyle(SLIDER, TEXT_COLOR_NORMAL,SLI_COLOR);
    GuiSetStyle(SLIDER, TEXT_COLOR_FOCUSED,SLI_COLOR);
    GuiSetStyle(SLIDER, TEXT_COLOR_PRESSED,SLI_COLOR);
    GuiSetStyle(SLIDER, SLIDER_WIDTH, 24);
    GuiSetStyle(SLIDER, SLIDER_PADDING, 1);
    GuiSetStyle(SLIDER, BORDER_WIDTH,0);

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

    // init Audio
    InitAudioDevice();
    AttachAudioMixedProcessor(ProcessAudio);
    // Load music files
    FilePathList musicFiles = GetMusicFromDirectory(dirPath,FILE_FILTER,true);

    // randomize initial song or not
    if (isShuffle) LoadMusicByIndex(GetRandomValue(0,musicFileCount),musicFiles);
    else LoadMusicByIndex(current_play,musicFiles);

    if (isPlay) PlayMusicStream(music);  // autoplay at start
    
    // set initial volume and panning
    SetMusicPan(music, pan);
    SetMusicVolume(music, volume);

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    SetTargetFPS(60);

    selectedFile = current_play;
    Rectangle displayArea = { 9, 6, 349,29 };

    float titleX = displayArea.x ;
    float speed = 60.0f;
    char *titleStr = malloc(2048);

    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);


 while (!WindowShouldClose())
{
        strcpy(titleStr, GetFileNameWithoutExt(musicFiles.paths[current_play]));
        Vector2 titleSize = MeasureTextEx(fontx32, titleStr, 32, 0);
        float titleWidth = titleSize.x;
        bool needScroll = titleWidth > displayArea.width;

        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        // set scroll text speed
        float dt = GetFrameTime();
        if (needScroll) {
            titleX -= speed * dt;
            if (titleX <= displayArea.x - titleWidth) titleX += titleWidth + displayArea.width;
        }

        Vector2 mousePos = GetMousePosition();
        UpdateMusicStream(music);   // Update music buffer with new stream data

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

        // Set audio pan
        if (IsKeyDown(KEY_LEFT))
        {
            isPan = true;
            pan -= 0.02f;
            if (pan < -1.0f) pan = -1.0f;
            SetMusicPan(music, pan);
        }
        else if (IsKeyDown(KEY_RIGHT))
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
        else if (IsKeyPressed(KEY_S)) isShuffle = !isShuffle;
       
        else if (IsKeyPressed(KEY_P))
        {
            isScan = !isScan;
         
        }
        // Set audio volume
        else if (IsKeyDown(KEY_DOWN))
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

        if (btnAction[0]) { // Stop button
                ResumeMusicStream(music);
                StopMusicStream(music);
                //UpdateMusicStream(music);
                isStop=true;
                isPlay=false;
                isPause=false;
                }

        if (btnAction[1] && !isPause ) { // play button
                StopMusicStream(music);
                PlayMusicStream(music);
                //UpdateMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
        }
        
        if (btnAction[2] && isPlay) { // pause
            isPause = !isPause;
            if (isPause) PauseMusicStream(music);
            else ResumeMusicStream(music);
        }

        if (btnAction[4] && isPlay) // seek -10sec
        {
                    if (current_pos < 10.0f) {
                        current_pos = 0.0f; 
                        SeekMusicStream(music, 0.0f);
                       // UpdateMusicStream(music);
                    }
                    else {
                        SeekMusicStream(music, current_pos - SEEK_TIME);
                        //UpdateMusicStream(music);
                    }
        }
        if (btnAction[5] && isPlay) // seek +10sec
        {
                    if (current_pos + SEEK_TIME >= GetMusicTimeLength(music)) 
                    {
                        current_pos = 0.0f;
                        SeekMusicStream(music, 0.0f);
                        //UpdateMusicStream(music);
                    }
                    else {
                        SeekMusicStream(music, current_pos + SEEK_TIME);
                        //UpdateMusicStream(music);
                    }
        }

        if (btnAction[3]) { // Previous song : no shuffle on previous song
                current_play--;
                if (current_play<=0) current_play=0;
                selectedFile = current_play;
                StopMusicStream(music);
                LoadMusicByIndex(current_play,musicFiles);
                PlayMusicStream(music);
                //UpdateMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
                 titleX = displayArea.x;
        }

        if (btnAction[6]) { // Next song based on SHUFFLE setting

            if (isShuffle) {
                int shuffle_index = GetRandomValue(0,musicFileCount);
                if (shuffle_index == musicFileCount) --shuffle_index;
                if (musicFileCount > 1 && shuffle_index == current_play)
                    shuffle_index = (shuffle_index + 1) % musicFileCount;
                current_play = shuffle_index;
            } else {
                current_play++;
                if (current_play >= musicFileCount) current_play = 0;
            }
                selectedFile = current_play;
                StopMusicStream(music);
                LoadMusicByIndex(current_play,musicFiles);
                PlayMusicStream(music);
                //UpdateMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
                titleX = displayArea.x;
            }

        // Get normalized time played for current music stream
        timePlayed = GetMusicTimePlayed(music)/GetMusicTimeLength(music);
        if (timePlayed > 1.0f) timePlayed = 1.0f;   // Make sure time played is no longer than music
        current_pos = GetMusicTimePlayed(music); //just to simplify some checks

        // set button status in stop, play, pause
        if (isStop) {
            btnState[0] = 1;
            srcRect[0].y = btnState[0]*frameHeight;   
            }

        if (isPlay) {
            btnState[1] = 1;
            srcRect[1].y = btnState[1]*frameHeight;   
            }

        if (isPause) {
            btnState[2] = 1;
            srcRect[2].y = btnState[2]*frameHeight;   
            }


        // auto move on next song and randomize played song if shuffle is enabled
        if (GetMusicTimePlayed(music) >= GetMusicTimeLength(music) - 0.05f)
            {
                StopMusicStream(music);
                UnloadMusicStream(music);
                    if (isShuffle) {
                        int shuffle_index = GetRandomValue(0,musicFileCount);
                        if (shuffle_index == musicFileCount) --shuffle_index;
                        if (musicFileCount > 1 && shuffle_index == current_play)
                            shuffle_index = (shuffle_index + 1) % musicFileCount;
                        current_play = shuffle_index;
                    } else {
                        current_play++;
                        if (current_play >= musicFileCount) current_play = 0;
                    }
                selectedFile = current_play;
                LoadMusicByIndex(current_play,musicFiles);
                PlayMusicStream(music);
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
            // grid flaags
            DrawLine(434,8,434,81,BORDER_COLOR);
            DrawLine(367,26,500,26,BORDER_COLOR);
            DrawLine(367,45,500,45,BORDER_COLOR);
            DrawLine(367,64,500,64,BORDER_COLOR);
            
            //volume slider
            DrawRectangleLinesEx((Rectangle){508,(int)104-(volume*94),23,6},2,FG_COLOR);
            DrawTextEx(font,"VOL",(Vector2){509,8},16,0, TEXT_COLOR);
            DrawTextEx(font,TextFormat("%03.f",volume*100),(Vector2){509,94},16,0,TEXT_COLOR);

            // pan slider
            DrawRectangleLinesEx((Rectangle){(int)(368 + (pan + 1.0f)/2.0f*124), 90, 6, 18},2,FG_COLOR);
            DrawTextEx(font,"Left",(Vector2){370,90},16,0, TEXT_COLOR);
            DrawTextEx(font,"Right",(Vector2){464,90},16,0, TEXT_COLOR);

            BeginScissorMode( (int)displayArea.x, (int)displayArea.y, (int)displayArea.width, (int)displayArea.height);
                if (needScroll) DrawTextEx(fontx32, titleStr, (Vector2){ titleX, displayArea.y }, 32, 0, FG_COLOR);
                else DrawTextEx(fontx32, titleStr, (Vector2){ displayArea.x, displayArea.y}, 32,0, FG_COLOR);
            EndScissorMode();

            // Draw buttons bar
                for (int i = 0; i < NUM_BUTTONS; ++i) {
                    DrawRectangle(btnRect[i].x,btnRect[i].y,btnRect[i].width,btnRect[i].height, FG_COLOR);
                    DrawTextureRec(btnTexture[i], srcRect[i], (Vector2){ btnRect[i].x, btnRect[i].y }, WHITE); // Draw button frame
                }
            // tempo attuale brano e durata totale brano
            DrawTextEx(font,"Hour   Min    Sec",(Vector2){10,41},16,0, TEXT_COLOR);
            char timeStr[32];
            int hour   = (int)GetMusicTimePlayed(music) / 3600;
            int minute = ((int)GetMusicTimePlayed(music) / 60) % 60;
            int second = (int)GetMusicTimePlayed(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d %02d %02d", hour , minute, second);
            DrawTextEx(fontx32,timeStr,(Vector2){10,53},32,0, FG_COLOR);
            int hours   = (int)GetMusicTimeLength(music) / 3600;
            int minutes = ((int)GetMusicTimeLength(music) / 60) % 60;
            int seconds = (int)GetMusicTimeLength(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d:%02d:%02d", hours, minutes, seconds);
            DrawTextEx(font,timeStr,(Vector2){160,64},16,0, FG_COLOR);

            // a sort of visualizer : giusto per vivacizzare....
            for (int i = 0; i < 134; ++i) //cambiare questo valore anche nelle varbi
                DrawLine(225 + i, 80 - (int)(averageVolume[i]*32), 225 + i, 80, VIS_COLOR);

            // show Play / Stop /Pause status
            DrawTextEx(font,"Play",(Vector2){370,8},16,0, isPlay ? ON_COLOR : OFF_COLOR);
            DrawTextEx(font,"Stop",(Vector2){370,26},16,0, isStop ? ON_COLOR : OFF_COLOR);
            DrawTextEx(font,"Pause",(Vector2){370,45},16,0, isPause ? ON_COLOR : OFF_COLOR);
            // Shuffle flag
            DrawTextEx(font,"Shuffle",(Vector2){370,64},16,0, isShuffle ? ON_COLOR : OFF_COLOR);
            // Mute flag
            DrawTextEx(font,"Mute",(Vector2){438,8},16,0, isMute ? ON_COLOR:OFF_COLOR);
            // PAN flag
            DrawTextEx(font,"(< PAN >)",(Vector2){438,26},16,0, isPan ? ON_COLOR : OFF_COLOR);
            // scan 10 second of every son in the list
            DrawTextEx(font,"Scan",(Vector2){438,45},16,0, isScan ? ON_COLOR: OFF_COLOR);


            // song of songs
            DrawTextEx(font,TextFormat("%04d ",current_play+1),(Vector2){136,41},16,0, TEXT_COLOR);
            DrawTextEx(font,TextFormat("of %04d",musicFileCount),(Vector2){168,41},16,0, TEXT_COLOR);

            // // seek slider bar    
                float songLength = GetMusicTimeLength(music);
                float sliderSeek = GetMusicTimePlayed(music)/songLength;
                if (isStop < GuiSliderBar((Rectangle){9,screenHeight-38,screenWidth-18,16},NULL,NULL, &sliderSeek,0,1.0f))
                    SeekMusicStream(music, sliderSeek * songLength);

            //statusbar with some info
            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, BLACK); 
            DrawText(TextFormat("version %s", TOOL_VERSION), 64, screenHeight-16, 10, GRAY); 
            DrawText("[Q] exit program.",screenWidth-96, screenHeight-16,10,BLACK);
        EndDrawing();
    }

    free(titleStr);
    UnloadDirectoryFiles(musicFiles);
    UnloadTexture(backGround);
    UnloadRenderTexture(target);
    UnloadFont(font);
    UnloadFont(fontx32);
    DetachAudioMixedProcessor(ProcessAudio);  // Disconnect audio processor
    UnloadMusicStream(music); // Unloaad music stream
    // unload buttons texture 
    for (int i = 0; i < NUM_BUTTONS; i++) UnloadTexture(btnTexture[i]);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

