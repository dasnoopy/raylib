/*******************************************************************************
*
*   raylib MPlayer (a sort of MOD4WIN revival)
*   Small utility to play mp3 files based on Raylib
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************/
        
#define TOOL_NAME               "Raylib Music Player"
#define TOOL_SHORT_NAME         "rmplayer"
#define TOOL_COMMENT            "A Mod4Win clone for Linux written in C using Raylib- Play MP3 and OGG file"
#define TOOL_VERSION            "2.3.7"

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
#include <pthread.h>
#include <sys/stat.h>

// gcc -Wall -Werror rmplayer.c  -o rmplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

// window initial size
#define screenWidth   508
#define screenHeight  279
#define miniScrWidth 367 // 367 mini
#define miniScrHeight 118

// visualizer variables
static float exponent = 1.00f;                 // Audio exponentiation value
static float averageVolume[134] = { 0.0f };   // Average volume history

// Texture variables
#define NUM_BUTTONS 7
#define SEEK_TIME 10.0f
#define NUM_FRAMES  3       // Number of frames (rectangles) for the button sprite texture
#define MAX_FONTS 8

float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
float currentTime = 0.0f;
bool isPlay=false;
bool isStop = true;
bool isPause = false;  
bool isMute = false;
bool isRepeat = false;
bool isID3 = true;
float volume = 1.00f;            // Default audio volume [0.0f..1.0f]
float prev_volume = 0.50f;

// some custom colors
Color bgColor; 
Color textColor;
Color borderColor; // grids color
#define onColor      accentColor
#define offColor     textColor

// Music library
#define MAX_FILEPATH_SIZE       1024
#define FILE_FILTER      ".mp3;.ogg"

// file selection & id3 tag
char ID3tag[1024] = { '\0' };
char titleStr[1024] = { '\0' };
int selectedIndex = 0; // selected song in the file list
int currPlay = 0; //playing song
int prevPlay = 0; //previous played song when shuffle is ON

static FilePathList files;

// define stream 
static Music music;


// config file
#define APP_DIR_NAME "rmplayer"
#define SAVE_FILE_NAME "rmplayer.cfg"
#define PATH_BUF_SIZE 1024

typedef struct Config
{
    bool isPlay;
    bool isShuffle;
    Color accentColor;
    char musicDir[256];
    char titleFnt[256];
    bool dgtEffect;
    bool isMini;
    bool isVumeter;
} Config;

// vumeter
#define MAX_SAMPLES          512
#define NUM_BARS             20 // numero di barre verticali
#define SMOOTHING_FACTOR     0.18f  
#define PI                   3.14159265358979323846f
// Parametri di comportamento del picco Hi-Fi
#define PEAK_HOLD_FRAMES     60     // Quanti frame il picco resta fermo in alto (0.5 secondi a 60 FPS)
#define PEAK_DECAY_SPEED     0.025f // Velocità di discesa del picco dopo l'attesa

typedef struct { float real; float imag; } Complex;

float rawSamples[MAX_SAMPLES] = { 0 };
float barValues[NUM_BARS] = { 0 };
// Array per la gestione del picco massimo stile Hi-Fi
float peakValues[NUM_BARS] = { 0 };
int peakHoldTimers[NUM_BARS] = { 0 };

// Functions
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

// Get path to the score file based on the os.
static void get_config_path(char *buf, size_t len) {
    char dir[PATH_BUF_SIZE];
    const char *xdg = getenv("XDG_DATA_HOME");
    if (xdg && xdg[0]) {
        snprintf(dir, sizeof(dir), "%s/%s", xdg, APP_DIR_NAME);
    } else {
        const char *home = getenv("HOME");
        if (!home)
            home = ".";
        snprintf(dir, sizeof(dir), "%s/.local/share/%s", home, APP_DIR_NAME);
    }
    mkdir(dir, 0755); // fails safely if already exists.
    snprintf(buf, len, "%s/%s", dir, SAVE_FILE_NAME);
}

bool LoadConfig(Config* cfg) {
    char path[PATH_BUF_SIZE];
    get_config_path(path, sizeof(path));
    FILE* fp = fopen(path, "r");
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
            else if (strcmp(key, "isMini") == 0) cfg->isMini = ParseBool(value);
            else if (strcmp(key, "isVumeter") == 0) cfg->isVumeter = ParseBool(value);
        }
        // Style / UI section
        else if (strcmp(currentSection, "style") == 0) {
            if (strcmp(key, "accentColor") == 0) cfg->accentColor = ParseColor(value);
            else if (strcmp(key, "titleFnt") == 0) strncpy(cfg->titleFnt, value, sizeof(cfg->titleFnt) - 1);
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
        snprintf(ID3tag,sizeof(ID3tag), "%s: <empty>", label);
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

void GetTitle (int idx){
    //get ID3 tags
    struct id3_file *file;
    struct id3_tag *tag;
    file = id3_file_open(files.paths[idx], ID3_FILE_MODE_READONLY);
    
    if (!file) fprintf(stderr, "Errore apertura file\n");

    tag = id3_file_tag(file);
    if (isID3) { // show ID3 tag
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
    else { // show file info
        char tmpInfo[64];
        // complete path + filename
        strcpy(titleStr, files.paths[idx]);
        strcat (titleStr, " [ ");
        // file size in bytes
        snprintf(tmpInfo, sizeof(tmpInfo),"%d KBytes",GetFileLength(files.paths[idx])/1024);
        strcat (titleStr, tmpInfo);
        strcat (titleStr, ", ");
        // sample rate
        snprintf(tmpInfo, sizeof(tmpInfo),"%i Hz",music.stream.sampleRate);
        strcat (titleStr, tmpInfo);
        strcat (titleStr, ", ");
        // sample size
        snprintf(tmpInfo, sizeof(tmpInfo),"%i bits",music.stream.sampleSize);
        strcat (titleStr, tmpInfo);
        strcat (titleStr, ", ");
        // channels
        snprintf(tmpInfo, sizeof(tmpInfo),"%i channel (%s)", music.stream.channels, (music.stream.channels == 1)? "mono" : (music.stream.channels == 2)? "stereo" : "multi");
        strcat (titleStr, tmpInfo);
        strcat (titleStr, " ] ");

        }
}

// Funzione che restituisce un FilePathList dei file in basePath con estensioni filter
FilePathList GetMusicFromDirectory(const char *basePath, const char *filter, bool includeSubdirs){
    files = LoadDirectoryFilesEx(basePath, filter, includeSubdirs);
    
    if (files.count == 0) {
        printf("Nessun file trovato in '%s' con filtro '%s'\n", basePath, filter);     
        exit(0);
    }
    
    return files; 
}

void LoadMusicByIndex(int idx, FilePathList files) {
    if (idx < 0 || idx >= files.count) return;
    selectedIndex = idx;
    currPlay = idx;
    music = LoadMusicStream(files.paths[idx]);
    GetTitle(idx);
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

// Struttura FFT In-Place
void FFT(Complex *X, int n) {
    if (n <= 1) return;
    Complex *even = malloc(n / 2 * sizeof(Complex));
    Complex *odd  = malloc(n / 2 * sizeof(Complex));
    for (int i = 0; i < n / 2; i++) {
        even[i] = X[2 * i];
        odd[i]  = X[2 * i + 1];
    }
    FFT(even, n / 2);
    FFT(odd, n / 2);
    for (int k = 0; k < n / 2; k++) {
        float angle = -2.0f * PI * k / n;
        Complex t = {
            .real = cosf(angle) * odd[k].real - sinf(angle) * odd[k].imag,
            .imag = sinf(angle) * odd[k].real + cosf(angle) * odd[k].imag
        };
        X[k] = (Complex){ .real = even[k].real + t.real, .imag = even[k].imag + t.imag };
        X[k + n / 2] = (Complex){ .real = even[k].real - t.real, .imag = even[k].imag - t.imag };
    }
    free(even);
    free(odd);
}

void AudioProcessCallback(void *buffer, unsigned int frames) {
    float *samples = (float *)buffer;
    for (unsigned int i = 0; i < frames && i < MAX_SAMPLES; i++) {
        // Attenuazione preventiva di sicurezza (0.1f) per evitare saturazione hardware
        rawSamples[i] = samples[i * 2] * 0.1f; 
    }
}

int main (int argc, char *argv[]) {
    
    // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI ); // | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "rMPlayer");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

    Image image = LoadImage("assets/background.png");     // Loaded in CPU memory (RAM)
    Texture2D background = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);  

    // Set UI style
    // Custom GUI font loading
    Font titleFnt;
    Font digitFnt= LoadFontEx("fonts/rmdigit.otf", 20, NULL, 0); // all other text
    Font textFnt = LoadFontEx("fonts/PixelOperator.ttf", 16, NULL, 0); // all other text

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
    SetAudioStreamBufferSizeDefault(65535);
    

    // default config value
    Config cfg = {
        .isPlay = false,
        .isShuffle = false,
        .accentColor = {128,255,0,255}, //green
        .musicDir = "/home/public/Music", //default music folder
        .titleFnt = "fonts/rmplayer.otf", // title font
        .dgtEffect = false,
        .isMini = false,
        .isVumeter = true
    };

    //load config from file
    if (!LoadConfig(&cfg)) {
        printf("Errore durante apertura file di configurazione! Verrano usati valori di default.\n");
    }

    // assign  values from config file
    bool isPlay = cfg.isPlay;
    bool isShuffle = cfg.isShuffle;
    bool isMini = cfg.isMini;
    bool isVumeter = cfg.isVumeter;
    Color accentColor = cfg.accentColor;
    char *musicDir = cfg.musicDir;
    titleFnt = LoadFontEx(cfg.titleFnt, 28, NULL, 0);
    bool dgtEffect = cfg.dgtEffect;

    // Load music files
    FilePathList musicFiles = GetMusicFromDirectory(musicDir,FILE_FILTER,true);


            // always start loading a random song
                selectedIndex = isShuffle ? GetRandomValue(0,files.count) : 0;
                LoadMusicByIndex(selectedIndex,musicFiles);
                prevPlay=selectedIndex;

            // auto play song based on cfg setting
            if (isPlay) {
                isStop=false;
                isPause =false;
                PlayMusicStream(music);  // autoplay at start
            }

            // init Audio Processor
            if (isVumeter) AttachAudioMixedProcessor(AudioProcessCallback);
            else  AttachAudioMixedProcessor(ProcessAudio);

            // set FPS (uso questo sistema per regolare la velocità di scorrimento)
            //SetTargetFPS(60);// https://bedroomcoders.co.uk/posts/218

            // scroll title / id3
            Rectangle displayArea = { 8, 8, 352,36 };
            float titleX = displayArea.x ;
            float speed = 60.0f;

            // filelist variables
            Rectangle filesArea = { 8,118,screenWidth-16,139 };
            int rowHeight = 20;
            int visibleRows = 7;
            const int centerRow = 3;
            int scrollOffset = 0;     // primo file visualizzato


            // set colors darker starting from fg color
            Color bgColor = darkenColor(accentColor,0.15f);
            Color textColor = darkenColor(accentColor, 0.60f);
            Color borderColor = darkenColor(accentColor,0.30f);

            // set window size and center on screen
            if (isMini) {
                SetWindowSize(miniScrWidth,miniScrHeight);
                SetWindowPosition(GetMonitorWidth(0) / 2 - miniScrWidth/2, GetMonitorHeight(0) / 2 - miniScrHeight/2);  // center monitor
            }
            else {
                SetWindowSize(screenWidth,screenHeight);
                SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  // center monitor
            }

         //  vumeter
            Complex fftBuffer[MAX_SAMPLES];
            // Parametri dinamici di calibrazione
            float minDb = -55.0f; 
            float maxDb = -0.0f; // sensibilita Decibel
            float maxSeenMagnitude = 0.01f; // Auto-gain tracker

    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);




 while (!WindowShouldClose())
{
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        currentTime = GetMusicTimePlayed(music); //just to simplify some checks
        // set scroll text speed
        Vector2 titleSize = MeasureTextEx(titleFnt, titleStr, 28, 0);
        float titleWidth = titleSize.x;
        bool needScroll = titleWidth > displayArea.width;
        float dt = GetFrameTime();
        if (needScroll) {
            titleX -= (speed * dt);
            if (titleX <= displayArea.x - titleWidth) titleX += titleWidth + displayArea.width;
        }
        // Get normalized time played for current music stream
        // used for the progressbar
        timePlayed = GetMusicTimePlayed(music)/GetMusicTimeLength(music);
        if (timePlayed > 1.0f) timePlayed = 1.0f;   // Make sure time played is no longer than music

        // calculating song times
          char curTimeStr[32]= { '\0' };
          char totTimeStr[32]= { '\0' };
            int hour   = (int)GetMusicTimePlayed(music) / 3600;
            int minute = ((int)GetMusicTimePlayed(music) / 60) % 60;
            int second = (int)GetMusicTimePlayed(music) % 60;
            snprintf(curTimeStr,sizeof(curTimeStr),"%02d:%02d:%02d", hour , minute, second);

            int hours   = (int)GetMusicTimeLength(music) / 3600;
            int minutes = (int)GetMusicTimeLength(music) / 60 % 60;
            int seconds = (int)GetMusicTimeLength(music) % 60;
            snprintf(totTimeStr,sizeof(totTimeStr),"%02d:%02d:%02d", hours, minutes, seconds);

            // clock & vol text size
            Vector2 clockSize = MeasureTextEx(digitFnt, "88:88", 20, 0);
            Vector2 volumeSize = MeasureTextEx(digitFnt, "v 100", 20, 0);

    
        // set initial volume 
        SetMasterVolume(volume);

        // mouse position and update sound stream
        Vector2 mousePos = GetMousePosition();
        UpdateMusicStream(music);   // Update music buffer with new stream data

        // auto move on next song
        if (GetMusicTimePlayed(music) >= (GetMusicTimeLength(music) - 0.450f)) {
                prevPlay = selectedIndex;
                StopMusicStream(music);
                UnloadMusicStream(music);
                    if (isShuffle) {
                        int shuffleIndex = GetRandomValue(0,files.count);
                        //if (shuffleIndex == files.count) --shuffleIndex;
                        if (files.count > 1 && shuffleIndex == selectedIndex)
                            shuffleIndex = (shuffleIndex + 1) % files.count;
                        selectedIndex = shuffleIndex;
                    } else {
                        selectedIndex++;
                        if (selectedIndex >= files.count) selectedIndex = 0;
                    }
                    if (isRepeat) selectedIndex = prevPlay;
                LoadMusicByIndex(selectedIndex,musicFiles);
                PlayMusicStream(music);
                selectedIndex = currPlay;
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
if (!isMini) {// when mini view is active fileselectio is disabled
        //------------------------------------------------------------------------------
        // scrollFiles with mouse
        //------------------------------------------------------------------------------
        //if (CheckCollisionPointRec(mousePos,filesArea)) {
            if (files.count >= visibleRows) {
                        selectedIndex  = -(int)GetMouseWheelMove() + selectedIndex;  
                        if (IsKeyPressed(KEY_DOWN)) selectedIndex++;
                        if (IsKeyPressed(KEY_UP)) selectedIndex--;
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                            if (selectedIndex >= 0 && selectedIndex < files.count) {
                                StopMusicStream(music);
                                //UnloadMusicStream(music);
                                LoadMusicByIndex(selectedIndex,musicFiles);
                                PlayMusicStream(music);
                                //UpdateMusicStream(music);
                                isStop=false;
                                isPlay=true;
                                isPause=false;
                                }
                            }
                    // checks
                            if (selectedIndex < 0) selectedIndex=0;
                            if (selectedIndex > files.count-1) selectedIndex=files.count-1;
                }
}  // all above keybindigs are disable in mini view modo

        if (IsKeyPressed(KEY_I)) {
            isID3 = !isID3;
            GetTitle(currPlay);
        }

        if (IsKeyPressed(KEY_F)) {
            isMini= !isMini;
            if (isMini) SetWindowSize(miniScrWidth,miniScrHeight);
            else SetWindowSize(screenWidth,screenHeight);
        }
        
        if (IsKeyPressed(KEY_S)) isShuffle = !isShuffle;
        if (IsKeyPressed(KEY_R)) isRepeat = !isRepeat;
         
        // Set audio volume
        if (IsKeyDown(KEY_PAGE_DOWN))
        {
            volume -= (volume >= 0.0f) ? 0.01f : 0.0f;
            if (volume < 0.0f) volume = 0.0f, isMute=true;
            SetMasterVolume(volume);
        }
        if (IsKeyDown(KEY_PAGE_UP))
        {
            isMute = false;
            volume += (volume <= 1.0f) ? 0.01f : 0.0f;
            if (volume > 1.0f) volume = 1.0f;
            SetMasterVolume(volume);
        }
        if (IsKeyPressed(KEY_X)) selectedIndex = currPlay;


        if (IsKeyPressed(KEY_M)) // MUTE
        {
            isMute = !isMute;
                if (isMute) {
                    prev_volume = volume;
                    volume = 0.0f;
                    }
                else volume = prev_volume;
        }

        // Restart music playing (stop and play)
        if (IsKeyPressed(KEY_SPACE)) {
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

        if ((btnAction[4] || (IsKeyPressed(KEY_LEFT))) && isPlay) // seek -10sec
        {
                    if (currentTime < 10.0f) {
                        currentTime = 0.0f; 
                        SeekMusicStream(music, 0.0f);
                       // UpdateMusicStream(music);
                    }
                    else SeekMusicStream(music, currentTime - SEEK_TIME);

        }
        if ((btnAction[5] || (IsKeyPressed(KEY_RIGHT))) && isPlay) // seek +10sec
        {
                    if (currentTime + SEEK_TIME >= GetMusicTimeLength(music)) {
                        currentTime = 0.0f;
                        SeekMusicStream(music, 0.0f);
                        //UpdateMusicStream(music);
                    }
                    else  SeekMusicStream(music, currentTime + SEEK_TIME);
        }

        if (btnAction[3] || IsKeyPressed(KEY_P)) { // Previous song : no shuffle on previous song
                if (isShuffle) selectedIndex = prevPlay;
                else selectedIndex--;
                if (selectedIndex < 0) selectedIndex=0;
                StopMusicStream(music);
                UnloadMusicStream(music);
                LoadMusicByIndex(selectedIndex,musicFiles);
                PlayMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
        }

        if (btnAction[6] || IsKeyPressed(KEY_N)) { // Next song based on SHUFFLE setting
            prevPlay = selectedIndex; //save for 1 shot prev.song
            if (isShuffle) {
                int shuffleIndex = GetRandomValue(0,files.count);
                if (shuffleIndex == files.count) --shuffleIndex;
                if (files.count > 1 && shuffleIndex == selectedIndex)
                    shuffleIndex = (shuffleIndex + 1) % files.count;
                selectedIndex = shuffleIndex;
                } 
            else {
                selectedIndex++;
                if (selectedIndex >= files.count) selectedIndex = 0;
            }
                StopMusicStream(music);
                UnloadMusicStream(music);
                LoadMusicByIndex(selectedIndex,musicFiles);
                PlayMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
            }

        if (IsKeyPressed(KEY_ONE)) {
            if (!isMini) SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  // center monitor
            else SetWindowPosition(GetMonitorWidth(0) / 2 - miniScrWidth/2, GetMonitorHeight(0) / 2 - miniScrHeight/2);  // center monitor
        }

        if (IsKeyPressed(KEY_TWO)) {
            if (!isMini) SetWindowPosition(0,GetMonitorHeight(0) - screenHeight); //bottom-left;
            else SetWindowPosition(0,GetMonitorHeight(0) - miniScrHeight); //bottom-left;
        }

        if (IsKeyPressed(KEY_THREE)) {
            if (!isMini) SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2,GetMonitorHeight(0) - screenHeight); //bottom-middle
            else SetWindowPosition(GetMonitorWidth(0) / 2 - miniScrWidth/2,GetMonitorHeight(0) - miniScrHeight); 
        }
        
        if (IsKeyPressed(KEY_FOUR)) {
            if (!isMini) SetWindowPosition(GetMonitorWidth(0) - screenWidth ,GetMonitorHeight(0) - screenHeight); //bottom-right
            else SetWindowPosition(GetMonitorWidth(0) - miniScrWidth ,GetMonitorHeight(0) - miniScrHeight);
        }

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
    
        //-----------------------------------------------------------------------------------------
        // vumeter update
        //-----------------------------------------------------------------------------------------
        // 1. Finestra di Hann ed esecuzione FFT
        for (int i = 0; i < MAX_SAMPLES; i++) {
            float window = 0.5f * (1.0f - cosf(2.0f * PI * i / (MAX_SAMPLES - 1)));
            fftBuffer[i] = (Complex){ .real = rawSamples[i] * window, .imag = 0.0f };
        }

        FFT(fftBuffer, MAX_SAMPLES);

        // 2. Divisione logaritmica e calcolo spettro
        for (int i = 0; i < NUM_BARS; i++) {
            int indexStart = (int)powf(2.0f, (float)i * (log2f(MAX_SAMPLES / 2) / NUM_BARS));
            int indexEnd = (int)powf(2.0f, (float)(i + 1) * (log2f(MAX_SAMPLES / 2) / NUM_BARS));
            
            if (indexEnd <= indexStart) indexEnd = indexStart + 1;
            if (indexEnd > MAX_SAMPLES / 2) indexEnd = MAX_SAMPLES / 2;

            float magnitudeSum = 0.0f;
            int count = 0;

            for (int j = indexStart; j < indexEnd; j++) {
                float mag = sqrtf(fftBuffer[j].real * fftBuffer[j].real + 
                                  fftBuffer[j].imag * fftBuffer[j].imag);
                magnitudeSum += mag;
                count++;
            }

            float averageMagnitude = (count > 0) ? (magnitudeSum / count) : 0.0f;

            // Auto-Gain: Tracciamo il picco più alto mai registrato per scalare i dati di conseguenza
            if (averageMagnitude > maxSeenMagnitude) maxSeenMagnitude = averageMagnitude;
            // Lentamente facciamo decadere il picco massimo per adattarsi a parti più silenziose del brano
            maxSeenMagnitude *= 0.9995f; 

            // Normalizziamo l'ampiezza in base al massimo picco reale registrato
            float normalizedMag = averageMagnitude / maxSeenMagnitude;

            // Calcolo Decibel convertito
            float db = 20.0f * log10f(normalizedMag + 0.00001f);
            
            // Mappatura lineare sui limiti dB
            float targetValue = (db - minDb) / (maxDb - minDb);
            if (targetValue < 0.0f) targetValue = 0.0f;
            if (targetValue > 1.0f) targetValue = 1.0f;

            // Applichiamo lo smoothing per frenare la discesa
            barValues[i] += (targetValue - barValues[i]) * SMOOTHING_FACTOR;

            // Logica del Picco Massimo Hi-Fi
            if (barValues[i] >= peakValues[i]) {
                peakValues[i] = barValues[i];
                peakHoldTimers[i] = PEAK_HOLD_FRAMES; // Resetta il timer di attesa in cima
            } else {
                if (peakHoldTimers[i] > 0) {
                    peakHoldTimers[i]--; // Il picco resta fermo ad aspettare
                } else {
                    peakValues[i] -= PEAK_DECAY_SPEED; // Il picco scende per gravità
                    if (peakValues[i] < barValues[i]) peakValues[i] = barValues[i];
                }
            }
        }

        // do someting whe window loses focus
        if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) SetWindowOpacity(0.5f);
        else SetWindowOpacity(1.0f);




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
            DrawTexture(background, screenWidth/2 - background.width/2, screenHeight/2 - background.height/2, WHITE); // WHITE

            // mini window borders hack
            if (isMini){
            DrawRectangle(miniScrWidth-2,1,2,miniScrHeight,GRAY);
            DrawRectangle(1,miniScrHeight-2,miniScrWidth,2,GRAY);
            DrawLine(0,miniScrHeight,3,miniScrHeight-3,LIGHTGRAY);
            DrawLine(miniScrWidth-3,3,miniScrWidth,0,LIGHTGRAY);
            }
            
            // song title
            BeginScissorMode( (int)displayArea.x, (int)displayArea.y, (int)displayArea.width, (int)displayArea.height);
                if (needScroll) DrawTextEx(titleFnt, titleStr, (Vector2){ titleX, displayArea.y }, 28, 0, accentColor);
                else DrawTextEx(titleFnt, titleStr, (Vector2){ displayArea.x + 2, displayArea.y}, 28,0, accentColor);
            EndScissorMode();

            // Draw buttons bar
                for (int i = 0; i < NUM_BUTTONS; ++i) {
                    DrawRectangle(btnRect[i].x,btnRect[i].y,btnRect[i].width,btnRect[i].height, accentColor); // buttons background for transparency
                    DrawTextureRec(btnTexture[i], srcRect[i], (Vector2){ btnRect[i].x, btnRect[i].y }, WHITE); // Draw button frame // WHITE
                }
            // tempo attuale brano e durata totale brano
            DrawText(totTimeStr,10,46,10, textColor);
            if (dgtEffect) DrawTextEx(digitFnt,"88:88:88",(Vector2){10,58},20,0, borderColor);
            DrawTextEx(digitFnt,curTimeStr,(Vector2){10,58},20,0, accentColor);


            // time since app started
            DrawText(TextFormat("%02i:%02i:%02i",(int) GetTime()/3600, (int) GetTime() / 60 % 60,(int) GetTime() % 60),234-clockSize.x, 46,10,textColor);

            // a sort of visualizer : giusto per vivacizzare....
            BeginScissorMode(223,43,136,40);
                    if (isVumeter) {
                        // draw vumeter
                        float barWidth = (float) 135 / NUM_BARS; // larghezza totale grafico
                        float barSpacing = 1.0f;  // space between bars (direttamente proporzionale a larghezza barre)
                        
                        const int maxSegments = 18; //nr. segmente singola barra
                        const float segmentHeight = 1.0f; //altezza segmento... anche se e' linea 
                        const float segmentGap = 1.0f;   // distanza tra i segmenty 
                        float baseYPos = 81; //base del vumeter

                        for (int i = 0; i < NUM_BARS; i++) {
                            float xPos = 224 + i * barWidth; // posizione X iniziale vumeter

                            int segmentsToLight = (int)(barValues[i] * maxSegments); 
                            int peakSegment = (int)(peakValues[i] * maxSegments) - 1;
                            if (peakSegment < 0 && peakValues[i] > 0.01f) peakSegment = 0;

                            for (int j = 0; j < maxSegments; j++) {
                                float segYPos = baseYPos - (j * (segmentHeight + segmentGap)) - segmentHeight;
                                // Logica di disegno combinata barra + picco
                                bool drawActiveSegment = (j < segmentsToLight);
                                bool drawPeakSegment = (j == peakSegment);
                                DrawLine(xPos, segYPos,xPos +(barWidth - barSpacing), segYPos, (drawActiveSegment || drawPeakSegment) ? accentColor:borderColor );
                           }
                        }
                    }
                    else {
                        // amplitude visualizer 
                            // background grid
                            for (int h = 0; h<4 ; h++) DrawLine(223, 50 + (h*8), 359, 50 + (h*8), borderColor);
                            for (int v = 0; v < 17; v++) DrawLine(227 + (v*8), 43, 226 + (v*8), 81, borderColor);
                            // visualizer
                            for (int i = 0; i < 134; ++i) //cambiare questo valore anche nella funzione relativa
                                DrawLine(225 + i, 80 - (int)(averageVolume[i]*36), 225 + i, 80, accentColor);
                    }
            EndScissorMode();

            // KHz / stereo - mono  of current song
            DrawText(TextFormat("%i kHz",music.stream.sampleRate/1000),105,58,10, accentColor);
            DrawText(TextFormat("%s", (music.stream.channels == 1)? "mono" : (music.stream.channels == 2)? "stereo" : "multi"),105,69,10, accentColor);
            
            //  flags grid
            DrawLine(434,8,434,81,borderColor);
            DrawLine(367,26,500,26,borderColor);
            DrawLine(367,45,500,45,borderColor);
            DrawLine(367,64,500,64,borderColor);

            // progressbar background grid points
                // for (int h = 0; h<131 ; h+=2) 
                //      for (int v = 0; v < 20; v+=2)
                //          DrawPixel(369 + h, 90 + v,borderColor);

            // only progressbar
            DrawRectangleRec((Rectangle){369,90, 128 * timePlayed, 19}, accentColor);  // riempimento
            //for (int i = 0; i < (timePlayed * 130); i+=4) DrawRectangleLinesEx((Rectangle){368+i,90,3,19},2,accentColor);

            //volume value
               DrawTextEx(digitFnt,TextFormat("%03.f v",volume*100),(Vector2){214-volumeSize.x,58}, 20,0, accentColor);

            // PLAY flag
            DrawRectangle(368,27,64,16,isPlay ? onColor:bgColor);
            DrawTextEx(textFnt,"Play",(Vector2){370,26},16,0, isPlay ? bgColor : offColor);

            // STOP flag
            DrawRectangle(368,9,64,15,isStop ? onColor:bgColor);
            DrawTextEx(textFnt,"Stop",(Vector2){370,8},16,0, isStop ? bgColor : offColor);

            // PAUSE flag
            DrawRectangle(368,46,64,16,isPause ? onColor:bgColor);
            DrawTextEx(textFnt,"Pause",(Vector2){370,45},16,0, isPause ? bgColor : offColor);

            // Shuffle flag
            DrawRectangle(435,9,64,15,isShuffle ? onColor:bgColor);
            DrawTextEx(textFnt,"Shuffle",(Vector2){438,8},16,0, isShuffle ? bgColor : offColor);
            
            // Mute flag
            DrawRectangle(435,46,64,16,isMute ? onColor:bgColor);
            DrawTextEx(textFnt,"Mute",(Vector2){438,45},16,0, isMute ? bgColor:offColor);
            
            // REPEAT flag
            DrawRectangle(435,27,64,16,isRepeat ? onColor:bgColor);
            DrawTextEx(textFnt,"Repeat",(Vector2){438,26},16,0, isRepeat ? bgColor : offColor);

            // INFO flag
            DrawRectangle(368,65,64,15,!isID3 ? onColor:bgColor);
            DrawTextEx(textFnt, "Info",(Vector2){370,64},16,0, !isID3 ? bgColor : offColor);

    if (!isMini) { // draw all control when mini window is disabled
            // file selection
            {
              BeginScissorMode( (int)filesArea.x, (int)filesArea.y, (int)filesArea.width, (int)filesArea.height);
                if (files.count < visibleRows) visibleRows = files.count;

                scrollOffset = selectedIndex - centerRow;
                if (scrollOffset < 0) scrollOffset = 0;
                int maxOffset = files.count - visibleRows;
                if (maxOffset < 0) maxOffset = 0;
                if (scrollOffset > maxOffset) scrollOffset = maxOffset;

                    for (int i = 0; i < visibleRows; ++i) {
                        DrawLineDashed((Vector2){filesArea.x, filesArea.y + (i*rowHeight)}, (Vector2){screenWidth-8, filesArea.y +(i*rowHeight)},1,1,textColor);
                        //if (i % 2) DrawRectangleRec((Rectangle){filesArea.x+1,filesArea.y +(i*rowHeight),filesArea.width-2,rowHeight-1}, darkenColor(textColor,0.42f));
                        int fileIndex = scrollOffset + i;
                        if (fileIndex > files.count) break;
                        if (fileIndex == selectedIndex) DrawRectangle(filesArea.x,filesArea.y +(i*rowHeight),filesArea.width,rowHeight-1, onColor);
                        DrawTextEx(textFnt,TextFormat("%04i\t%s",fileIndex + 1,GetFileName(files.paths[fileIndex])),(Vector2){filesArea.x + 2, filesArea.y +(i*rowHeight)+1},16,0,(fileIndex == selectedIndex)? bgColor : textColor);
                        }   
                //vertical divider
                DrawLine(42,filesArea.y,42,filesArea.y + filesArea.height,borderColor);
                EndScissorMode();
            }
        }

            //statusbar with some info
            DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, BLACK); 

            DrawText(TextFormat("version %s", TOOL_VERSION), 64, screenHeight-16, 10, GRAY); 
            DrawText(TextFormat("%04d of %04d",currPlay + 1, files.count),(screenWidth/2)-24,screenHeight-16,10,BLACK );
            DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);
            //DrawFPS(screenWidth-100,screenHeight-50);
        EndDrawing();
    }
    
    //unload reosurce
    UnloadDirectoryFiles(musicFiles);
    UnloadImage(image);
    UnloadTexture(background);
    UnloadRenderTexture(target);
    DetachAudioMixedProcessor(ProcessAudio);  // Disconnect audio processor // visulizer
    DetachAudioMixedProcessor(AudioProcessCallback); // vumeter
    UnloadMusicStream(music); // Unloaad music stream
    // unload fonts
    UnloadFont(titleFnt);
    UnloadFont(textFnt);
    UnloadFont(digitFnt);
    // unload buttons texture 
    for (int i = 0; i < NUM_BUTTONS; ++i) UnloadTexture(btnTexture[i]);
    
    //cleanup
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
