/*******************************************************************************
*
*   SIM PLAYER : a raylib simple music player
*   Small utility to play mp3 files based on raylib version 6.0
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************/
		
#define TOOL_NAME               "Simple Music Player"
#define TOOL_SHORT_NAME         "simplayer"
#define TOOL_COMMENT            "Simple but modern currMusic player written in C99 using Raylib - Play MP3 and OGG file"
#define TOOL_VERSION            "3.1.5"

#include <stdio.h>
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
#include <pwd.h>
#include <id3tag.h>

// gcc -Wall -Werror simplayer.c  -o simplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

// window initial size
#define screenWidth   640
#define screenHeight  396

// some custom colors
const Color accentColor = SKYBLUE;//CLITERAL(Color){136, 192, 208, 255}; 
const Color primaryColor = CLITERAL(Color){253, 255, 255, 255}; 
const Color secondaryColor = CLITERAL(Color){156, 158, 158, 255};
const Color borderColor = CLITERAL(Color){50, 70, 70, 255};
const Color bgColor = CLITERAL(Color){10, 20, 30, 255};

// faded songs (time in seconds)
#define FADE_TIME 5.0f // Durata della dissolvenza in secondi
typedef enum {
	STATE_PLAYING_SINGLE, // esecuzione normale
	STATE_CROSSFADING     // dissolvenza incrociata tra due brani
} PlayerState;

PlayerState state = STATE_PLAYING_SINGLE;
float fadeTimer;

// Struttura tagID3
typedef struct AudioMetadata {
    char title[256];
    char artist[256];
    char album[256];
    char year[32];
} AudioMetadata;

AudioMetadata songId3;

// define stream 
	Music currMusic = { 0 };
	Music nextMusic = { 0 };

// Key repeat structure
typedef struct {
    int currentKey;      // Il tasto attualmente premuto
    float timer;         // Timer per calcolare i delay
    bool isRepeating;    // true se ha superato il delay iniziale
    float firstDelay;    // Tempo di attesa iniziale 
    float repeatRate;    // Intervallo di ripetizione 
} KeyRepeater;

// variables
#define SEEK_TIME 10.0f // seek time

bool isPlay = true;
bool isStop = true;
bool isPause = false;  
bool isMute = false;
bool isVumeter = true; // true:default : show 36 band equalizer , if set false show file library
float currVolume = 0.50f;            // Default audio currVolume [0.0f..1.0f]
float prevVolume = 0.50f;
char *currMusicDir;
bool isVumColored = true; //false:default : start with monochrome vumeter / true: use colored vumeter

// files extension
#define FILTER_MP3      ".mp3"
#define FILTER_OGG      ".ogg"

// file selection
#define MAX_FILES 32768
#define MAX_NAME 1024
char files[MAX_FILES][MAX_NAME];
bool isShuffle = true;
int totalFiles = 0;
int selectedIndex = 0; // selected song in the file list
int currPlay = 0; //playing song

// barra path su Linux oppure su WIN
#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

// colori vumeter se colorato..
#define color0  CLITERAL(Color){ 141,  90, 163, 255 } // Viola orchidea pastello (Inizio)
#define color1  CLITERAL(Color){ 131,  92, 166, 255 } // Viola ametista
#define color2  CLITERAL(Color){ 120,  95, 168, 255 } // Viola profondo morbido
#define color3  CLITERAL(Color){ 109,  97, 171, 255 } // Viola bluastro
#define color4  CLITERAL(Color){  97,  99, 173, 255 } // Indaco intenso pastello
#define color5  CLITERAL(Color){  94, 107, 176, 255 } // Blu oltremare spento
#define color6  CLITERAL(Color){  91, 114, 179, 255 } // Pervinca carico
#define color7  CLITERAL(Color){  89, 122, 181, 255 } // Blu fiordaliso pastello
#define color8  CLITERAL(Color){  86, 129, 184, 255 } // Blu denim vivido
#define color9  CLITERAL(Color){  83, 137, 186, 255 } // Avio acceso
#define color10 CLITERAL(Color){  81, 144, 189, 255 } // Blu cielo pastello
#define color11 CLITERAL(Color){  78, 152, 191, 255 } // Carta da zucchero vivido
#define color12 CLITERAL(Color){  76, 159, 194, 255 } // Azzurro glaciale
#define color13 CLITERAL(Color){  73, 167, 196, 255 } // Turchese morbido
#define color14 CLITERAL(Color){  71, 174, 199, 255 } // Turchese chiaro
#define color15 CLITERAL(Color){  68, 182, 201, 255 } // Ciano opaco
#define color16 CLITERAL(Color){  66, 189, 204, 255 } // Acquamarina pastello
#define color17 CLITERAL(Color){  63, 197, 207, 255 } // Azzurro ciano vivido (Fine)

// Colors to choose from
#define MAX_COLORS_COUNT  18  // Number of colors available (BLACK & WHITE are excluded)
const Color colors[MAX_COLORS_COUNT] = { color0,color1,color2,color3,color4, color5, color6,color7,color8,color9, color10,color11,color12,color13, color14, color15,color16,color17 };

// variabile per la ricerca numerica
int inputBuffer = 0;       // Accumula i numeri digitati
bool hasTyped = false;     // Flag per lo stato del timer
float lastTypeTime = 0.0f; // Timestamp dell'ultima cifra inserita
const float timeoutDuration = 2.0f;


// vumeter
#define MAX_SAMPLES          1024
#define NUM_BARS             36 // numero di barre verticali
#define SMOOTHING_FACTOR     0.18f  
#define PI                   3.14159265358979323846f
// Parametri di comportamento del picco Hi-Fi
#define PEAK_HOLD_FRAMES     30     // Quanti frame il picco resta fermo in alto (0.3 secondi a 60 FPS)
#define PEAK_DECAY_SPEED     0.015f // Velocità di discesa del picco dopo l'attesa

typedef struct { float real; float imag; } Complex;

float rawSamples[MAX_SAMPLES] = { 0 };
float barValues[NUM_BARS] = { 0 };
float peakValues[NUM_BARS] = { 0 };
int peakHoldTimers[NUM_BARS] = { 0 };


// -----------------------------------------------------------------------------
//						Functions
// -----------------------------------------------------------------------------

void drawRectangleRounded (int X, int Y, int W, int H, float radius, Color color)  {
// raylib vuole il "roundness" espresso come frazione rispetto al lato più corto (300)
  Rectangle  rect = { X, Y, W, H};   // toplx, toply, width, height
  int segments = 36;
  DrawRectangleRounded ( rect, radius, segments, color );
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

// estrai ID3 tag descriptor usando libid3tag
static void ExtractTagString(struct id3_frame *frame, char *dest, size_t maxLen) {
    if (frame && frame->nfields > 1) {
        id3_ucs4_t const *ucs4 = id3_field_getstrings(&frame->fields[1], 0);
        if (ucs4) {
            id3_utf8_t *utf8 = id3_ucs4_utf8duplicate(ucs4);
            if (utf8) {
                strncpy(dest, (char *)utf8, maxLen - 1);
                dest[maxLen - 1] = '\0';
                free(utf8);
                return; 
            }
        }
    }
    dest[0] = '\0';
}

// Legge i tagID3 o applica il fallback dal nome del file
AudioMetadata GetMp3Metadata(const char *filePath) {
    AudioMetadata meta = { 0 };

    // Lettura dei tag con libid3tag
    struct id3_file *file = id3_file_open(filePath, ID3_FILE_MODE_READONLY);
    if (file) {
        struct id3_tag *tag = id3_file_tag(file);
        if (tag) {
            // Cerchiamo i 4 frame principali
            struct id3_frame *title_frame  = id3_tag_findframe(tag, ID3_FRAME_TITLE, 0);
            struct id3_frame *artist_frame = id3_tag_findframe(tag, ID3_FRAME_ARTIST, 0);
            struct id3_frame *album_frame  = id3_tag_findframe(tag, ID3_FRAME_ALBUM, 0);
            struct id3_frame *year_frame   = id3_tag_findframe(tag, ID3_FRAME_YEAR, 0);

            ExtractTagString(title_frame,  meta.title,  sizeof(meta.title));
            ExtractTagString(artist_frame, meta.artist, sizeof(meta.artist));
            ExtractTagString(album_frame,  meta.album,  sizeof(meta.album));
            ExtractTagString(year_frame,   meta.year,   sizeof(meta.year));
        }
        id3_file_close(file);
    }

    // rileva nome file
    const char *fileName = GetFileName(filePath); 

    // Fallback Titolo
    if (strlen(meta.title) == 0) {
        char cleanName[256];
        strncpy(cleanName, fileName, sizeof(cleanName) - 1);
        cleanName[sizeof(cleanName) - 1] = '\0';
        char *dot = strrchr(cleanName, '.');
        if (dot) *dot = '\0';
        strncpy(meta.title, cleanName, sizeof(meta.title) - 1);
    }

    // Fallback Artista
    if (strlen(meta.artist) == 0) strncpy(meta.artist, "Artist not defined", sizeof(meta.artist) - 1);
    // Fallback Album
    if (strlen(meta.album) == 0) strncpy(meta.album, "Album not defined", sizeof(meta.album) - 1);
    // Fallback Anno/Data
    if (strlen(meta.year) == 0) strncpy(meta.year, "...", sizeof(meta.year) - 1);

    return meta;
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

void LoadMusicByIndex(int index) {
	//currPlay = index;
	currMusic = LoadMusicStream(files[index]);
	songId3 = GetMp3Metadata(files[index]);
}

void LoadNextMusicByIndex(int index) {
	//currPlay = index;
	nextMusic = LoadMusicStream(files[index]);
	songId3 = GetMp3Metadata(files[index]);
}


void getMusicFolder(void) {
	// currMusic folder must be a folder called "Music" inside your home dir (could be also aa symbolic link to real Music dir)
	if ((currMusicDir = getenv("HOME")) == NULL) currMusicDir = getpwuid(getuid())->pw_dir;
	strcat(currMusicDir,"/Music\0");
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

int loadFilesRecursive(const char *path, char files[MAX_FILES][MAX_NAME], int count)
{
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

            if (ext && (strcmp(ext, FILTER_MP3) == 0 || strcmp(ext, FILTER_OGG) == 0)) {
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

void shuffleFiles(char files[MAX_FILES][MAX_NAME], int count) 
{
    // Se c'è solo un file o nessuno, non serve mescolare
    if (count <= 1) return;

    // Variabile temporanea per lo scambio delle stringhe
    char temp[MAX_NAME];

    // Algoritmo di Fisher-Yates
    for (int i = count - 1; i > 0; i--) {
        // Sceglie un indice casuale tra 0 e i (inclusi)
        int j = GetRandomValue(0, i);

        // Se l'indice è identico, salta lo scambio (evita doppioni)
        if (i == j) continue;

        // Scambia files[i] con files[j]
        strcpy(temp, files[i]);
        strcpy(files[i], files[j]);
        strcpy(files[j], temp);
    }
}

//------------------------------------------------------------------------------------
// Spectrum analyzer audio processing function
//------------------------------------------------------------------------------------

static inline unsigned int ReverseBits(unsigned int x, int bits) {
    unsigned int y = 0;
    for (int i = 0; i < bits; i++) {
        y = (y << 1) | (x & 1);
        x >>= 1;
    }
    return y;
}

void FFT_Iterative(Complex *X, int n) {
    int bits = (int)log2f((float)n);
    for (int i = 0; i < n; i++) {
        unsigned int rev = ReverseBits(i, bits);
        if (i < (int)rev) {
            Complex temp = X[i];
            X[i] = X[rev];
            X[rev] = temp;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * PI / len;
        Complex wlen = { .real = cosf(angle), .imag = sinf(angle) };
        wlen.imag = sinf(angle); // Correzione typo inline
        for (int i = 0; i < n; i += len) {
            Complex w = { .real = 1.0f, .imag = 0.0f };
            for (int j = 0; j < len / 2; j++) {
                Complex u = X[i + j];
                Complex v = {
                    .real = X[i + j + len / 2].real * w.real - X[i + j + len / 2].imag * w.imag,
                    .imag = X[i + j + len / 2].real * w.imag + X[i + j + len / 2].imag * w.real
                };
                X[i + j] = (Complex){ .real = u.real + v.real, .imag = u.imag + v.imag };
                X[i + j + len / 2] = (Complex){ .real = u.real - v.real, .imag = u.imag - v.imag };
                float nextReal = w.real * wlen.real - w.imag * wlen.imag;
                w.imag = w.real * wlen.imag + w.imag * wlen.real;
                w.real = nextReal;
            }
        }
    }
}

void AudioProcessCallback(void *buffer, unsigned int frames) {
    float *samples = (float *)buffer;
    for (unsigned int i = 0; i < frames && i < MAX_SAMPLES; i++) {
        rawSamples[i] = samples[i * 2] * 0.1f; 
    }
}
int main (int argc, char *argv[]) 
{
	// Set configuration flags for window creation
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TRANSPARENT ); // | FLAG_MSAA_4X_HINT | FLAG_WINDOW_TOPMOST); 
	InitWindow(screenWidth, screenHeight, "simplayer");
	SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    SetTargetFPS(120);

	// Set UI style
	// Custom GUI font loading
	Font titleFnt = LoadFontEx("fonts/ManropeV5-SemiBold.otf", 36, NULL, 0);
	Font artistFnt = LoadFontEx("fonts/ManropeV5-Regular.otf", 28, NULL, 0);
	Font defaultFnt = LoadFontEx("fonts/DINPro-Medium.otf", 18, NULL, 0);
	Font digitFnt= LoadFontEx("fonts/SF-Mono-Semibold.ttf", 24, NULL, 0); 
	Font filesFnt= LoadFontEx("fonts/PixelOperator.ttf", 16, NULL, 0); 

    // Create framebuffer
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
	
	//load currVolume icon
	Texture2D volumeIcon = LoadTexture("assets/volumeIcon.png");          // Image converted to texture, GPU memory (VRAM)
	//background image
	Texture2D background = LoadTexture("assets/background.png");          // Image converted to texture, GPU memory (VRAM)

	// init Audio
	InitAudioDevice();
	SetAudioStreamBufferSizeDefault(65535);

	// load music files into array
	getMusicFolder();
	int totalFiles = loadFilesRecursive(currMusicDir, files, 0);

    // 2. Ordina o mescola files nella lista
    if (!isShuffle && totalFiles > 0) {
        qsort(files, totalFiles, MAX_NAME, compareFiles); 
    }
    else shuffleFiles(files, totalFiles);

	// load first song to play  based on shuffle setting
		selectedIndex = 0;
		LoadMusicByIndex(selectedIndex);

	// auto play song at start
	if (isPlay) {
		isStop=false;
		isPause =false;
		PlayMusicStream(currMusic);  // autoplay at start
	}

	// init Audio Processor
	AttachAudioMixedProcessor(AudioProcessCallback);

	// scroll title / id3
	Rectangle displayArea = { 18, 18, 548,90 };
	float titleX = displayArea.x ;
	float speed = 60.0f;

	// filelist variables
	Rectangle filesArea = { 14,94,screenWidth-28,204 };
	int rowHeight = 23;
	int visibleRows = 9;
	const int centerRow = 4;
	int scrollOffset = 0;     // primo file visualizzato

	// visualizer  area / variables for effects 
	Rectangle visArea = { 14, 92, screenWidth-28, 208};

	//infoArea e calcolo lunghezza timeProgressbar
	Rectangle infoArea = {18,312,screenWidth-30,110};
	Vector2 timeText = MeasureTextEx(digitFnt, "00:00:00", (float)digitFnt.baseSize, 0);

	// flag area / currMusic file info
	Vector2 xyFlags= {556,8};

 	// vumeter
	Complex fftBuffer[MAX_SAMPLES];
	float minDb = -50.0f; 
	float maxDb = 4.0f; // sensibilita Decibel
	float maxSeenMagnitude = 0.01f; // Auto-gain tracker


    // set key repeater speed (400ms initial delay, 80ms repeat rate)
    KeyRepeater repeater = {
        .currentKey = 0,
        .timer = 0.0f,
        .isRepeating = false,
        .firstDelay = 0.40f,
        .repeatRate = 0.064f
    };

    // Variabili per gestire il tempo della transizione tra tag artista e tag albu,
    bool isTransitioning = true;
    float transitionProgress = 0.0f; // Va da 0.0 (inizio) a 1.0 (fine)
    const float transitionDuration = 10.00f; // Durata della transizione: 1.0f = 1 sec

	// fai riapparire finestra dopo averla nascosta durante il carimento iniziale
	SetWindowSize(screenWidth,screenHeight);
	SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  
	//SetWindowPosition(8,40);  
	ClearWindowState(FLAG_WINDOW_HIDDEN);

while (!WindowShouldClose())
	{
	//----------------------------------------------------------------------------------
	// Update
	//----------------------------------------------------------------------------------

		// Calcola la percentuale di avanzamento della traccia
		float timePlayed = GetMusicTimePlayed(currMusic);
		float timeLength = GetMusicTimeLength(currMusic);
		float timeRemaining = timeLength - timePlayed;
		float timeProgress = (timeLength > 0.0f) ? (timePlayed / timeLength) : 0.0f;
		// get frame time
		float deltaTime = GetFrameTime();

		// set scroll text speed
		Vector2 titleSize = MeasureTextEx(titleFnt, songId3.title, (float)titleFnt.baseSize, 0);
		float titleWidth = titleSize.x;
		bool needScroll = titleWidth > displayArea.width;

		if (needScroll) {
			titleX -= (speed * deltaTime);
			if (titleX <= displayArea.x - titleWidth) titleX += titleWidth + displayArea.width;
		}

		// calculating song times in HH:MM:SS
		  char curTimeStr[32]= { '\0' };
		  char totTimeStr[32]= { '\0' };
			int hour   = (int)timePlayed / 3600;
			int minute = (int)timePlayed / 60 % 60;
			int second = (int)timePlayed % 60;
			snprintf(curTimeStr,sizeof(curTimeStr),"%02d:%02d:%02d", hour , minute, second);

			int hours   = (int)timeLength / 3600;
			int minutes = (int)timeLength / 60 % 60;
			int seconds = (int)timeLength % 60;
			snprintf(totTimeStr,sizeof(totTimeStr),"%02d:%02d:%02d", hours, minutes, seconds);

	// ---------------------------------------------------------------------------
	// faded songs
	// ---------------------------------------------------------------------------

	// set initial currVolume 
	SetMusicVolume(currMusic, currVolume);
	SetMusicVolume(nextMusic, currVolume);
	
		// update sound stream
		if (IsMusicStreamPlaying(currMusic)) UpdateMusicStream(currMusic);
		if (state == STATE_CROSSFADING && IsMusicStreamPlaying(nextMusic)) UpdateMusicStream(nextMusic);

		switch (state)
		{
			case STATE_PLAYING_SINGLE:
				// Se mancano meno di FADE_TIME secondi alla fine dell brano corrente  parte il crossfade
				if (timeRemaining <= FADE_TIME && timeLength > FADE_TIME) 
				{
                    currPlay = (currPlay + 1) % totalFiles;
                    selectedIndex = currPlay;
					LoadNextMusicByIndex(currPlay);
					SetMusicVolume(nextMusic, 0.0f); // Parte silenzioso
					PlayMusicStream(nextMusic);
					fadeTimer = 0.0f;
					state = STATE_CROSSFADING;
				}

				//  caso in cui il brano finisce improvvisamente (o è troppo corto per il fade)
				else if ((!IsMusicStreamPlaying(currMusic) || timeRemaining <= 0.1f) && !isPause && !isStop)
				{
					UnloadMusicStream(currMusic);
                    currPlay = (currPlay + 1) % totalFiles;
					LoadMusicByIndex(currPlay);
					SetMusicVolume(currMusic, currVolume);
					PlayMusicStream(currMusic);
				}
				break;

			case STATE_CROSSFADING:
				fadeTimer += GetFrameTime(); // Avanzamento del tempo reale del frame
				
				// Calcoliamo la percentuale di timeProgresso del fade (da 0.0 a 1.0)
				float timeProgress = fadeTimer / FADE_TIME;
				if (timeProgress > 1.0f) timeProgress = 1.0f;

				// cambio volume tra "vecchia" e "nuova" canzone piu' dolce...
				float currentVol = currVolume * cosf(timeProgress * (PI / 2.0f));
				float nextVol = currVolume * sinf(timeProgress * (PI / 2.0f));

				SetMusicVolume(currMusic, currentVol);
				SetMusicVolume(nextMusic, nextVol);

				// Quando il fade è completato
				if (timeProgress >= 1.0f) 
				{
					SetMusicVolume(currMusic, 0.0f); // Azzera il volume della canzone attuale
					StopMusicStream(currMusic);    // 1. IMPORTANTE: Ferma l'audio della vecchia canzone
					UnloadMusicStream(currMusic); // Elimina la vecchia canzone
					currMusic = nextMusic;        // e la nuova canzone diventa quella corrente
					SetMusicVolume(currMusic, currVolume); // Assicurati che la nuova sia a pieno volume
					state = STATE_PLAYING_SINGLE;    // Ritorna allo stato normale
					selectedIndex = currPlay;
					// aggiorna info brano
					songId3 = GetMp3Metadata(files[selectedIndex]);
				}
				break;
		}
// -----------------------------------------------------------------------------

		// Set audio currVolume
		if (UpdateKeyRepeater(&repeater, KEY_KP_SUBTRACT, deltaTime) || IsKeyPressed(KEY_F1)) {
			currVolume -= (currVolume >= 0.0f) ? 0.05f : 0.0f;
			if (currVolume < 0.0f) currVolume = 0.0f, isMute=true;
		}

		if (UpdateKeyRepeater(&repeater, KEY_KP_ADD, deltaTime) || IsKeyPressed(KEY_F2)) {
			isMute = false;
			currVolume += (currVolume <= 1.0f) ? 0.05f : 0.0f;
			if (currVolume > 1.0f) currVolume = 1.0f;
		}

		if (IsKeyPressed(KEY_M)) // MUTE
		{
			isMute = !isMute;
				if (isMute) {
					prevVolume = currVolume;
					currVolume = 0.0f;
					}
				else currVolume = prevVolume;
		}

    // attiva i seguenti KEYPRESS/opzioni solo se si e' in normale riproduzione del brano
    // (durante il tempo di FADE tra le due canzoni non permette di fare nulla!)
	if (state == STATE_PLAYING_SINGLE)
	{
				if (IsKeyPressed(KEY_P) && isPlay && !isStop) { // pause
					isPause = !isPause;
					if (isPause) PauseMusicStream(currMusic);
					else ResumeMusicStream(currMusic);
				}

				// stop/play currMusic 
				if (IsKeyPressed(KEY_SPACE)) {
						if (isPlay) {
							isStop=true;
							isPlay=false;
							isPause=false;
							StopMusicStream(currMusic);
							}
						else {
							 isStop=false;
							 isPlay=true;
							 isPause=false;
							 state = STATE_PLAYING_SINGLE;
							 PlayMusicStream(currMusic);
							}
					}

				if (UpdateKeyRepeater(&repeater, KEY_LEFT, deltaTime) && isPlay) // seek -10sec
				{
							if (timePlayed < 10.0f) SeekMusicStream(currMusic, 0.0f);
							else SeekMusicStream(currMusic, timePlayed - SEEK_TIME);
				}

				if (UpdateKeyRepeater(&repeater, KEY_RIGHT, deltaTime) && isPlay) // seek +10sec
				{
							if (timePlayed + SEEK_TIME > timeLength) SeekMusicStream(currMusic, timePlayed);
							else  SeekMusicStream(currMusic, timePlayed + SEEK_TIME);
				}


				if (IsKeyPressed(KEY_N)) { // Next song
                        currPlay = (currPlay + 1) % totalFiles;
                        selectedIndex = currPlay;
						StopMusicStream(currMusic);
						UnloadMusicStream(currMusic);
						LoadMusicByIndex(currPlay);
						PlayMusicStream(currMusic);
						isStop=false;
						isPlay=true;
						isPause=false;
					}

				if (IsKeyPressed(KEY_Z)) { //previous song
                        currPlay-=1;

                        if (currPlay < 0) currPlay=0;
                        else {
                        selectedIndex = currPlay;
						StopMusicStream(currMusic);
						UnloadMusicStream(currMusic);
						LoadMusicByIndex(currPlay);
						PlayMusicStream(currMusic);
						isStop=false;
						isPlay=true;
						isPause=false;
                    }
				}

				if (IsKeyPressed(KEY_S)) {
                    isShuffle = !isShuffle;
                    if (isShuffle) shuffleFiles(files, totalFiles);
                    else qsort(files, totalFiles, MAX_NAME, compareFiles);  // ordinamento file

                    // ricarica subito la canzone presente nella stessa posizione prima dello shuffle on/off
                        StopMusicStream(currMusic);
                        LoadMusicByIndex(currPlay);
                        PlayMusicStream(currMusic);
				    }
                    
                if (IsKeyPressed(KEY_R)) isVumColored = !isVumColored;

				// Quick copy of filename without path and extension, with keyboard shortcuts
		        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
		        {
		            if (IsKeyPressed(KEY_C)) SetClipboardText(GetFileNameWithoutExt(files[selectedIndex]));
		        }

	   }

		//------------------------------------------------------------------------------
		// scrollFiles with mouse and keybinding in music library
		//----------------------------------------------------------------------
		if (!isVumeter) {
			if (totalFiles >= visibleRows) 
			{
				selectedIndex  = -(int)GetMouseWheelMove() + selectedIndex;  

				if (IsKeyPressed(KEY_HOME)) selectedIndex = 0;
				if (IsKeyPressed(KEY_END)) selectedIndex = totalFiles -1;

				// tasti con il repeat rate attivo -----------------------------
		        if (UpdateKeyRepeater(&repeater, KEY_DOWN, deltaTime)) selectedIndex++;
		        else if (UpdateKeyRepeater(&repeater, KEY_UP, deltaTime)) selectedIndex--;
		        else if (UpdateKeyRepeater(&repeater, KEY_PAGE_DOWN, deltaTime)) selectedIndex += 9;
		        else if (UpdateKeyRepeater(&repeater, KEY_PAGE_UP, deltaTime)) selectedIndex -= 9;
		        // -------------------------------------------------------------
				if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
					if (selectedIndex >= 0 && selectedIndex < totalFiles) {
						currPlay = selectedIndex;
                        StopMusicStream(currMusic);
						UnloadMusicStream(currMusic);
						LoadMusicByIndex(currPlay);
						PlayMusicStream(currMusic);
						//UpdateMusicStream(currMusic);
						isStop=false;
						isPlay=true;
						isPause=false;
						}
					}
				// checks
				if (selectedIndex < 0) selectedIndex=0;
				if (selectedIndex > totalFiles-1) selectedIndex=totalFiles-1;
			}

		// --------------------------------------------------------------
		// numeric search 
		// --------------------------------------------------------------
				float currentTime = (float)GetTime();

				// Timeout di 2 secondi
				if (hasTyped && (currentTime - lastTypeTime > timeoutDuration)) {
				    inputBuffer = 0;
				    hasTyped = false;
				}

                // caratteri numerici ('0'-'9')
                int charPressed = GetCharPressed();
                while (charPressed > 0) {
                    if (charPressed >= '0' && charPressed <= '9') {
                        int digit = charPressed - '0';
                        
                        // Calcoliamo in anticipo quale sarebbe il potenziale nuovo valore
                        int potentialNextValue = (inputBuffer * 10) + digit;

                        // controllo per non superare il limite di MAX_FILES
                        if (potentialNextValue <= MAX_FILES) {
                            inputBuffer = potentialNextValue;
                            lastTypeTime = currentTime; 
                            hasTyped = true;

                            // Controllo dei limiti min-max
                            if (inputBuffer >= totalFiles) selectedIndex = totalFiles - 1;
                            else selectedIndex = inputBuffer - 1 ;
                        }
                    }
                    charPressed = GetCharPressed();
                }

				// BS & DEL
				if (IsKeyPressed(KEY_BACKSPACE) && hasTyped) {
				    inputBuffer /= 10;
				    selectedIndex = inputBuffer;
				    lastTypeTime = currentTime;
				    if (inputBuffer == 0) hasTyped = false;
				}

				if (IsKeyPressed(KEY_DELETE)) {
				    inputBuffer = 0;
				    selectedIndex = currPlay;
				    hasTyped = false;
				}
			}

		if (IsKeyPressed(KEY_ESCAPE)) isVumeter = !isVumeter;

		//-----------------------------------------------------------------------------------------
		// vumeter update
		//-----------------------------------------------------------------------------------------
        for (int i = 0; i < MAX_SAMPLES; i++) {
            float window = 0.5f * (1.0f - cosf(2.0f * PI * i / (MAX_SAMPLES - 1)));
            fftBuffer[i] = (Complex){ .real = rawSamples[i] * window, .imag = 0.0f };
        }

        FFT_Iterative(fftBuffer, MAX_SAMPLES);

        for (int i = 0; i < NUM_BARS; i++) {
            // Distribuzione logaritmica migliorata per spaziare meglio bassi e alti
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

            // --- CURVA DI EQUALIZZAZIONE (Modifica Principale) ---
            // Creiamo un moltiplicatore basato sull'indice della barra per dare un "boost" progressivo agli alti.
            // Le frequenze alte tendono a disperdersi matematicamente nella FFT; questo compensa la perdita.
            float eqBoost = 1.0f + (float)i * 0.25f; 
            averageMagnitude *= eqBoost;

            if (averageMagnitude > maxSeenMagnitude) maxSeenMagnitude = averageMagnitude;
            maxSeenMagnitude *= 0.9995f; 

            float normalizedMag = averageMagnitude / maxSeenMagnitude;
            float db = 20.0f * log10f(normalizedMag + 0.00001f);
            
            float targetValue = (db - minDb) / (maxDb - minDb);
            if (targetValue < 0.0f) targetValue = 0.0f;
            if (targetValue > 1.0f) targetValue = 1.0f;

            // Smoothing asimmetrico: reattività istantanea in salita, morbidezza in discesa
            float currentSmoothing = (targetValue > barValues[i]) ? 0.35f : SMOOTHING_FACTOR;
            barValues[i] += (targetValue - barValues[i]) * currentSmoothing;

            if (barValues[i] >= peakValues[i]) {
                peakValues[i] = barValues[i];
                peakHoldTimers[i] = PEAK_HOLD_FRAMES; 
            } else {
                if (peakHoldTimers[i] > 0) {
                    peakHoldTimers[i]--; 
                } else {
                    peakValues[i] -= PEAK_DECAY_SPEED; 
                    if (peakValues[i] < barValues[i]) peakValues[i] = barValues[i];
                }
            }
        }
        //--------------------------------------------------------------------------------------------

        // transizione cambia info tra artista e album (year)
            transitionProgress += (deltaTime / transitionDuration);
            if (transitionProgress >= 1.0f) {
                transitionProgress = 0.0f;
                isTransitioning = !isTransitioning;
            }

        // impostazioni visive del vumeter --------------------------------------------------
        int baseYPos = visArea.y + 136; //base del vumeter

        // Larghezza e spaziatura delle barre forzate a valori interi stabili
        int barWidth = (int) visArea.width / NUM_BARS; // larghezza totale grafico
        int barSpacing = 3;  // space between bars (direttamente proporzionale a larghezza barre)

        const int maxSegments = 34; //nr. segmenti singola barra

        // Geometria VU Meter reale (valori interi assoluti)
        const int segmentHeight = 2; 
        const int segmentGap = 2;    

        // Geometria riflesso compressa (forzata a interi assoluti fissi)
        // 4 pixel di altezza e esattamente 2 pixel di spazio vuoto costante tra i segmenti
        const int mirrorSegHeight = 2;
        const int mirrorSegGap = 0;
        //-------------------------------------------------------------------------------
//------------------------------------------------------------------------------
		//
		// do something when  window loses focus
		//
		if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) SetWindowOpacity(0.92f);
		else SetWindowOpacity(1.0f);

//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------
	BeginTextureMode(target);
		ClearBackground(BLANK);
	EndTextureMode();

	BeginDrawing();
		ClearBackground (BLANK);

		//drawRectangleRounded(0,0,screenWidth,screenHeight,0.072f,bgColor);
		//load player background image
		DrawTexture(background, 0, 0, accentColor); // WHITE

		// song Title
		BeginScissorMode( (int)displayArea.x, (int)displayArea.y, (int)displayArea.width, (int)displayArea.height);
			// song title
			if (needScroll) DrawTextEx(titleFnt, songId3.title, (Vector2){ titleX, displayArea.y }, (float)titleFnt.baseSize, 0, primaryColor);
			else DrawTextEx(titleFnt, songId3.title, (Vector2){ displayArea.x, displayArea.y}, (float)titleFnt.baseSize,0, primaryColor);
			
			DrawLine(displayArea.x,displayArea.y+39,displayArea.width+20,displayArea.y+39,borderColor);
			
			// song Artist or album|year info (KEY PRESS_I)
			if (isTransitioning) DrawTextEx(artistFnt, TextFormat("%.50s",songId3.artist), (Vector2){ displayArea.x, displayArea.y+42 }, (float)artistFnt.baseSize, 0, secondaryColor);
			else DrawTextEx(artistFnt, TextFormat("%.50s (%s)", songId3.album, songId3.year), (Vector2){ displayArea.x, displayArea.y+42 }, (float)artistFnt.baseSize, 0, secondaryColor);
		EndScissorMode();

		// file type / KHz / stereo - mono  of current song
		DrawTextEx(defaultFnt,TextFormat("%s",TextToUpper(GetFileExtension(files[selectedIndex]))),(Vector2){xyFlags.x+24,xyFlags.y+3}, (float)defaultFnt.baseSize,1, primaryColor);
		DrawTextEx(defaultFnt,TextFormat("%i kHz",currMusic.stream.sampleRate/1000),(Vector2){xyFlags.x+24,xyFlags.y+23}, (float)defaultFnt.baseSize,0, secondaryColor);
		DrawTextEx(defaultFnt,TextFormat("%i bit",currMusic.stream.sampleSize),(Vector2){xyFlags.x+24,xyFlags.y+43}, (float)defaultFnt.baseSize,0, secondaryColor);
		DrawTextEx(defaultFnt,TextFormat("%s", (currMusic.stream.channels == 1)? "mono" : (currMusic.stream.channels == 2)? "stereo" : "multi"),(Vector2){xyFlags.x+24,xyFlags.y+63}, (float)defaultFnt.baseSize,0, secondaryColor);

	if (isVumeter) { 
		 // draw 40 band vumeter
		 BeginScissorMode(visArea.x,visArea.y,visArea.width,visArea.height);
                    //---------------------------------------------------------------------------------
					// draw vumeter
                    // -------------------------------------------------------------------------------
                    //DrawRectangleRec(visArea,DARKGRAY); // DEBUG: visualizer area
                    int count = 0;
                    Color barColor;
                    Color dimColor;

                     for (int i = 0; i < NUM_BARS; i++) {
                        int xPos = visArea.x + (i * barWidth); // posizione X iniziale vumeter
                        int segmentsToLight = (int)(barValues[i] * maxSegments); 
                        int peakSegment = (int)(peakValues[i] * maxSegments) - 1;
                        if (peakSegment < 0 && peakValues[i] > 0.01f) peakSegment = 0;

                        // bar colors (da 1 a 18 poi torna indietro)
                        if (isVumColored) {
                            barColor = colors[count];
                            if (i < 17) count++;
                            else count = 17 - (i-17);
                        } 
                        // colore barre fisso
                        else barColor = darkenColor(accentColor,0.72);
                        
                        // dimmed bar color
                        dimColor = (Color){ barColor.r / 2.4f, barColor.g / 2.4f, barColor.b / 2.4f, 255 };


                        for (int x = 0; x < maxSegments; x++) {

                            //vumeter
                            int segYPos = baseYPos - (x * (segmentHeight + segmentGap)) - segmentHeight;
                            
                            // peak color
                            if (x >= segmentsToLight && !isVumColored) barColor = accentColor; 

                            // disegno barra + picco
                            bool drawActiveSegment = (x < segmentsToLight);
                            bool drawPeakSegment = (x == peakSegment);

                            DrawLine(xPos, segYPos, xPos + (barWidth - barSpacing), segYPos, (drawActiveSegment || drawPeakSegment) ? barColor:dimColor);

                            // riflesso speculare  con spazio alla base
                            int segYPosMirror = 4 + baseYPos + (x * (mirrorSegHeight + mirrorSegGap)) + mirrorSegGap;

                            if (drawActiveSegment) DrawLine(xPos, segYPosMirror, xPos + (barWidth - barSpacing), segYPosMirror, darkenColor(barColor, 0.44f));
                            else if (drawPeakSegment) DrawLine(xPos, segYPosMirror, xPos + (barWidth - barSpacing), segYPosMirror, darkenColor(barColor, 0.52f));
                            else DrawLine(xPos, segYPosMirror, xPos + (barWidth - barSpacing), segYPosMirror, darkenColor(dimColor, 0.56f));
                         }
                    }
		EndScissorMode();

        }
		else {
			// draw file selection
			//DrawRectangleRec(filesArea,accentColor); // DEBUG
			BeginScissorMode( (int)filesArea.x, (int)filesArea.y, (int)filesArea.width, (int)filesArea.height);
				if (totalFiles < visibleRows) visibleRows = totalFiles;

				scrollOffset = selectedIndex - centerRow;
				if (scrollOffset < 0) scrollOffset = 0;
				int maxOffset = totalFiles - visibleRows;
				if (maxOffset < 0) maxOffset = 0;
				if (scrollOffset > maxOffset) scrollOffset = maxOffset;

					for (int i = 0; i < visibleRows; i++) {
						// faded text color
						float fadeValue = (i <= centerRow) ? (i + 1) * 0.3f : (visibleRows - i) * 0.3f;

						// Calcolo empirico del contrasto per il colore testo rispetto al background (solo file selezionato)
						Color fadeColor = darkenColor(primaryColor,fadeValue);
						Color selColor = accentColor.g > 180 || (accentColor.r + accentColor.g + accentColor.b) > 450 ? BLACK : WHITE;

						DrawLine(filesArea.x, filesArea.y + (i*rowHeight), screenWidth-8, filesArea.y +(i*rowHeight),borderColor);

						int fileIndex = scrollOffset + i;
						if (fileIndex > totalFiles) break;

                        // even/odd row background
                        // if (i % 2 == 1) DrawRectangleRec((Rectangle){filesArea.x+1,filesArea.y +(i*rowHeight),filesArea.width-2,rowHeight-1}, lightenColor(bgColor, 0.032f));

						if (fileIndex == selectedIndex) DrawRectangle(filesArea.x,filesArea.y +(i*rowHeight),filesArea.width,rowHeight-1, accentColor);
							// print INDEX
                            DrawTextEx(filesFnt,TextFormat("%04i",fileIndex + 1),(Vector2){filesArea.x + 6, filesArea.y +(i*rowHeight)+2},(float)filesFnt.baseSize,0,(fileIndex == selectedIndex)? selColor : fadeColor );
                            // print truncate too long filenames (max. 88 chars)
                            const char *fileNameTrunc = TextSubtext(GetFileName(files[fileIndex]),0,88); 
							DrawTextEx(filesFnt,TextFormat("%s",fileNameTrunc), (Vector2){filesArea.x + 46, filesArea.y +(i*rowHeight)+2},(float)filesFnt.baseSize,0,(fileIndex == selectedIndex)? selColor : fadeColor);
						
						}   
				//vertical divider between index and filename 
				DrawLine(filesArea.x+40,filesArea.y,filesArea.x+40,filesArea.y + filesArea.height,borderColor);
			EndScissorMode();
			}
                            

		
        //volume icon and its percentage value

        for (int i = 0; i < 100; i+=4) DrawLine(12+i,360,12+i,360-(i/4),(i<currVolume*100)?accentColor:borderColor);

		DrawTexture(volumeIcon,infoArea.x+4,infoArea.y,isMute?borderColor:secondaryColor);
		   if (!isMute) DrawTextEx(defaultFnt,TextFormat("%.f%%",currVolume*100),(Vector2){infoArea.x+44,infoArea.y+6}, (float)defaultFnt.baseSize,0,primaryColor);
		   else DrawTextEx(defaultFnt,TextFormat("%.f%%",currVolume*100),(Vector2){infoArea.x+44,infoArea.y+6}, (float)defaultFnt.baseSize,0, borderColor);
		
		// tempo avanzamento brano centrato orizzontalmente
		DrawTextEx(digitFnt,curTimeStr,(Vector2){(screenWidth/2)-(timeText.x/2),infoArea.y},(float)digitFnt.baseSize,0, primaryColor);
		
		// timeProgress bar lunga in base al font usato per stampare i tempi del brano
		DrawRectangle((screenWidth/2)-(timeText.x/2),infoArea.y+28,timeText.x,3,borderColor); // sfondo timeProgress bar
		for (int i = 0; i < (timeProgress * timeText.x); i++) DrawRectangleRec((Rectangle){(screenWidth/2)-(timeText.x/2),infoArea.y+27,i,5},accentColor);

		// tempo totale brano centrato orizzontalmente
		DrawTextEx(digitFnt,totTimeStr,(Vector2){(screenWidth/2)-(timeText.x/2),infoArea.y+35},(float)digitFnt.baseSize,0, secondaryColor);

		// current song position / total songs
		DrawTextEx(defaultFnt,TextFormat("%04d",currPlay + 1),(Vector2){infoArea.x+210,infoArea.y+20},(float)defaultFnt.baseSize,1, accentColor);
		DrawTextEx(defaultFnt,TextFormat("%04d",totalFiles),(Vector2){infoArea.x+360,infoArea.y+20},(float)defaultFnt.baseSize,1, accentColor);

		// STOP flag
		DrawRectangle(xyFlags.x-8,xyFlags.y+298,4,12,isStop ? accentColor:borderColor);
		DrawTextEx(defaultFnt,"STOP",(Vector2){xyFlags.x,xyFlags.y+295},(float)defaultFnt.baseSize,1, isStop ? WHITE : borderColor);

		// PLAY flag
		DrawRectangle(xyFlags.x-8,xyFlags.y+318,4,12,isPlay ? accentColor:borderColor);
		DrawTextEx(defaultFnt,"PLAY",(Vector2){xyFlags.x,xyFlags.y+315},(float)defaultFnt.baseSize,1, isPlay ? WHITE : borderColor);

		// PAUSE flag
		DrawRectangle(xyFlags.x-8,xyFlags.y+338,4,12,isPause ? accentColor:borderColor);
		DrawTextEx(defaultFnt,"PAUSE",(Vector2){xyFlags.x,xyFlags.y+335},(float)defaultFnt.baseSize,1, isPause ? WHITE : borderColor);

		// Shuffle flag
		DrawRectangle(xyFlags.x-8,xyFlags.y+358,4,12,isShuffle ? accentColor:borderColor);
		DrawTextEx(defaultFnt,"SHUFFLE",(Vector2){xyFlags.x,xyFlags.y+355},(float)defaultFnt.baseSize,1, isShuffle ? WHITE : borderColor);

		//statusbar with some info
		//DrawText(TextFormat("%s", TOOL_SHORT_NAME), 18, screenHeight-18, 10, secondaryColor); 
		DrawText(TextFormat("v%s", TOOL_VERSION), 18,screenHeight-18, 10, borderColor); 
		//DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);

	EndDrawing();
}
	
	//unload resource
	DetachAudioMixedProcessor(AudioProcessCallback); // disconnect audio preocessor for vumeter
	UnloadMusicStream(currMusic);
	if (state == STATE_CROSSFADING) UnloadMusicStream(nextMusic);
	
    UnloadRenderTexture(target);
    UnloadTexture(background);
    UnloadTexture(volumeIcon);
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
