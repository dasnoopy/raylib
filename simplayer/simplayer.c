/*******************************************************************************
*
*   raylib simple music player
*   Small utility to play mp3 musicFiles based on Raylib
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************/
        
#define TOOL_NAME               "Simple Music Player"
#define TOOL_SHORT_NAME         "simplayer"
#define TOOL_COMMENT            "Simple but modern music player written in C99 using Raylib - Play MP3 and OGG file"
#define TOOL_VERSION            "1.6.3"

#include <stdio.h>
#include <time.h>
#include <raylib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>

// gcc -Wall -Werror simplayer.c  -o simplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

// window initial size
#define screenWidth   640
#define screenHeight  408

// Texture variables
#define SEEK_TIME 10.0f // seek time
float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
float currentTime = 0.0f;
bool isPlay = true;
bool isShuffle = true;
bool isStop = true;
bool isPause = false;  
bool isMute = false;
bool isVisFiles = false; // false:default : show visualizer , True show file selection
float volume = 0.80f;            // Default audio volume [0.0f..1.0f]
float prev_volume = 0.80f;
char *musicDir = "/home/andrea/Music";

// Music library && files management
#define FILTER_MP3      ".mp3"
#define FILTER_OGG      ".ogg"

size_t fileCount;
#define MAX_FILES 4096
#define MAX_NAME 1024
char files[MAX_FILES][MAX_NAME];

// barra path su Linux oppure su WIN
#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

// file selection
char titleStr[1024] = { '\0' };
char artistStr[1024] = { '\0' };
int selectedIndex = 0; // selected song in the file list
int currPlay = 0; //playing song
int prevPlay = 0; //previous played song when shuffle is ON


// define stream 
static Music music;

// config file
#define APP_DIR_NAME "simplayer"
#define PATH_BUF_SIZE 1024

#define MAX_COLORS_COUNT    12          // Number of colors available (BLACK & WHITE are excluded)
Color colors[MAX_COLORS_COUNT] = { ORANGE, RED, MAROON, GOLD, YELLOW, BLUE, SKYBLUE, LIME, GREEN, PINK, PURPLE, VIOLET};

// vumeter
#define MAX_SAMPLES          512
#define NUM_BARS             36 // numero di barre verticali
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

void extractArtistAndTitle(char *string, char **artist, char **title)
 {
    char *separator;

    if ((string == NULL) || (artist == NULL) || (title == NULL))
        return;
    separator = strstr(string, " - ");
    if (separator != NULL)
     {
        size_t length;

        length  = separator - string;
        *artist = malloc(1 + length);
        if (*artist != NULL)
        {
            memcpy(*artist, string, length);
           (*artist)[length] = '\0';
        }
        *title = strdup(separator + 3);
     }
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

void drawRectangleRounded (int X, int Y, int W, int H, Color color)  {
  Rectangle  rect = { X, Y, W, H};   // toplx, toply, width, height
  float radius = 0.046;                        // rotate degrees
  int     segs = 12;
  DrawRectangleRounded ( rect, radius, segs, color );
}

void GetTagsFromFilename(int idx){
// get Artist and Title song from filename (format :  "artist name - song titile.mp3")
        char buffer[512];
        char *artist;
        char *title;

        strcpy (buffer, GetFileNameWithoutExt(files[idx]));
        extractArtistAndTitle(buffer, &artist, &title);

            if (artist != NULL) {
                strcpy(artistStr, artist );
                strcat(artistStr, "\0");
            }
            if (title != NULL){
                strcpy(titleStr, title );
                strcat(titleStr, "\0");
            }
        free(artist);
        free(title);
    }


void LoadMusicByIndex(int idx) {
    currPlay = idx;
    music = LoadMusicStream(files[idx]);
    GetTagsFromFilename(idx);
}

//------------------------------------------------------------------------------------
// Audio processing function
//------------------------------------------------------------------------------------

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

int compare_files(const void *a, const void *b) {
    const char *fa = (const char *)a;
    const char *fb = (const char *)b;
    return strcmp(fa, fb); // case sensitive
    //return strcasecmp((const char *)fa, (const char *)fb); // no case sensitive
}

int load_files_recursive(const char *path, char files[MAX_FILES][MAX_NAME], int count)
{

    if (!DirectoryExists(path)) {
        // Failed to locate, write the error and kill the program.
        TraceLog(LOG_ERROR, "Failed to locate: %s directory; exit program!", musicDir);
        exit(1);
    }   

    DIR *dir = opendir(path);
    
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < MAX_FILES) {
        const char *name = entry->d_name;

        // salta "." e ".."
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, SEPARATOR, name);

        struct stat st;
        if (stat(fullpath, &st) == -1) continue;

        // se directory procedi con ricorsione
        if (S_ISDIR(st.st_mode)) count = load_files_recursive(fullpath, files, count);
        // se è file → controlla estensione
        else if (S_ISREG(st.st_mode)) {
            const char *ext = strrchr(name, '.');

            if (ext && ( strcmp(ext, FILTER_MP3) == 0  || strcmp(ext, FILTER_OGG) == 0)) {
                 int written = snprintf(files[count], MAX_NAME, "%s", fullpath);
                if (written >= 0 && written < MAX_NAME) count++;
            }
        }
    }
    closedir(dir);
    // sort files list
    qsort(files, count, MAX_NAME, compare_files);  // ordinamento file
    return count;
}


int main (int argc, char *argv[]) 
{

    // Set configuration flags for window creation
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TRANSPARENT); // | FLAG_WINDOW_TOPMOST); 

    InitWindow(screenWidth, screenHeight, "simplayer");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works

    // Set UI style
    // Custom GUI font loading
    Font titleFnt = LoadFontEx("fonts/ManropeV5-Bold.otf", 36, NULL, 0);
    Font artistFnt = LoadFontEx("fonts/ManropeV5-Regular.otf", 28, NULL, 0);
    Font defaultFnt = LoadFontEx("fonts/DINPro-Medium.otf", 18, NULL, 0);
    Font digitFnt= LoadFontEx("fonts/SF-Mono-Semibold.ttf", 24, NULL, 0); 
    Font filesFnt= LoadFontEx("fonts/ManropeV5-Regular.otf", 20, NULL, 0); 

    SetTextureFilter(digitFnt.texture, TEXTURE_FILTER_BILINEAR);
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);  
    
    //load volume icon
    Image volumeICO = LoadImage("assets/volumeICO.png");     // Loaded in CPU memory (RAM)
    Texture2D volumeICO_texture = LoadTextureFromImage(volumeICO);          // Image converted to texture, GPU memory (VRAM)
    UnloadImage(volumeICO);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM
    
    Image image = LoadImage("assets/background.png");     // Loaded in CPU memory (RAM)
    Texture2D background = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
    UnloadImage(image); 

    // init Audio
    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(65535);
    
// some custom colors
Color accentColor = SKYBLUE; // colors[GetRandomValue(0,MAX_COLORS_COUNT-1)]; // choose a random color from RAAYLIB color table
Color primaryColor =  WHITE;
Color secondaryColor = CLITERAL(Color){176, 178, 178, 255};;
Color borderColor = CLITERAL(Color){60, 70, 80, 255};
//Color bgColor = CLITERAL(Color){10, 20, 30, 255};

// file open/save variables

            // load music files into array
            int fileCount = load_files_recursive(musicDir, files, 0);

            // load first song to play  based on shuffle setting
                selectedIndex = isShuffle ? GetRandomValue(0,fileCount-1) : 0;
                LoadMusicByIndex(selectedIndex);
                prevPlay=selectedIndex;

            // auto play song based on autoplay setting
            if (isPlay) {
                isStop=false;
                isPause =false;
                PlayMusicStream(music);  // autoplay at start
            }

            // init Audio Processor
            AttachAudioMixedProcessor(AudioProcessCallback);

            // set FPS (uso questo sistema per regolare la velocità di scorrimento)
            //SetTargetFPS(60);// https://bedroomcoders.co.uk/posts/218

            // scroll title / id3
            Rectangle displayArea = { 18, 24, 504,36 };
            float titleX = displayArea.x ;
            float speed = 60.0f;

            // filelist variables
            Rectangle filesArea = { 16,108,screenWidth-32,206 };
            int rowHeight = 23;
            int visibleRows = 9;
            const int centerRow = 4;
            int scrollOffset = 0;     // primo file visualizzato

            // visualizer  area / variables for effects
            Rectangle visArea = {16,110,screenWidth-32,200};

            //infoArea
            Rectangle infoArea = {18,330,screenWidth-36,110};
            Vector2 timeSize = MeasureTextEx(digitFnt, "00:00:00", (float)digitFnt.baseSize, 0);

            // flag area / music file info
            Vector2 xyFlags= {556,20};

            SetWindowSize(screenWidth,screenHeight);
            SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  

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
        Vector2 titleSize = MeasureTextEx(titleFnt, titleStr, 36, 0);
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

    
        // set initial volume 
        SetMasterVolume(volume);

        // update sound stream
        UpdateMusicStream(music);   // Update music buffer with new stream data

        // auto move on next song
        if (GetMusicTimePlayed(music) >= (GetMusicTimeLength(music) - 0.450f)) {
                prevPlay = selectedIndex;
                StopMusicStream(music);
                UnloadMusicStream(music);
                if (isShuffle) {
                    int shuffleIndex = GetRandomValue(0,fileCount-1);
                    if (shuffleIndex == fileCount) --shuffleIndex;
                    // if new song is equal to current , select next one
                    if (fileCount > 0 && shuffleIndex == selectedIndex) shuffleIndex = (shuffleIndex + 1) % fileCount;
                    selectedIndex = shuffleIndex;
                    } 
                else selectedIndex = (selectedIndex + 1) % fileCount;

                LoadMusicByIndex(selectedIndex);
                PlayMusicStream(music);
                selectedIndex = currPlay;
            }
        
        

        // Set audio volume
        if (IsKeyDown(KEY_PAGE_DOWN)) {
            volume -= (volume >= 0.0f) ? 0.01f : 0.0f;
            if (volume < 0.0f) volume = 0.0f, isMute=true;
        }

        if (IsKeyDown(KEY_PAGE_UP)) {
            isMute = false;
            volume += (volume <= 1.0f) ? 0.01f : 0.0f;
            if (volume > 1.0f) volume = 1.0f;
        }

        if (IsKeyPressed(KEY_M)) // MUTE
        {
            isMute = !isMute;
                if (isMute) {
                    prev_volume = volume;
                    volume = 0.0f;
                    }
                else volume = prev_volume;
        }

        if (IsKeyPressed(KEY_P) && isPlay &&!isStop) { // pause
            isPause = !isPause;
            if (isPause) PauseMusicStream(music);
            else ResumeMusicStream(music);
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

        if (IsKeyPressed(KEY_LEFT) && isPlay) // seek -10sec
        {
                    if (currentTime < 10.0f) {
                        currentTime = 0.0f; 
                        SeekMusicStream(music, 0.0f);
                       // UpdateMusicStream(music);
                    }
                    else SeekMusicStream(music, currentTime - SEEK_TIME);

        }
        if (IsKeyPressed(KEY_RIGHT) && isPlay) // seek +10sec
        {
                    if (currentTime + SEEK_TIME >= GetMusicTimeLength(music)) {
                        currentTime = 0.0f;
                        SeekMusicStream(music, 0.0f);
                        //UpdateMusicStream(music);
                    }
                    else  SeekMusicStream(music, currentTime + SEEK_TIME);
        }

        if (IsKeyPressed(KEY_N)) { // Next song based on SHUFFLE setting
            prevPlay = selectedIndex; //save for 1 shot prev.song
            if (isShuffle) {
                int shuffleIndex = GetRandomValue(0,fileCount-1);
                if (shuffleIndex == fileCount) --shuffleIndex;
                // if new song is equal to current , select next one
                if (fileCount > 0 && shuffleIndex == selectedIndex) shuffleIndex = (shuffleIndex + 1) % fileCount;
                selectedIndex = shuffleIndex;
                } 
            else selectedIndex = (selectedIndex + 1) % fileCount;

                StopMusicStream(music);
                UnloadMusicStream(music);
                LoadMusicByIndex(selectedIndex);
                PlayMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
            }

        if (IsKeyPressed(KEY_Z)) { // Previous song : no shuffle on previous song
                if (isShuffle) selectedIndex = prevPlay;
                else selectedIndex--;
                if (selectedIndex < 0) selectedIndex=0;
                StopMusicStream(music);
                UnloadMusicStream(music);
                LoadMusicByIndex(selectedIndex);
                PlayMusicStream(music);
                isStop=false;
                isPlay=true;
                isPause=false;
        }


        if (IsKeyPressed(KEY_S)) isShuffle = !isShuffle;
        if (IsKeyPressed(KEY_C)) accentColor = colors[GetRandomValue(0,MAX_COLORS_COUNT-1)];

if (isVisFiles) {  // when mini view is active fileselectio is disabled
        //------------------------------------------------------------------------------
        // scrollFiles with mouse
        //------------------------------------------------------------------------------
        //if (CheckCollisionPointRec(mousePos,filesArea)) {

            if (fileCount >= visibleRows) {
                        selectedIndex  = -(int)GetMouseWheelMove() + selectedIndex;  
            
                        if (IsKeyPressed(KEY_X)) selectedIndex = currPlay;
                        if (IsKeyPressed(KEY_HOME)) selectedIndex = 0;
                        if (IsKeyPressed(KEY_END)) selectedIndex = fileCount -1;
            
                        if (IsKeyPressed(KEY_DOWN)) selectedIndex++;
                        if (IsKeyPressed(KEY_UP)) selectedIndex--;
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                            prevPlay = currPlay; //save for 1 shot prev.song
                            if (selectedIndex >= 0 && selectedIndex < fileCount) {
                                StopMusicStream(music);
                                //UnloadMusicStream(music);
                                LoadMusicByIndex(selectedIndex);
                                PlayMusicStream(music);
                                //UpdateMusicStream(music);
                                isStop=false;
                                isPlay=true;
                                isPause=false;
                                }
                            }
                    // checks
                            if (selectedIndex < 0) selectedIndex=0;
                            if (selectedIndex > fileCount-1) selectedIndex=fileCount-1;
                }
}  // all above keybindigs are disable in mini view modo

        if (IsKeyPressed(KEY_F)) isVisFiles = !isVisFiles;

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

//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------
    BeginTextureMode(target);
        ClearBackground(BLANK);
    EndTextureMode();

    BeginDrawing();
        ClearBackground (BLANK);

        //drawRectangleRounded(0,0,screenWidth,screenHeight,bgColor);
        //load player background image
        DrawTexture(background, screenWidth/2 - background.width/2, screenHeight/2 - background.height/2, accentColor); // WHITE

        // song Title
        BeginScissorMode( (int)displayArea.x, (int)displayArea.y, (int)displayArea.width, (int)displayArea.height);
            if (needScroll) DrawTextEx(titleFnt, titleStr, (Vector2){ titleX, displayArea.y }, (float)titleFnt.baseSize, 0, primaryColor);
            else DrawTextEx(titleFnt, titleStr, (Vector2){ displayArea.x, displayArea.y}, (float)titleFnt.baseSize,0, primaryColor);
        EndScissorMode();
        // song Artist
        DrawLineDashed((Vector2){displayArea.x,displayArea.y+39},(Vector2){displayArea.width+20,displayArea.y+39},1,1,accentColor);
        DrawTextEx(artistFnt, artistStr, (Vector2){ displayArea.x, displayArea.y+42 }, (float)artistFnt.baseSize, 0, secondaryColor);

        // STOP flag
        DrawRectangle(xyFlags.x-8,xyFlags.y+3,4,12,isStop ? accentColor:borderColor);
        DrawTextEx(defaultFnt,"STOP",(Vector2){xyFlags.x,xyFlags.y},(float)defaultFnt.baseSize,1, isStop ? WHITE : borderColor);

        // PLAY flag
        DrawRectangle(xyFlags.x-8,xyFlags.y+23,4,12,isPlay ? accentColor:borderColor);
        DrawTextEx(defaultFnt,"PLAY",(Vector2){xyFlags.x,xyFlags.y+20},(float)defaultFnt.baseSize,1, isPlay ? WHITE : borderColor);

        // PAUSE flag
        DrawRectangle(xyFlags.x-8,xyFlags.y+43,4,12,isPause ? accentColor:borderColor);
        DrawTextEx(defaultFnt,"PAUSE",(Vector2){xyFlags.x,xyFlags.y+40},(float)defaultFnt.baseSize,1, isPause ? WHITE : borderColor);

        // Shuffle flag
        DrawRectangle(xyFlags.x-8,xyFlags.y+63,4,12,isShuffle ? accentColor:borderColor);
        DrawTextEx(defaultFnt,"SHUFFLE",(Vector2){xyFlags.x,xyFlags.y+60},(float)defaultFnt.baseSize,1, isShuffle ? WHITE : borderColor);


    if (!isVisFiles) { 
         // draw 40 band vumeter
         BeginScissorMode(visArea.x,visArea.y,visArea.width,visArea.height);

                    // draw vumeter
                    float barWidth = (float) visArea.width / NUM_BARS; // larghezza totale grafico
                    float barSpacing = 4.0f;  // space between bars (direttamente proporzionale a larghezza barre)
                    
                    const int maxSegments = 48; //nr. segmenti singola barra
                    const float segmentHeight = 2.0f; //altezza segmento... anche se e' linea 
                    const float segmentGap = 2.0f;   // distanza tra i segmenty 
                    float baseYPos = visArea.y + visArea.height; //base del vumeter

                    for (int i = 0; i < NUM_BARS; i++) {
                        float xPos = 1 + visArea.x + i * barWidth; // posizione X iniziale vumeter

                        int segmentsToLight = (int)(barValues[i] * maxSegments); 
                        int peakSegment = (int)(peakValues[i] * maxSegments) - 1;
                        if (peakSegment < 0 && peakValues[i] > 0.01f) peakSegment = 0;

                        for (int j = 0; j < maxSegments; j++) {
                            float segYPos = baseYPos - (j * (segmentHeight + segmentGap)) - segmentHeight;
                            // Logica di disegno combinata barra + picco
                            bool drawActiveSegment = (j < segmentsToLight);
                            bool drawPeakSegment = (j == peakSegment);
                            DrawLine(xPos, segYPos,xPos +(barWidth - barSpacing), segYPos, (drawActiveSegment || drawPeakSegment) ? lightenColor(accentColor,0.60f):darkenColor(accentColor,0.24f) );
                         }
                    }
        EndScissorMode();
    }
        else {
            // draw file selection
            //DrawRectangle(0,filesArea.y-4,screenWidth,filesArea.height+8,darkenColor(accentColor,0.088f));
            BeginScissorMode( (int)filesArea.x, (int)filesArea.y, (int)filesArea.width, (int)filesArea.height);
                if (fileCount < visibleRows) visibleRows = fileCount;

                scrollOffset = selectedIndex - centerRow;
                if (scrollOffset < 0) scrollOffset = 0;
                int maxOffset = fileCount - visibleRows;
                if (maxOffset < 0) maxOffset = 0;
                if (scrollOffset > maxOffset) scrollOffset = maxOffset;

                    for (int i = 0; i < visibleRows; i++) {
                        // faded text color
                        float fadeValue = (i <= 4) ? (i + 1) * 0.4f : (9 - i) * 0.4f;
                        Color fadeColor = darkenColor(secondaryColor,fadeValue);
                        DrawLine(filesArea.x, filesArea.y + (i*rowHeight), screenWidth-8, filesArea.y +(i*rowHeight),borderColor);
                        //if (i % 2) DrawRectangleRec((Rectangle){filesArea.x+1,filesArea.y +(i*rowHeight),filesArea.width-2,rowHeight-1}, darkenColor(textColor,0.42f));
                        int fileIndex = scrollOffset + i;
                        if (fileIndex > fileCount) break;
                        if (fileIndex == selectedIndex) DrawRectangle(filesArea.x,filesArea.y +(i*rowHeight),filesArea.width,rowHeight-1, accentColor);
                            DrawTextEx(defaultFnt,TextFormat("%04i",fileIndex + 1),(Vector2){filesArea.x + 6, filesArea.y +(i*rowHeight)+2},(float)defaultFnt.baseSize,0,(fileIndex == selectedIndex)? BLACK : primaryColor);
                            DrawTextEx(defaultFnt,TextFormat("%s",GetFileName(files[fileIndex])),(Vector2){filesArea.x + 52, filesArea.y +(i*rowHeight)+2},(float)defaultFnt.baseSize,0,(fileIndex == selectedIndex)? BLACK : fadeColor);
                        
                        }   
                //vertical divider
                DrawLine(filesArea.x+44,filesArea.y,filesArea.x+44,filesArea.y + filesArea.height,borderColor);
            EndScissorMode();
            }

        //statusbar with some info
        // DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, BLACK); 
        // DrawText(TextFormat("version %s", TOOL_VERSION), 64, screenHeight-16, 10, GRAY); 
        // DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);

        //volume info
        DrawTexture(volumeICO_texture,infoArea.x,infoArea.y +14,secondaryColor);
           if (!isMute) DrawTextEx(defaultFnt,TextFormat("%.f%%",volume*100),(Vector2){infoArea.x+32,infoArea.y+21}, (float)defaultFnt.baseSize,0,primaryColor);
           else DrawTextEx(defaultFnt,TextFormat("%.f%%",volume*100),(Vector2){infoArea.x+32,infoArea.y+21}, (float)defaultFnt.baseSize,0, borderColor);
         
        // tempo attuale brano / progressbar / durata totale brano
        DrawTextEx(digitFnt,curTimeStr,(Vector2){(screenWidth/2)-(timeSize.x/2),infoArea.y},(float)digitFnt.baseSize,0, primaryColor);
        //
        DrawRectangle((screenWidth/2)-(timeSize.x/2),infoArea.y+28,timeSize.x,3,borderColor);
        for (int i = 0; i < (timePlayed * 94); i++) DrawRectangleRec((Rectangle){i+((screenWidth/2)-(timeSize.x/2)),infoArea.y+28,1,3},accentColor);
        //
        DrawTextEx(digitFnt,totTimeStr,(Vector2){(screenWidth/2)-(timeSize.x/2),infoArea.y+34},(float)digitFnt.baseSize,0, secondaryColor);

        // current song position / total songs
        DrawTextEx(defaultFnt,TextFormat("%04d",currPlay + 1),(Vector2){infoArea.x+210,infoArea.y+20},(float)defaultFnt.baseSize,1, secondaryColor);
        DrawTextEx(defaultFnt,TextFormat("%04d",fileCount),(Vector2){infoArea.x+360,infoArea.y+20},(float)defaultFnt.baseSize,1, secondaryColor);

        // file type / KHz / stereo - mono  of current song
        DrawTextEx(defaultFnt,TextFormat("%s",TextToUpper(GetFileExtension(files[selectedIndex]))),(Vector2){xyFlags.x+24,327}, (float)defaultFnt.baseSize,1, secondaryColor);
        DrawTextEx(defaultFnt,TextFormat("%i kHz",music.stream.sampleRate/1000),(Vector2){xyFlags.x+24,343}, (float)defaultFnt.baseSize,0, secondaryColor);
        DrawTextEx(defaultFnt,TextFormat("%i bits",music.stream.sampleSize),(Vector2){xyFlags.x+24,359}, (float)defaultFnt.baseSize,0, secondaryColor);
        DrawTextEx(defaultFnt,TextFormat("%s", (music.stream.channels == 1)? "mono" : (music.stream.channels == 2)? "stereo" : "multi"),(Vector2){xyFlags.x+24,374}, (float)defaultFnt.baseSize,0, secondaryColor);

    EndDrawing();
}
    
    //unload resource

    DetachAudioMixedProcessor(AudioProcessCallback); // disconnect audio preocessor for vumeter
    UnloadMusicStream(music); // Unloaad music stream
    // unload fonts
    UnloadFont(defaultFnt);
    UnloadFont(titleFnt);
    UnloadFont(artistFnt);
    UnloadFont(digitFnt);
    UnloadFont(filesFnt);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}
