/*******************************************************************************
*
*   raylib simple currMusic player
*   Small utility to play mp3 currMusicFiles based on Raylib
*   
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************/
		
#define TOOL_NAME               "Simple Music Player"
#define TOOL_SHORT_NAME         "simplayer"
#define TOOL_COMMENT            "Simple but modern currMusic player written in C99 using Raylib - Play MP3 and OGG file"
#define TOOL_VERSION            "2.1.5"

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
#include <sys/types.h>
#include <pwd.h>

// gcc -Wall -Werror simplayer.c  -o simplayer -lraylib -lm -lid3tag
// archlinux : pacman -S raylib libid3tag

// window initial size
#define screenWidth   640
#define screenHeight  408

// faded songs (time in seconds)
#define FADE_TIME 5.0f // Durata della dissolvenza in secondi
typedef enum {
	STATE_PLAYING_SINGLE, // esecuzione normale
	STATE_CROSSFADING     // dissolvenza incrociata tra due brani
} PlayerState;

PlayerState state = STATE_PLAYING_SINGLE;
float fadeTimer;

// define stream 
	Music currMusic = { 0 };
	Music nextMusic = { 0 };

// variables
#define SEEK_TIME 10.0f // seek time

bool isPlay = true;
bool isShuffle = true;
bool isStop = true;
bool isPause = false;  
bool isMute = false;
bool isVumeter = true; // true:default : show 36 band equalizer , if set false show file library
float currVolume = 0.80f;            // Default audio currVolume [0.0f..1.0f]
float prevVolume = 0.50f;
char *currMusicDir;

// Music library && files management
#define FILTER_MP3      ".mp3"
#define FILTER_OGG      ".ogg"

size_t trackCount;
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

#define MAX_COLORS_COUNT    21// Number of colors available (BLACK & WHITE are excluded)
// custom Colors
#define myYELLOW     CLITERAL(Color){ 255, 233, 3, 255 }     // Yellow / Giallo Modena Ferrari
#define myGOLD       CLITERAL(Color){ 239,191,4, 255 }     // Gold
#define myORANGE     CLITERAL(Color){ 255, 128, 0, 255 }     //  Orange Mclaren Papaya
#define myPINK       CLITERAL(Color){ 255, 192, 203, 255 }     //  Pink Panther
#define myRED        CLITERAL(Color){ 205, 33, 42, 255 }     //  Red /Rosso bandiera
#define myMAROON     CLITERAL(Color){ 148,34,34, 255 }     //  Maroon / Granata
#define myGREEN      CLITERAL(Color){ 141, 198, 84, 255 }      // Green
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

// Colors to choose from
const Color colors[MAX_COLORS_COUNT] = {
        myYELLOW, myGOLD, myORANGE, myPINK, myRED, myMAROON, myGREEN, myLIME, myDARKGREEN,
        mySKYBLUE, myBLUE, myDARKBLUE, myPURPLE, myVIOLET, myDARKPURPLE, myBEIGE, myBROWN, myDARKBROWN,
        myLIGHTGRAY, myGRAY, myDARKGRAY };
//

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

// variabile per la ricerca
int inputBuffer = 0;       // Accumula i numeri digitati
bool hasTyped = false;     // Flag per lo stato del timer
float lastTypeTime = 0.0f; // Timestamp dell'ultima cifra inserita
const float timeoutDuration = 2.0f;


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
	currMusic = LoadMusicStream(files[idx]);
	GetTagsFromFilename(idx);
}

void LoadNextMusicByIndex(int idx) {
	currPlay = idx;
	nextMusic = LoadMusicStream(files[idx]);
	//GetTagsFromFilename(idx);
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

void getMusicFolder(void) {
	// currMusic folder must be a folder called "Music" inside your home dir (could be also aa symbolic link to real Music dir)
	if ((currMusicDir = getenv("HOME")) == NULL) currMusicDir = getpwuid(getuid())->pw_dir;
	strcat(currMusicDir,"/Music\0");
}


int load_files_recursive(const char *path, char files[MAX_FILES][MAX_NAME], int count)
{

	if (!DirectoryExists(path)) {
		// Failed to locate, write the error and kill the program.
		TraceLog(LOG_ERROR, "Failed to locate: %s directory; exit program!", currMusicDir);
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
	Font filesFnt= LoadFontEx("fonts/PixelOperator.ttf", 16, NULL, 0); 

	SetTextureFilter(defaultFnt.texture, TEXTURE_FILTER_POINT);
	RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);  
	
	//load currVolume icon
	Image volumeICO = LoadImage("assets/volumeICO.png");     // Loaded in CPU memory (RAM)
	Texture2D volumeICO_texture = LoadTextureFromImage(volumeICO);          // Image converted to texture, GPU memory (VRAM)
	UnloadImage(volumeICO);   // Once image has been converted to texture and uploaded to VRAM, it can be unloaded from RAM
	
	Image image = LoadImage("assets/background.png");     // Loaded in CPU memory (RAM)
	Texture2D background = LoadTextureFromImage(image);          // Image converted to texture, GPU memory (VRAM)
	UnloadImage(image); 

	
// some custom colors
Color accentColor = SKYBLUE; //colors[GetRandomValue(0,MAX_COLORS_COUNT-1)]; // choose a random color from RAAYLIB color table
Color primaryColor = CLITERAL(Color){253, 255, 255, 255}; // almnost WHITE
Color secondaryColor = CLITERAL(Color){156, 158, 158, 255};
Color borderColor = CLITERAL(Color){50, 70, 70, 255};
// Color bgColor = CLITERAL(Color){10, 20, 30, 248};


	// init Audio
	InitAudioDevice();
	SetAudioStreamBufferSizeDefault(65535);


// file open/save variables

			// load currMusic files into array
			getMusicFolder();
			int trackCount = load_files_recursive(currMusicDir, files, 0);

			// load first song to play  based on shuffle setting
				selectedIndex = isShuffle ? GetRandomValue(0,trackCount-1) : 0;
				LoadMusicByIndex(selectedIndex);
				prevPlay=selectedIndex;

			// auto play song based on autoplay setting
			if (isPlay) {
				isStop=false;
				isPause =false;
				PlayMusicStream(currMusic);  // autoplay at start
			}

			// init Audio Processor
			AttachAudioMixedProcessor(AudioProcessCallback);

			// set FPS (uso questo sistema per regolare la velocità di scorrimento)
			//SetTargetFPS(60);// https://bedroomcoders.co.uk/posts/218

			// scroll title / id3
			Rectangle displayArea = { 18, 24, 532,90 };
			float titleX = displayArea.x ;
			float speed = 60.0f;

			// filelist variables
			Rectangle filesArea = { 16,108,screenWidth-32,206 };
			int rowHeight = 23;
			int visibleRows = 9;
			const int centerRow = 4;
			int scrollOffset = 0;     // primo file visualizzato

			// visualizer  area / variables for effects
			Rectangle visArea = {16,110,screenWidth-32,190};

			//infoArea e calcolo lunghezza timeProgressbar
			Rectangle infoArea = {18,325,screenWidth-30,110};
			Vector2 timeText = MeasureTextEx(digitFnt, "00:00:00", (float)digitFnt.baseSize, 0);

			// flag area / currMusic file info
			Vector2 xyFlags= {556,20};

		 //  vumeter
			Complex fftBuffer[MAX_SAMPLES];
			// Parametri dinamici di calibrazione
			float minDb = -55.0f; 
			float maxDb = -0.0f; // sensibilita Decibel
			float maxSeenMagnitude = 0.01f; // Auto-gain tracker

	// fai riapparire finestra dopo caricamento iniziale
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

		// set scroll text speed
		Vector2 titleSize = MeasureTextEx(titleFnt, titleStr, 36, 0);
		float titleWidth = titleSize.x;

		bool needScroll = titleWidth > displayArea.width;
		float dt = GetFrameTime();
		if (needScroll) {
			titleX -= (speed * dt);
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


	// faded songs
	// set initial currVolume 
	SetMusicVolume(currMusic, currVolume);
	SetMusicVolume(nextMusic, currVolume);
	
		// update sound stream
		UpdateMusicStream(currMusic);
		if (state == STATE_CROSSFADING) UpdateMusicStream(nextMusic);

		switch (state)
		{
			case STATE_PLAYING_SINGLE:
				// Se mancano meno di FADE_TIME secondi alla fine dell brano corrente  parte il crossfade

				if (timeRemaining <= FADE_TIME && timeLength > FADE_TIME) 
				{
					prevPlay=selectedIndex;
					// Passa al brano successivo (se SHUFFLE brano a caso altrimenti successivo nella lista)
					if (isShuffle) {
						int shuffleIndex = GetRandomValue(0,trackCount-1);
						if (shuffleIndex == trackCount) --shuffleIndex;
						// if new song is equal to current , select next one
						if (trackCount > 0 && shuffleIndex == selectedIndex) shuffleIndex = (shuffleIndex + 1) % trackCount;
						selectedIndex = shuffleIndex;
						} 
					else selectedIndex = (selectedIndex + 1) % trackCount;
					
					LoadNextMusicByIndex(selectedIndex);
					PlayMusicStream(nextMusic);
					SetMusicVolume(nextMusic, 0.0f); // Parte silenzioso

					fadeTimer = 0.0f;
					state = STATE_CROSSFADING;
				}
				//  caso in cui il brano finisce improvvisamente (o è troppo corto per il fade)
				else if ((!IsMusicStreamPlaying(currMusic) || timeRemaining <= 0.1f) && !isPause && !isStop)
				{
					UnloadMusicStream(currMusic);
				  prevPlay=selectedIndex;
					if (isShuffle) {
						int shuffleIndex = GetRandomValue(0,trackCount-1);
						if (shuffleIndex == trackCount) --shuffleIndex;
						// if new song is equal to current , select next one
						if (trackCount > 0 && shuffleIndex == selectedIndex) shuffleIndex = (shuffleIndex + 1) % trackCount;
						selectedIndex = shuffleIndex;
						} 
					else selectedIndex = (selectedIndex + 1) % trackCount;
					
					LoadMusicByIndex(selectedIndex);
					PlayMusicStream(currMusic);
					SetMusicVolume(currMusic, currVolume);
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
					UnloadMusicStream(currMusic); // Elimina la vecchia canzone
					currMusic = nextMusic;        // e la nuova canzone diventa quella corrente
					state = STATE_PLAYING_SINGLE;    // Ritorna allo stato normale
					selectedIndex = currPlay;
					// aggiorna info brano
					GetTagsFromFilename(selectedIndex);
				}
				break;
		}

	

		// Set audio currVolume
		if (IsKeyDown(KEY_PAGE_DOWN)) {
			currVolume -= (currVolume >= 0.0f) ? 0.01f : 0.0f;
			if (currVolume < 0.0f) currVolume = 0.0f, isMute=true;
		}

		if (IsKeyDown(KEY_PAGE_UP)) {
			isMute = false;
			currVolume += (currVolume <= 1.0f) ? 0.01f : 0.0f;
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

		if (IsKeyPressed(KEY_LEFT) && isPlay && state == STATE_PLAYING_SINGLE) // seek -10sec
		{
					if (timePlayed < 10.0f) {
						timePlayed = 0.0f; 
						SeekMusicStream(currMusic, 0.0f);
					}
					else SeekMusicStream(currMusic, timePlayed - SEEK_TIME);

		}
		if (IsKeyPressed(KEY_RIGHT) && isPlay && state == STATE_PLAYING_SINGLE) // seek +10sec
		{
					if (timePlayed + SEEK_TIME > timeLength) {
						timePlayed = 0.0f;
						SeekMusicStream(currMusic, 0.0f);
					}
					else  SeekMusicStream(currMusic, timePlayed + SEEK_TIME);
		}

		if (IsKeyPressed(KEY_N)) { // Next song based on SHUFFLE setting
			prevPlay = selectedIndex; //save for 1 shot prev.song
			if (isShuffle) {
				int shuffleIndex = GetRandomValue(0,trackCount-1);
				if (shuffleIndex == trackCount) --shuffleIndex;
				// if new song is equal to current , select next one
				if (trackCount > 0 && shuffleIndex == selectedIndex) shuffleIndex = (shuffleIndex + 1) % trackCount;
				selectedIndex = shuffleIndex;
				} 
			else selectedIndex = (selectedIndex + 1) % trackCount;

				StopMusicStream(currMusic);
				UnloadMusicStream(currMusic);
				LoadMusicByIndex(selectedIndex);
				PlayMusicStream(currMusic);
				isStop=false;
				isPlay=true;
				isPause=false;
			}

		if (IsKeyPressed(KEY_Z)) { // Previous song : no shuffle on previous song
				if (isShuffle) selectedIndex = prevPlay;
				else selectedIndex--;
				if (selectedIndex < 0) selectedIndex=0;
				StopMusicStream(currMusic);
				UnloadMusicStream(currMusic);
				LoadMusicByIndex(selectedIndex);
				PlayMusicStream(currMusic);
				isStop=false;
				isPlay=true;
				isPause=false;
		}

		if (IsKeyPressed(KEY_S)) isShuffle = !isShuffle;

		if (IsKeyPressed(KEY_C)) accentColor = colors[GetRandomValue(0,MAX_COLORS_COUNT-1)];

		//------------------------------------------------------------------------------
		// scrollFiles with mouse and keybinding in music library
		//------------------------------------------------------------------------------
		if (!isVumeter) {
			if (trackCount >= visibleRows) {
						selectedIndex  = -(int)GetMouseWheelMove() + selectedIndex;  
			
						if (IsKeyPressed(KEY_X)) selectedIndex = currPlay;
						if (IsKeyPressed(KEY_HOME)) selectedIndex = 0;
						if (IsKeyPressed(KEY_END)) selectedIndex = trackCount -1;
			
						if (IsKeyPressed(KEY_DOWN)) selectedIndex++;
						if (IsKeyPressed(KEY_UP)) selectedIndex--;
						if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
							prevPlay = currPlay; //save for 1 shot prev.song
							if (selectedIndex >= 0 && selectedIndex < trackCount) {
								StopMusicStream(currMusic);
								//UnloadMusicStream(currMusic);
								LoadMusicByIndex(selectedIndex);
								PlayMusicStream(currMusic);
								//UpdateMusicStream(currMusic);
								isStop=false;
								isPlay=true;
								isPause=false;
								}
							}
					// checks
							if (selectedIndex < 0) selectedIndex=0;
							if (selectedIndex > trackCount-1) selectedIndex=trackCount-1;
				}
			}

		if (IsKeyPressed(KEY_F)) isVumeter = !isVumeter;

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

// numeric search --------------------------------------------------------------
		float currentTime = (float)GetTime();

// 1. Gestione del Timeout di 2 secondi
if (hasTyped && (currentTime - lastTypeTime > timeoutDuration)) {
    inputBuffer = 0;
    hasTyped = false;
}

// 2. Intercettazione tasti numerici (0-9)
int keyPressed = GetKeyPressed();
if (keyPressed >= KEY_ZERO && keyPressed <= KEY_NINE) {
    int digit = keyPressed - KEY_ZERO;
    
    // Costruzione dinamica del numero per avvicinamento
    inputBuffer = (inputBuffer * 10) + digit;
    lastTypeTime = currentTime; 
    hasTyped = true;

    // Controllo dei limiti su base 0 (Clamping)
    if (inputBuffer >= trackCount) selectedIndex = trackCount - 1;
    else selectedIndex = inputBuffer - 1;
}

// 3. Tasti di controllo (Opzionali ma raccomandati)
if (IsKeyPressed(KEY_BACKSPACE) && hasTyped) {
    inputBuffer /= 10;
    selectedIndex = inputBuffer;
    lastTypeTime = currentTime;
    if (inputBuffer == 0) hasTyped = false;
}

if (IsKeyPressed(KEY_DELETE)) {
    inputBuffer = 0;
    selectedIndex = 0;
    hasTyped = false;
}
//------------------------------------------------------------------------------

		// // do something when  window loses focus
		if (IsWindowState(FLAG_WINDOW_UNFOCUSED)) SetWindowOpacity(0.80f);
		else SetWindowOpacity(1.0f);
//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------
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

			// song Artist
			DrawLineDashed((Vector2){displayArea.x,displayArea.y+39},(Vector2){displayArea.width+20,displayArea.y+39},1,1,accentColor);
			DrawTextEx(artistFnt, artistStr, (Vector2){ displayArea.x, displayArea.y+42 }, (float)artistFnt.baseSize, 0, secondaryColor);
		EndScissorMode();

		// file type / KHz / stereo - mono  of current song
		DrawTextEx(defaultFnt,TextFormat("%s",TextToUpper(GetFileExtension(files[selectedIndex]))),(Vector2){xyFlags.x+24,xyFlags.y+3}, (float)defaultFnt.baseSize,1, primaryColor);
		DrawTextEx(defaultFnt,TextFormat("%i kHz",currMusic.stream.sampleRate/1000),(Vector2){xyFlags.x+24,xyFlags.y+23}, (float)defaultFnt.baseSize,0, secondaryColor);
		DrawTextEx(defaultFnt,TextFormat("%i bits",currMusic.stream.sampleSize),(Vector2){xyFlags.x+24,xyFlags.y+43}, (float)defaultFnt.baseSize,0, secondaryColor);
		DrawTextEx(defaultFnt,TextFormat("%s", (currMusic.stream.channels == 1)? "mono" : (currMusic.stream.channels == 2)? "stereo" : "multi"),(Vector2){xyFlags.x+24,xyFlags.y+63}, (float)defaultFnt.baseSize,0, secondaryColor);

	if (isVumeter) { 
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
						Color barColor = lightenColor(accentColor,0.60f);//colors[GetRandomValue(0,MAX_COLORS_COUNT-1)];
						
						int segmentsToLight = (int)(barValues[i] * maxSegments); 
						int peakSegment = (int)(peakValues[i] * maxSegments) - 1;
						if (peakSegment < 0 && peakValues[i] > 0.01f) peakSegment = 0;

						for (int j = 0; j < maxSegments; j++) {
							float segYPos = baseYPos - (j * (segmentHeight + segmentGap)) - segmentHeight;
							// Logica di disegno combinata barra + picco
							bool drawActiveSegment = (j < segmentsToLight);
							bool drawPeakSegment = (j == peakSegment);
							DrawLine(xPos, segYPos,xPos +(barWidth - barSpacing), segYPos, (drawActiveSegment || drawPeakSegment) ? barColor:darkenColor(accentColor,0.36f) );
						 }
					}
		EndScissorMode();
	}
		else {
			// draw file selection
			//DrawRectangle(0,filesArea.y-4,screenWidth,filesArea.height+8,darkenColor(accentColor,0.088f));
			BeginScissorMode( (int)filesArea.x, (int)filesArea.y, (int)filesArea.width, (int)filesArea.height);
				if (trackCount < visibleRows) visibleRows = trackCount;

				scrollOffset = selectedIndex - centerRow;
				if (scrollOffset < 0) scrollOffset = 0;
				int maxOffset = trackCount - visibleRows;
				if (maxOffset < 0) maxOffset = 0;
				if (scrollOffset > maxOffset) scrollOffset = maxOffset;

					for (int i = 0; i < visibleRows; i++) {
						// faded text color
						float fadeValue = (i <= centerRow) ? (i + 1) * 0.3f : (visibleRows - i) * 0.3f;

						// Calcolo dinamico del contrasto per il testo
						Color fadeColor = darkenColor(primaryColor,fadeValue);
						Color selColor = (accentColor.r + accentColor.g + accentColor.b) / 3 > 128 ? BLACK : WHITE;
						
						DrawLine(filesArea.x, filesArea.y + (i*rowHeight), screenWidth-8, filesArea.y +(i*rowHeight),borderColor);
						// even/odd row background
						//if (i % 2) DrawRectangleRec((Rectangle){filesArea.x+1,filesArea.y +(i*rowHeight),filesArea.width-2,rowHeight-1}, darkenColor(accentColor,0.12f));
						int fileIndex = scrollOffset + i;
						if (fileIndex > trackCount) break;
						if (fileIndex == selectedIndex) DrawRectangle(filesArea.x,filesArea.y +(i*rowHeight),filesArea.width,rowHeight-1, accentColor);
							DrawTextEx(filesFnt,TextFormat("%04i",fileIndex + 1),(Vector2){filesArea.x + 6, filesArea.y +(i*rowHeight)+2},(float)filesFnt.baseSize,0,(fileIndex == selectedIndex)? selColor : fadeColor );
							DrawTextEx(filesFnt,TextFormat("%s",GetFileName(files[fileIndex])),(Vector2){filesArea.x + 52, filesArea.y +(i*rowHeight)+2},(float)filesFnt.baseSize,0,(fileIndex == selectedIndex)? selColor : fadeColor);
						
						}   
				//vertical divider
				DrawLine(filesArea.x+44,filesArea.y,filesArea.x+44,filesArea.y + filesArea.height,borderColor);
			EndScissorMode();
			}



		//volume icon and its percentage value
		DrawTexture(volumeICO_texture,infoArea.x-4,infoArea.y +14,isMute?borderColor:secondaryColor);
		   if (!isMute) DrawTextEx(defaultFnt,TextFormat("%.f%%",currVolume*100),(Vector2){infoArea.x+32,infoArea.y+21}, (float)defaultFnt.baseSize,0,primaryColor);
		   else DrawTextEx(defaultFnt,TextFormat("%.f%%",currVolume*100),(Vector2){infoArea.x+32,infoArea.y+21}, (float)defaultFnt.baseSize,0, borderColor);
		 
		// tempo avanzamento brano centrato orizzontalmente
		DrawTextEx(digitFnt,curTimeStr,(Vector2){(screenWidth/2)-(timeText.x/2),infoArea.y},(float)digitFnt.baseSize,0, primaryColor);
		
		// timeProgress bar lunga in base al font usato per stampare i tempi del brano
		DrawRectangle((screenWidth/2)-(timeText.x/2),infoArea.y+28,timeText.x,3,borderColor); // sfondo timeProgress bar
		for (int i = 0; i < (timeProgress * timeText.x); i++) DrawRectangleRec((Rectangle){i+((screenWidth/2)-(timeText.x/2)),infoArea.y+28,1,3},accentColor);



		// tempo totale brano centrato orizzontalmente
		DrawTextEx(digitFnt,totTimeStr,(Vector2){(screenWidth/2)-(timeText.x/2),infoArea.y+34},(float)digitFnt.baseSize,0, secondaryColor);

		// current song position / total songs
		DrawTextEx(defaultFnt,TextFormat("%04d",currPlay + 1),(Vector2){infoArea.x+210,infoArea.y+20},(float)defaultFnt.baseSize,1, secondaryColor);
		DrawTextEx(defaultFnt,TextFormat("%04d",trackCount),(Vector2){infoArea.x+360,infoArea.y+20},(float)defaultFnt.baseSize,1, secondaryColor);


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
		//DrawText(TextFormat("%s", TOOL_SHORT_NAME), 8, screenHeight-16, 10, secondaryColor); 
		DrawText(TextFormat("version %s", TOOL_VERSION), 18,screenHeight-18, 10, borderColor); 
		//DrawText("[Q] exit program.",screenWidth-94, screenHeight-16,10,GRAY);

	EndDrawing();
}
	
	//unload resource
	DetachAudioMixedProcessor(AudioProcessCallback); // disconnect audio preocessor for vumeter
	UnloadMusicStream(currMusic);
	if (state == STATE_CROSSFADING) UnloadMusicStream(nextMusic);
	
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
