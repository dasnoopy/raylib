/*******************************************************************************************
*
*   raylib MPlayer (a sort of MOD4WIN revival)
*   Small utility to play mp3 files based on Raylib
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************
* 
* gestion errori / problemi apertura file... eg se cambio nome ad un file mp3 quando il programma si incazza?
* gestione errore se tag mp3 hanno problemi o mancano
* 
*******************************************************************************************/

#define TOOL_NAME               "Raylib Music Player"
#define TOOL_SHORT_NAME         "rmplayer"
#define TOOL_COMMENT            "A Mod4Win clone for Linux written in C using Raylib- Play MP3 and OGG file"
#define TOOL_VERSION            "1.3.5"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <id3tag.h>  
#include <unistd.h>

// gcc -Wall -Werror rmplayer.c  -o rmplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

typedef struct Config
{
    bool isPlay;
    bool isShuffle;
    Color accentColor;
    char musicDir[256];
    char titleFnt[256];
    char digitFnt[256];
    bool dgtEffect;
} Config;

// window initial size
#define screenWidth   540
#define screenHeight  156

// initial window position
int screenX = 32;
int screenY = 880;

// visualizer variables
static float exponent = 0.88f;                 // Audio exponentiation value
static float averageVolume[134] = { 0.0f };   // Average volume history

// Texture variables
#define NUM_BUTTONS 7
#define SEEK_TIME 10.0f
#define NUM_FRAMES  3       // Number of frames (rectangles) for the button sprite texture
#define MAX_FONTS 8

// define stream 
Music music;

float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
float currentTime = 0.0f;
bool isStop = true;
bool isPause = false;  
bool isMute = false;
bool isPan = false;
bool isRepeat = false;
float pan = 0.0f;               // Default audio pan center [-1.0f..1.0f]
float volume = 0.50f;            // Default audio volume [0.0f..1.0f]
float prev_volume = 0.50f;

// some custom colors
Color bgColor; 
Color textColor;
Color borderColor; // grids color
#define onColor      accentColor // CLITERAL(Color){ 0, 255, 0, 255}
#define offColor     textColor //CLITERAL(Color){ 0,64, 0,255}

// Music library
#define MAX_FILEPATH_SIZE       1024
#define FILE_FILTER      ".mp3;.ogg"

char ID3tag[1024] = { '\0' };
char titleStr[1024] = { '\0' };
int musicFileCount = 0;
int current_play = 0;
int prevPlay = 0;
int selectedFile = -1;

static char *Trim(char *str)
{
    while (isspace((unsigned char)*str)) str++;
        if (*str == 0) return str;
        char *end = str + strlen(str) - 1;
        while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static bool ParseBool(const char *value) {
    return (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0);
}

static Color ParseColor(const char *value) {
    Color color = WHITE;
    int r, g, b, a;

    if (sscanf(value, "%d,%d,%d,%d",
               &r, &g, &b, &a) == 4)
    {
        color.r = (unsigned char)r;
        color.g = (unsigned char)g;
        color.b = (unsigned char)b;
        color.a = (unsigned char)a;
    }
    return color;
}

Color DarkenColor(Color color, float factor)
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

bool LoadConfig(const char* filename, Config* cfg) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return false;

    char line[512];
    char currentSection[64] = "";

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = Trim(line);
        if (*trimmed == '\0' || *trimmed == '#' || *trimmed == ';') continue;

        if (trimmed[0] == '[') {
            char* end = strchr(trimmed, ']');
            if (end) {
                size_t len = end - trimmed - 1;
                strncpy(currentSection, trimmed + 1, len);
                currentSection[len] = '\0';
            }
            continue;
        }

        char* eq = strchr(trimmed, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = Trim(trimmed);
        char* value = Trim(eq + 1);

        // Player section
        if (strcmp(currentSection, "player") == 0) {
            if (strcmp(key, "isPlay") == 0) cfg->isPlay = ParseBool(value);
            else if (strcmp(key, "isShuffle") == 0) cfg->isShuffle = ParseBool(value);
            else if (strcmp(key, "musicDir") == 0) strncpy(cfg->musicDir, value, sizeof(cfg->musicDir) - 1);
        }
        // Style / UI section
        else if (strcmp(currentSection, "style") == 0) {
            if (strcmp(key, "accentColor") == 0) cfg->accentColor = ParseColor(value);
            else if (strcmp(key, "titleFnt") == 0) strncpy(cfg->titleFnt, value, sizeof(cfg->titleFnt) - 1);
            else if (strcmp(key, "digitFnt") == 0) strncpy(cfg->digitFnt, value, sizeof(cfg->digitFnt) - 1);
            else if (strcmp(key, "dgtEffect") == 0) cfg->dgtEffect = ParseBool(value);
        }
    }
    fclose(fp);
    return true;
}


static void getID3tags(struct id3_tag *tag, const char *id, const char *label)
{
    struct id3_frame *frame;
    union id3_field *field;
    id3_ucs4_t const *ucs4;
    id3_utf8_t *utf8;

    frame = id3_tag_findframe(tag, id, 0);
    if (!frame) {
        snprintf(ID3tag,sizeof(ID3tag), "%s: n/a", label);
        return;
    }

    field = &frame->fields[1];
    ucs4 = id3_field_getstrings(field, 0);
    if (!ucs4) {
        snprintf(ID3tag,sizeof(ID3tag), "%s: <empty>", label);
        return;
    }
    utf8 = id3_ucs4_utf8duplicate(ucs4);
    if (!utf8) {
        snprintf(ID3tag,sizeof(ID3tag),"%s: <conversion error>", label);
        return;
    }
    snprintf(ID3tag, sizeof(ID3tag), "%s", utf8);
    free(utf8);
}

// Funzione che restituisce un FilePathList dei file in basePath con estensioni filter
FilePathList GetMusicFromDirectory(const char *basePath, const char *filter, bool includeSubdirs){
    FilePathList files = LoadDirectoryFilesEx(basePath, filter, includeSubdirs);
    if (files.count == 0) printf("Nessun file trovato in '%s' con filtro '%s'\n", basePath, filter);
    else musicFileCount = files.count;
    return files; 
}

void LoadMusicByIndex(int idx, FilePathList files) {
    if (idx < 0 ) idx=0;
    else if (idx >= musicFileCount) idx = musicFileCount-1;

    current_play = idx;
    music = LoadMusicStream(files.paths[idx]);
    
    //get ID3 tags
    struct id3_file *file;
    struct id3_tag *tag;
    file = id3_file_open(files.paths[idx], ID3_FILE_MODE_READONLY);
    
    if (!file) {
        fprintf(stderr, "Errore apertura file\n");
    }

    tag = id3_file_tag(file);
            
    getID3tags(tag, "TPE1", "Artist");
    strcpy(titleStr, ID3tag );
    strcat (titleStr, " - ");
    getID3tags(tag, "TIT2", "Title");
    strcat(titleStr, ID3tag );
    //strcat(titleStr, "\0");
    strcat (titleStr, " [");
    getID3tags(tag, "TDRC", "Year");
    strcat(titleStr, ID3tag );
    strcat(titleStr, "]\0");
    id3_file_close(file);
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
    // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST); // | FLAG_WINDOW_TOPMOST);
    InitWindow(screenWidth, screenHeight, "rMPlayer");
    // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  // center monitor
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

    Image image = LoadImage("assets/background.png");     // Loaded in CPU memory (RAM)
    Texture2D background = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
    //SetTextureFilter(background, TEXTURE_FILTER_BILINEAR);  // Texture scale filter to use
      
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // Set UI style
    // Custom GUI font loading
    Font titleFnt;
    Font digitFnt;
    Font textFnt;
    textFnt = LoadFontEx("fonts/PixelOperator.ttf", 16, NULL, 0); // all other text

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
    SetAudioStreamBufferSizeDefault(4096);
    AttachAudioMixedProcessor(ProcessAudio);
    
    // default config value
    Config cfg = {
        .isPlay = false,
        .isShuffle = false,
        .accentColor = {255,255,255,255}, //green
        .musicDir = "/home/public/Music", //default music folder
        .titleFnt = "fonts/macano.otf", // title font
        .digitFnt = "fonts/segment-lcd.otf", // title font
        .dgtEffect= false
    };

    //load config from file
    if (!LoadConfig("rmplayer.cfg", &cfg)) {
        printf("Errore durante apertura file di configurazione! Verrano usati valori di default.\n");
    }

    // assign  values from config file
    bool isPlay = cfg.isPlay;
    bool isShuffle = cfg.isShuffle;
    Color accentColor = cfg.accentColor;
    char *musicDir = cfg.musicDir;
    titleFnt = LoadFontEx(cfg.titleFnt, 28, NULL, 0);
    digitFnt = LoadFontEx(cfg.digitFnt, 20, NULL, 0);
    bool dgtEffect = cfg.dgtEffect;

    // Load music files
    FilePathList musicFiles = GetMusicFromDirectory(musicDir,FILE_FILTER,true);
    
    // always start with a random song
        current_play = GetRandomValue(0,musicFileCount);
        LoadMusicByIndex(current_play,musicFiles);
        prevPlay=current_play;

    // auto start song on open
    if (isPlay) {
        isStop=false;
        isPause =false;
        PlayMusicStream(music);  // autoplay at start
    }

    // set FPS (uso questo sistema per regolare la velocità di scorrimento)
    // SetTargetFPS(60);// https://bedroomcoders.co.uk/posts/218

    // scroll title / id3
    selectedFile = current_play;
    Rectangle displayArea = { 9, 8, 349,30 };
    float titleX = displayArea.x ;
    float speed = 60.0f;
    
    // set colors darker starting from fg color
    Color bgColor = DarkenColor(accentColor,0.15f);
    Color textColor = DarkenColor(accentColor, 0.60f);
    Color borderColor = DarkenColor(accentColor,0.30f);

    // just a test
    //system("echo $HOME");

    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);

 while (!WindowShouldClose())
{
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------

        // set scroll text speed
        Vector2 titleSize = MeasureTextEx(titleFnt, titleStr, 28, 0);
        float titleWidth = titleSize.x;
        bool needScroll = titleWidth > displayArea.width;
        float dt = GetFrameTime();
        if (needScroll) {
            titleX -= (speed * dt);
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


// ***********************************************************
// Keybindigs
//  
// cursor up/down : Volume UP / DOWN
// cursor left/right : Panning audio left/Right
// C : center PAN
// M : Mute audio
// S : Shuffle playlist on / off
// R : repeat song on / off
// N : play next song
// P : play previous song 
// Q : leave app
// 1 : place window on center screen
// 2 : place window bottom-left
// 3 : place window bottom-middle
// 4 : place window bottom-right
// SPACE : play/stop song
// ***********************************************************
        
        // Set audio pan
        if (IsKeyDown(KEY_LEFT))
        {
            isPan = true;
            pan -= 0.01f;
            if (pan < -1.0f) pan = -1.0f;
            SetMusicPan(music, pan);
        }
        else if (IsKeyDown(KEY_RIGHT))
        {
            isPan = true;
            pan += 0.01f;
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
       
        else if (IsKeyPressed(KEY_R)) isRepeat = !isRepeat;
         
        // Set audio volume
        else if (IsKeyDown(KEY_DOWN))
        {
            volume -= (volume >= 0.0f) ? 0.01f : 0.0f;
            if (volume < 0.0f) volume = 0.0f, isMute=true;
            SetMasterVolume(volume);
        }
        else if (IsKeyDown(KEY_UP))
        {
            isMute = false;
            volume += (volume <= 1.0f) ? 0.01f : 0.0f;
            if (volume > 1.0f) volume = 1.0f;
            SetMasterVolume(volume);
        }
        
        else if (IsKeyPressed(KEY_M)) // MUTE
        {
            isMute = !isMute;
                if (isMute) {
                    prev_volume = volume;
                    volume = 0.0f;
                    }
                else volume = prev_volume;
        }

        // Restart music playing (stop and play)
        else if (IsKeyPressed(KEY_SPACE)) {
                if (!isStop) {
                    isStop=true;
                    isPlay=false;
                    isPause=false;
                    StopMusicStream(music);
                    }
                else {
                     isStop=false;
                     isPlay=true;
                     isPause=false;
                     PlayMusicStream(music);
                    }
            }
        
        if (btnAction[0]) { // Stop button
                //ResumeMusicStream(music);
                StopMusicStream(music);
                //UpdateMusicStream(music);
                isStop=true;
                isPlay=false;
                isPause=false;
                }

        if (btnAction[1]) { // play button
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
                    if (currentTime < 10.0f) {
                        currentTime = 0.0f; 
                        SeekMusicStream(music, 0.0f);
                       // UpdateMusicStream(music);
                    }
                    else SeekMusicStream(music, currentTime - SEEK_TIME);

        }
        if (btnAction[5] && isPlay) // seek +10sec
        {
                    if (currentTime + SEEK_TIME >= GetMusicTimeLength(music)) {
                        currentTime = 0.0f;
                        SeekMusicStream(music, 0.0f);
                        //UpdateMusicStream(music);
                    }
                    else  SeekMusicStream(music, currentTime + SEEK_TIME);
        }

        if (btnAction[3] || IsKeyPressed(KEY_P)) { // Previous song : no shuffle on previous song
                if (isShuffle) current_play = prevPlay;
                else current_play--;
                if (current_play<=0) current_play=0;
                selectedFile = current_play;
                StopMusicStream(music);
                LoadMusicByIndex(current_play,musicFiles);
                PlayMusicStream(music);
                //UpdateMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
                //titleX = displayArea.x;
        }

        if (btnAction[6] || IsKeyPressed(KEY_N)) { // Next song based on SHUFFLE setting
            prevPlay = current_play; //save for 1 shot prev.song
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
                //UnloadMusicStream(music);
                LoadMusicByIndex(current_play,musicFiles);
                PlayMusicStream(music);
                //UpdateMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
                //titleX = displayArea.x;
            }

        if (IsKeyPressed(KEY_ONE)) SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  // center monitor
        if (IsKeyPressed(KEY_TWO)) SetWindowPosition(0,GetMonitorHeight(0) - screenHeight); //bottom-left
        if (IsKeyPressed(KEY_THREE)) SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2,GetMonitorHeight(0) - screenHeight); //bottom-middle
        if (IsKeyPressed(KEY_FOUR)) SetWindowPosition(GetMonitorWidth(0) - screenWidth ,GetMonitorHeight(0) - screenHeight); //bottom-right
        
        // set toolbar button status in stop, play, pause
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


        // auto move on next song
        if (GetMusicTimePlayed(music) >= GetMusicTimeLength(music) - 0.05f)
            {
                prevPlay = current_play;
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
                    if (isRepeat) current_play = prevPlay;

                selectedFile = current_play;
                LoadMusicByIndex(current_play,musicFiles);
                PlayMusicStream(music);
            }
    
        // set initial volume and panning
        SetMusicPan(music, pan);
        SetMasterVolume(volume);
    
        // Get normalized time played for current music stream
        currentTime = GetMusicTimePlayed(music); //just to simplify some checks
        timePlayed = GetMusicTimePlayed(music)/GetMusicTimeLength(music);
        if (timePlayed > 1.0f) timePlayed = 1.0f;   // Make sure time played is no longer than music

        // //do someting whe window loses focus
        // if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) SetWindowOpacity(0.5f);
        // else SetWindowOpacity(1.0f);

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);
            ClearBackground(BLACK);
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            
            // Draw background at first
            DrawRectangle(0,0,screenWidth,screenHeight,bgColor);
            
            //load player background image
            DrawTexture(background, screenWidth/2 - background.width/2, screenHeight/2 - background.height/2, WHITE);

            // grid flags
            DrawLine(434,8,434,81,borderColor);
            DrawLine(367,26,500,26,borderColor);
            DrawLine(367,45,500,45,borderColor);
            DrawLine(367,64,500,64,borderColor);

            // visualizer grid
            for (int h = 0; h<4 ; h++) DrawLine(224, 50 + (h*8), 357, 50 + (h*8), borderColor);
            for (int v = 0; v < 17; v++) DrawLine(227 + (v*8), 44, 227 + (v*8), 79, borderColor);

            //volume slider
            DrawRectangleLinesEx((Rectangle){507,(int)105-(volume*97),25,6},2,accentColor);
            DrawTextEx(textFnt,"Max",(Vector2){508,8},16,0, textColor);
            DrawTextEx(textFnt,"Min",(Vector2){508,94},16,0, textColor);
            DrawTextEx(textFnt,TextFormat("Vol.: %02.f",volume*100),(Vector2){438,64},16,0,accentColor);

            // pan slider
            DrawRectangleLinesEx((Rectangle){(int)(368 + (pan + 1.0f)/2.0f*124), 88, 6, 23},2,accentColor);
            DrawTextEx(textFnt,"Left",(Vector2){370,90},16,0, textColor);
            DrawTextEx(textFnt,"Right",(Vector2){464,90},16,0, textColor);

            // song title
            BeginScissorMode( (int)displayArea.x, (int)displayArea.y, (int)displayArea.width, (int)displayArea.height);
                if (needScroll) DrawTextEx(titleFnt, titleStr, (Vector2){ titleX, displayArea.y }, 28, 0, accentColor);
                else DrawTextEx(titleFnt, titleStr, (Vector2){ displayArea.x, displayArea.y}, 28,0, accentColor);
            EndScissorMode();

            // Draw buttons bar
                for (int i = 0; i < NUM_BUTTONS; ++i) {
                    DrawRectangle(btnRect[i].x,btnRect[i].y,btnRect[i].width,btnRect[i].height, accentColor);
                    DrawTextureRec(btnTexture[i], srcRect[i], (Vector2){ btnRect[i].x, btnRect[i].y }, WHITE); // Draw button frame
                }
            // tempo attuale brano e durata totale brano
            //DrawTextEx(textFnt,"Hour:Min:Sec",(Vector2){10,41},16,0, textColor);
            char timeStr[32];
            int hour   = (int)GetMusicTimePlayed(music) / 3600;
            int minute = ((int)GetMusicTimePlayed(music) / 60) % 60;
            int second = (int)GetMusicTimePlayed(music) % 60;
            snprintf(timeStr,sizeof(timeStr),"%02d:%02d:%02d", hour , minute, second);
            if (dgtEffect) DrawTextEx(digitFnt,"88:88:88",(Vector2){10,58},20,0, textColor);
            DrawTextEx(digitFnt,timeStr,(Vector2){10,58},20,0, accentColor);
            int hours   = ((int)GetMusicTimeLength(music) -(int)GetMusicTimePlayed(music)) / 3600;
            int minutes = ((int)GetMusicTimeLength(music)- (int)GetMusicTimePlayed(music)) / 60 % 60;
            int seconds = ((int)GetMusicTimeLength(music)-(int)GetMusicTimePlayed(music)) % 60;
            snprintf(timeStr,sizeof(timeStr),"-%02d:%02d:%02d", hours, minutes, seconds);
            DrawTextEx(textFnt,timeStr,(Vector2){10,41},16,0, textColor);

            // clock
            time_t now = time (NULL);
            struct tm *t = localtime(&now);
            Vector2 clockSize = MeasureTextEx(digitFnt, "88:88", 20, 0);
            if (dgtEffect) DrawTextEx(digitFnt,"88:88",(Vector2){215-clockSize.x, 58}, 20,0, textColor);
            DrawTextEx(digitFnt,TextFormat("%02i:%02i", t->tm_hour, t->tm_min),(Vector2){215-clockSize.x, 58}, 20,0, accentColor);

            // a sort of visualizer : giusto per vivacizzare....
            for (int i = 0; i < 134; ++i) //cambiare questo valore anche nella funzione relativa
                DrawLine(225 + i, 80 - (int)(averageVolume[i]*32), 225 + i, 80, accentColor);

            // show Play / Stop /Pause status
            DrawRectangle(368,27,64,16,isPlay ? accentColor:bgColor);
            DrawTextEx(textFnt,"Play",(Vector2){370,26},16,0, isPlay ? bgColor : textColor);

            DrawRectangle(368,9,64,15,isStop ? accentColor:bgColor);
            DrawTextEx(textFnt,"Stop",(Vector2){370,8},16,0, isStop ? bgColor : textColor);

            DrawRectangle(368,46,64,16,isPause ? accentColor:bgColor);
            DrawTextEx(textFnt,"Pause",(Vector2){370,45},16,0, isPause ? bgColor : textColor);

            // Shuffle flag
            DrawRectangle(435,9,64,15,isShuffle ? accentColor:bgColor);
            DrawTextEx(textFnt,"Shuffle",(Vector2){438,8},16,0, isShuffle ? bgColor : textColor);
            
            // Mute flag
            DrawRectangle(435,27,64,16,isMute ? accentColor:bgColor);
            DrawTextEx(textFnt,"Mute",(Vector2){438,26},16,0, isMute ? bgColor:textColor);
            
            // PAN flag
            DrawTextEx(textFnt,"(< Pan >)",(Vector2){438,45},16,0, isPan ? onColor : offColor);
            
            // REPEAT flag
            DrawRectangle(368,65,64,15,isRepeat ? accentColor:bgColor);
            DrawTextEx(textFnt,"Repeat",(Vector2){370,64},16,0, isRepeat ? bgColor: textColor);

            // song of songs
            DrawTextEx(textFnt,TextFormat("%04d of %04d",current_play + 1 , musicFileCount),(Vector2){136,41},16,0, textColor);

            // only progressbar
            DrawRectangleRec((Rectangle){9,screenHeight-37, (int)((screenWidth-18) * timePlayed), 14}, accentColor);  // riempimento

            //statusbar with some info
            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, BLACK); 
            DrawText(TextFormat("version %s", TOOL_VERSION), 64, screenHeight-16, 10, GRAY); 
            DrawText(TextFormat("%i Hz", music.stream.sampleRate),194, screenHeight-16,10,BLACK);
            DrawText(TextFormat("/ %i bits", music.stream.sampleSize),242, screenHeight-16,10,BLACK);
            DrawText(TextFormat("/ %i channel (%s)", music.stream.channels, (music.stream.channels == 1)? "Mono" : (music.stream.channels == 2)? "Stereo" : "Multi"),292, screenHeight-16,10,DARKGRAY);     
            DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);
        
        EndDrawing();
    }

    UnloadDirectoryFiles(musicFiles);
    UnloadImage(image);
    UnloadTexture(background);
    UnloadRenderTexture(target);
    DetachAudioMixedProcessor(ProcessAudio);  // Disconnect audio processor
    UnloadMusicStream(music); // Unloaad music stream
    // unload fonts
    UnloadFont(titleFnt);
    UnloadFont(textFnt);
    UnloadFont(digitFnt);
    // unload buttons texture 
    for (int i = 0; i < NUM_BUTTONS; ++i) UnloadTexture(btnTexture[i]);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

