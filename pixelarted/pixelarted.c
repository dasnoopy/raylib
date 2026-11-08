/*******************************************************************************************
*
*   PIXEL ART EDITOR
*   A simple pixel perfect editor  to learn C using raylib/raygui library
*
*   Copyright (c) 2026 Andrea Antolini (@dasnoopy)
*
********************************************************************************************/

#define TOOL_NAME               "Pixel Art Editor"
#define TOOL_SHORT_NAME         "PixelArtEd"
#define TOOL_VERSION            "3.3.1"

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <raylib.h>
// raygui integration
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

const int screenWidth = 1080;
const int screenHeight = 732;

#define numCols       32
#define numRows       32
const int cellSize = 20;

// library & files management
size_t fileCount;
#define MAX_FILES 4096
#define MAX_NAME 1024
#define FILTER_PNG      ".png"
char files[MAX_FILES][MAX_NAME];
char fName[MAX_NAME] = "default.png";
bool fnameEditMode = false;
int selectedIndex = 0;
int rowHeight = 21;
int visibleRows;
int maxRows = 25;
int centerRow;
int scrollOffset = 0;     // primo file visualizzato

// barra path su Linux oppure su WIN
#ifdef _WIN32
#define SEPARATOR "\\"
#else
#define SEPARATOR "/"
#endif

#define MAX_COLORS_COUNT    24          // Number of colors available

 // initial X,Y coordinates for variuos interface elements
const Vector2 colorsBarPos = {40, 360};
const Vector2 spriteGridPos = {220, 48};
const Vector2 miniaturePos = {44,64};
const Vector2 toolbarPos = {40,200};
const Vector2 keyinfoPos = {912,620};
const Vector2 colorsInfoPos = {32,600};
const Rectangle libraryPos = {906,48, 160,526};
const Rectangle drawingArea = { spriteGridPos.x,spriteGridPos.y, numCols*cellSize,numRows*cellSize };

// Definizione variabili
int colorMouseHover = 0;
int miniatureScale= 3;
bool showGrid = true;
bool mouseHoverCells = false;

bool isEditing = false;
bool isDrawHmirr = false;
bool isDrawVmirr = false;
bool isFloodFill = false;
bool isColorRepl = false;
bool isDrawing = true;
// gestione stato load & save file
bool isOverLib = false;
bool isDebug = false;

// base colors palette (Vivaci / High-Saturation Variant)
Color baseColors[MAX_COLORS_COUNT] = {
    (Color){   0,   0,   0,   0 },  // Traansparent
    (Color){ 255, 255, 255, 255 },  // White
    (Color){ 255, 233,   3, 255 },  // Yellow 
    (Color){ 239, 191,   4, 255 },  // Gold
    (Color){ 255, 128,   0, 255 },  // Orange Mc
    (Color){ 255, 192, 203, 255 },  // Pink 
    (Color){ 205,  33,  42, 255 },  // Red 
    (Color){ 148,  34,  34, 255 },  // Dark Red
    (Color){ 141, 198,  84, 255 },  // Green
    (Color){  70, 163,  41, 255 },  // Lime
    (Color){  32, 104,  17, 255 },  // Dark Green
    (Color){  25, 174, 255, 255 },  // Sky Blue
    (Color){   0, 132, 200, 255 },  // Blue
    (Color){   0,  92, 148, 255 },  // Dark Blue
    (Color){ 144,  99, 205, 255 },  // Purple
    (Color){ 112,  74, 191, 255 },  // Violet
    (Color){  66,  49, 137, 255 },  // Dark Purple
    (Color){ 217, 182, 154, 255 },  // Beige
    (Color){ 121,  85,  61, 255 },  // Brown
    (Color){  73,  55,  43, 255 },  // Dark Brown
    (Color){ 189, 205, 212, 255 },  // Light Gray
    (Color){ 111, 131, 136, 255 },  // Gray
    (Color){  54, 78,   89, 255 },  // Dark Gray
    (Color){  10, 15,   20, 255 }   // Black
};

Color colors[MAX_COLORS_COUNT];

Color selectedColor = (Color){10,15,20,255 }; // Black
Color currentColor = (Color){0,0,0,0}; // Transparent

// Define colorsRecs data
Rectangle colorsRecs[MAX_COLORS_COUNT];

// ARDUINO Matrix tool colors (light)
#define FG_COLOR CLITERAL(Color){ 55, 65, 70, 255}
#define BG_COLOR CLITERAL(Color){ 218, 227, 227, 255} 
// grid and checkerboard
#define GRID_COLOR CLITERAL(Color){ 155, 160, 163, 255} 
#define GRID_BG_COLOR CLITERAL(Color){ 236, 241, 241, 255} 
// some funs
#define ON_COLOR CLITERAL(Color){ 12, 161, 166, 255}
#define OFF_COLOR CLITERAL(Color){ 242, 103, 39,255}
#define BORDER_COLOR CLITERAL(Color){ 120, 122,122, 255} 

// bottoni toolbar
// UI required variables
bool btnGrid = false;
bool btnNew = false;
bool btnClear = false;
bool btnHmirr = false;
bool btnVmirr = false;
bool btnShtUp = false;
bool btnShtDn = false;
bool btnShtSx = false;
bool btnShtDx = false;
bool btnRotate = false;
bool btnBruHor = false;
bool btnBruVer = false;
bool btnQuit = false;
float pastelSlider = 0.0f; 

// Key repeat structure
typedef struct {
    int currentKey;      // Il tasto attualmente premuto
    float timer;         // Timer per calcolare i delay
    bool isRepeating;    // true se ha superato il delay iniziale
    float firstDelay;    // Tempo di attesa iniziale 
    float repeatRate;    // Intervallo di ripetizione 
} KeyRepeater;

// Point struct, like Vector2 but using int
int px,py,px1,py1;
typedef struct {
    int x;
    int y;
} Point;

// Player state struct
typedef struct {
    Point cell;
    Color color;
} PlayerState;

// Struttura per gestire lo stato della matrice current ,mirror,  undo e redo
typedef struct {
    Color current[numRows][numCols]; // matrice attiva dove disegno
    Color mirror[numRows][numCols]; // matrice per funzione mirror
    Color undoBuffer[numRows][numCols];
    Color redoBuffer[numRows][numCols];
    bool hasUndo;
    bool hasRedo;
} EditorState;

EditorState editor; // 

//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------
// azzera matrice colore
void resetMatrix(void)
{
    for (int row = 0; row < numRows; row++)
        for (int col = 0; col < numCols; col++)
            editor.current[row][col] = BLANK; // (Color){col*32,row*32,128,255};
}

void copyMatrix(EditorState *state)
{
    // for(int i=0; i<N; i++)
    //     for(int j=0; j<M; j++) dst[(M*i)+j] = src[(M*i)+j];
    memcpy(state->mirror, state->current, sizeof(state->current));
}

// Salva lo stato corrente nell'Undo prima di una modifica
void SaveUndoState(EditorState *state) {
    // Copia la matrice corrente nel buffer di undo
    memcpy(state->undoBuffer, state->current, sizeof(state->current));
    state->hasUndo = true;
    
    // Una nuova azione cancella la possibilità di fare Redo
    state->hasRedo = false; 
}

// Esegue l'Undo (Annulla)
void PerformUndo(EditorState *state) {
    if (!state->hasUndo) return;

    // Salva lo stato current in redo
    memcpy(state->redoBuffer, state->current, sizeof(state->current));
    state->hasRedo = true;

    // da undo a current
    memcpy(state->current, state->undoBuffer, sizeof(state->current));
    
    // reset undo level
    state->hasUndo = false;
}

// Esegue il Redo (Ripristina)
void PerformRedo(EditorState *state) {
    if (!state->hasRedo) return;

    // cuurent in undo
    memcpy(state->undoBuffer, state->current, sizeof(state->current));
    state->hasUndo = true;

    // da redo a current
    memcpy(state->current, state->redoBuffer, sizeof(state->current));

    // reset redo level
    state->hasRedo = false;
}

// Funzione per trasformare dinamicamente un colore in pastello/contrastato
Color GetDynamicPastel(Color baseColor, float pastelFactor) {
    // pastelFactor deve essere normalizzato tra 0.0f e 1.0f
    float factor = pastelFactor / 255.0f;
    
    // Per effetto pastello,mix tra colore base e il bianco (255)
    // Più il fattore è alto, più ci avviciniamo al bianco, riducendo il contrasto.
    Color result;
    result.r = (unsigned char)(baseColor.r + (255 - baseColor.r) * factor);
    result.g = (unsigned char)(baseColor.g + (255 - baseColor.g) * factor);
    result.b = (unsigned char)(baseColor.b + (255 - baseColor.b) * factor);
    result.a = baseColor.a; // Mantieni l'alpha originale
    
    return result;
}

bool AreColorsEqualThreshold(Color c1, Color c2, int threshold) 
{
    return (abs(c1.r - c2.r) <= threshold) &&
           (abs(c1.g - c2.g) <= threshold) &&
           (abs(c1.b - c2.b) <= threshold) &&
           (c1.a == c2.a); // Alpha deve essere identico
}

// Funzione  per confrontare due colori
bool AreColorsEqual(Color c1, Color c2) 
{
    // return (c1.r == c2.r) && 
    //        (c1.g == c2.g) && 
    //        (c1.b == c2.b) && 
    //        (c1.a == c2.a);

    //
    return *(uint32_t*)&c1 == *(uint32_t*)&c2;
    // raylib ha gia cmq la funzione ColorIsEqual
}

// Restituisce un colore leggibile (BIANCO o NERO) in contrasto al colore di sfondo
Color GetReadableTextColor(Color bgColor) {
    
    // intercetta colori con trasparenza quasi a zero (che visivamente si fa fatica a distinguere)
    if (bgColor.a <= 24) return BLACK;

    // Formula della luminanza percepita dall'occhio umano
    float luminance = 0.299f * bgColor.r + 0.587f * bgColor.g + 0.114f * bgColor.b;
    return (luminance > 127.5f) ? BLACK : WHITE;
}

// trova i colori uguali nella matrice e mostra una "X" all'interno della "cella" (KEY_D)
void showSimilColors(Color color)
{
    Color textColor = GetReadableTextColor(color);
    for (int row = 0; row < numRows; row++)
        for (int col = 0; col < numCols; col++) 
            if (AreColorsEqual(editor.current[row][col], color)) DrawText("#", 6 + (spriteGridPos.x + cellSize*col), 6 + (spriteGridPos.y + cellSize*row),10, textColor);
}

void replaceColor(Color old, Color new)
{
    for (int row = 0; row < numRows; row++) 
        for (int col = 0; col < numCols; col++) 
             if (ColorIsEqual(editor.current[row][col],old)) editor.current[row][col] = new;
}

void floodFill(int row, int col, Color oldColor, Color newColor)
{
    int checkRow; /* The positions we're checking currently */
    int checkCol;
    struct /* vectors for checking adjacent cells */
    {
        int dx;
        int dy;
    }   adjacent[] = {{-1, 0}, {0, 1}, {1, 0}, {0, -1}};

    if(!ColorIsEqual(oldColor,newColor))
    {
        editor.current[row][col] = newColor;
        for(int i = 0; i < 4; i++)
        {
            checkRow = row + adjacent[i].dx;
            checkCol = col + adjacent[i].dy;
            if((checkRow < numRows) && (checkCol < numCols)) /* within window boundaries */
            {
                if((checkRow >= 0) && (checkCol >= 0))
                {
                    if(ColorIsEqual(editor.current[checkRow][checkCol],oldColor))
                    {
                        editor.current[checkRow][checkCol] = newColor;
                        floodFill(checkRow, checkCol, oldColor, newColor);
                    }
                }
            }
        }
    }
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

// Draw grid lines 
void drawGridLines(Color border)
{
        for (int row = 0; row <= numRows; row+=1) // horizontal lines
            DrawLineEx((Vector2){spriteGridPos.x, spriteGridPos.y + (row * cellSize)},(Vector2){spriteGridPos.x + (numCols* cellSize), spriteGridPos.y + (row * cellSize)},1, border);
        for (int col = 0; col <= numCols; col+=1) //vertical lines
            DrawLineEx((Vector2){spriteGridPos.x + (col * cellSize), spriteGridPos.y}, (Vector2){spriteGridPos.x + (col * cellSize), spriteGridPos.y + (numRows*cellSize)},1, border);
        DrawRectangleLinesEx(drawingArea,1,border);
}

void drawSprite()
{    
            for (int row = 0; row < numRows; row++)
                {
                    for (int col = 0; col < numCols; col++)
                    {
                        // disegna colore sfondo cella
                        // se si cambia disegno qui, cambiare anche cursore nella sezione BeginDrawing
                        DrawRectangleRec((Rectangle){(spriteGridPos.x + (cellSize * col)) , (spriteGridPos.y + (cellSize * row)), 
                          cellSize, 
                          cellSize }, 
                          editor.current[row][col]);
                    }
                }   
}

void drawThumbnail (void)
{
    // cornice e sfondo miniatura
        DrawRectangleLines(miniaturePos.x-2, miniaturePos.y-2 , 4+(numCols*miniatureScale), 4+(numRows*miniatureScale), BORDER_COLOR);
        DrawRectangle(miniaturePos.x,miniaturePos.y,numCols*miniatureScale,numRows*miniatureScale, GRID_BG_COLOR);
        //miniatura
        for (int row = 0; row < numRows; row++)
            for (int col = 0; col < numCols; ++col) DrawRectangle(miniaturePos.x + (miniatureScale * col), miniaturePos.y + (miniatureScale*row) ,miniatureScale,miniatureScale, editor.current[row][col]);
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

            if (ext && strcmp(ext, FILTER_PNG) == 0) {
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

void loadPngToGrid(const char *filename) {
    // 1. Inizializza TUTTA la matrice a trasparente (cancella lo sporco precedente)
    resetMatrix();

    Image img = LoadImage(filename);
    
    // Sicurezza: controlla se l'immagine è stata caricata correttamente
    if (img.data != NULL) {
        
        // 3. Determina i limiti massimi per non andare fuori dai bordi della matrice
        int maxWidth = (img.width > numCols) ? numCols : img.width;
        int maxHeight = (img.height > numRows) ? numRows : img.height;

        // 4. Copia i pixel dall'immagine alla matrice solo fino alle dimensioni effettive
        for (int y = 0; y < maxHeight; y++) {
            for (int x = 0; x < maxWidth; x++) {
                // GetImageColor estrae il colore RGBA corretto indipendentemente dal formato nativo del PNG
                editor.current[y][x] = GetImageColor(img, x, y);
            }
        }
        UnloadImage(img);
        // 3. resetta lo stato undo
        editor.hasUndo = false;
    }
}

void saveGridToPNG(const char *filename) {
    // Crea l'immagine 32x32 temporanea in RAM
    Image img = GenImageColor(numCols, numRows, BLANK);

    // Trasferisci i dati dall'array bidimensionale ai pixel del PNG
    for (int y = 0; y < numRows; y++) 
    {
        for (int x = 0; x < numCols; x++) 
        {
            Color indiceColore = editor.current[y][x];
            ImageDrawPixel(&img, x, y, indiceColore);
        }
    }
    // Esportiamo l'immagine a 32x32 pixel pixel-perfect in formato .PNG
    if (ExportImage(img, fName)) TraceLog(LOG_INFO, "file salvato con successo!");
    else TraceLog(LOG_ERROR, "Errore durante il salvataggio del file.");

    UnloadImage(img);

    // aggiorna files list
    fileCount = loadFilesRecursive(".", files,0);
        // setta variabile di visualizzazione libreria in base al numero di files presenti...
        if (fileCount<maxRows) visibleRows = fileCount;
        else visibleRows=maxRows;
        centerRow = visibleRows / 2;
    qsort(files, fileCount, MAX_NAME, compareFiles);  // ordinamento file

    // 3. resetta lo stato undo
    editor.hasUndo = false;
}

int main (int argc, char *argv[])
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED ); // | FLAG_MSAA_4X_HINT | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "pixelarted");
        // center window on the screen
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2); 
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetExitKey(KEY_NULL);       // Disable KEY_ESCAPE to close window, X-button still works

    // load TTF font with better antialiasing
    Font font = LoadFontEx("assets/PixelOperator.ttf", 16, 0, 0);
    // load TTF font with better antialiasing
    Font boldfont = LoadFontEx("assets/PixelOperator-Bold.ttf", 16, 0, 0);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    GuiSetIconScale(1);

// make a checkerboard grid for sprite area
    Image img = GenImageChecked(numRows*cellSize, numCols*cellSize, cellSize, cellSize, WHITE, RAYWHITE);
    Texture checkerboard = LoadTextureFromImage(img);
    UnloadImage(img);

    RenderTexture target = LoadRenderTexture(screenWidth, screenHeight);

 // camera2d settings only effect objects between BeginMode2D and EndMode2D
   Camera2D camera = { 0 };
    camera.target = (Vector2) { spriteGridPos.x, spriteGridPos.y};  // centro della zoom area
    camera.offset = (Vector2) { spriteGridPos.x, spriteGridPos.y};   // coordinate x origin zoom area
    camera.rotation = 0;        // rotation in deg
    camera.zoom     = 1.0f;      // magnification, i.e fov or zoom

// load file list and set file library variables
    fileCount = loadFilesRecursive(".", files,0); // carica files .png
    // setta variabile di visualizzazione libreria in base al numero di files presenti...
        if (fileCount<maxRows) visibleRows = fileCount;
        else visibleRows=maxRows;
        centerRow = visibleRows / 2;
    qsort(files, fileCount, MAX_NAME, compareFiles);  // ordinamento file


// Definisci un Rectangle con la poszione dei rettangoli della color palette
    int x=0;
    int y=0;
    const int col = 4;
    const int size = 26;
    const int space = 1;
    for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
        if (i % col == 0) { x=0; y++; }
            colorsRecs[i].x = colorsBarPos.x + (size * x);
            colorsRecs[i].y = colorsBarPos.y + (size * y);
            colorsRecs[i].width = size-space;
            colorsRecs[i].height = size-space;
            x++;
        }

    // Init player 0 (cursor for bin matrix)
        PlayerState player = { 0 };
        player.cell = (Point){ 0, 0 };

        // set key repeater speed (400ms initial delay, 80ms repeat rate)
        KeyRepeater repeater = {
            .currentKey = 0,
            .timer = 0.0f,
            .isRepeating = false,
            .firstDelay = 0.40f,
            .repeatRate = 0.064f
        };

    // 3. Inizializzi lo stato (es. tutto vuoto/trasparente)
    editor.hasUndo = false;
    editor.hasRedo = false;
    
    //reset matrice sprite e copia di backup
    resetMatrix();

    // set FPS
    SetTargetFPS(60);
    ClearWindowState(FLAG_WINDOW_HIDDEN);

while (!WindowShouldClose())
    {
        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
        Vector2 mousePos = GetMousePosition();
        //float speed = 150 * GetFrameTime();
        float deltaTime = GetFrameTime();
        
        //------------------------------------------------------------------------------
        // Choose color with mouse from colors palette
        //------------------------------------------------------------------------------

        for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            if (CheckCollisionPointRec(mousePos, colorsRecs[i]))
            {
                colorMouseHover = i;
                break;
            } else colorMouseHover = -1;
        }

        //LEFT MOUSE :select color
        if ((colorMouseHover >= 0) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) selectedColor = colors[colorMouseHover];

        //------------------------------------------------------------------------------
        // rileva se la posizione mouse e' dentro la matrice sprite...
        //------------------------------------------------------------------------------
        mouseHoverCells = CheckCollisionPointRec(mousePos,drawingArea);
        if (mouseHoverCells)
            {
            HideCursor(); //hide os cursor inside matrix
            isEditing = true;
                 // Icon painting mouse logic
                Vector2 mouseWorldPos = GetScreenToWorld2D(mousePos, camera);

                player.cell.x = (mouseWorldPos.y - spriteGridPos.y) / cellSize ;
                player.cell.y = (mouseWorldPos.x - spriteGridPos.x) / cellSize;

                // Make sure player does not go out of bounds
                if (player.cell.x < 0) player.cell.x = 0;
                else if (player.cell.x >= numRows) player.cell.x = numRows - 1;
                if (player.cell.y < 0) player.cell.y = 0;
                else if (player.cell.y >= numCols) player.cell.y = numCols - 1;

                // aggiorna posizione "cursore" e relativo colore...
                px= player.cell.x;
                py= player.cell.y;
                currentColor = editor.current[px][py];

                    // Salva undo appena il mouse viene premuto
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) SaveUndoState(&editor);

                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_SPACE)) {
                        if (isDrawing) editor.current[player.cell.x][player.cell.y] = selectedColor;
                        if (isDrawHmirr) {
                            editor.current[player.cell.x][player.cell.y] = selectedColor;
                            editor.current[numRows-player.cell.x-1][player.cell.y] = selectedColor;  
                        }
                        if (isDrawVmirr) {
                            editor.current[player.cell.x][player.cell.y] = selectedColor;
                            editor.current[player.cell.x][numCols-player.cell.y-1] = selectedColor;  
                        }
                        if (isColorRepl) replaceColor(currentColor, selectedColor);
                        if (isFloodFill) floodFill(px,py,currentColor,selectedColor);
                    }

                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                    selectedColor = editor.current[player.cell.x][player.cell.y];
                    px1=px;
                    py1=py;
                }
        }
        else { 
            ShowCursor(); //restore os cursor visibility outside matrix
            isEditing = false; // fuori dalla matrice faccio quello che voglio!
        } 

        // ---------------------------------------------------------------------
        // some keybinding action to test functionality
        //----------------------------------------------------------------------
        if (!fnameEditMode) // se stò digitando il nome file nel riquadro di input, disabilita i keybindings
        {
            if (btnQuit || IsKeyPressed(KEY_Q)) break;
            
            else if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                    if (IsKeyPressed(KEY_Z)) PerformUndo(&editor);
                    if (IsKeyPressed(KEY_R)) PerformRedo(&editor);
                }

            else if (IsKeyPressed(KEY_S)) saveGridToPNG(fName);
            else if (IsKeyPressed(KEY_D)) isDebug = !isDebug;
            else if (IsKeyPressed(KEY_L)) {
                    // aggiorna files list
                    fileCount = loadFilesRecursive(".", files,0);
                        // setta variabile di visualizzazione libreria in base al numero di files presenti...
                        if (fileCount<maxRows) visibleRows = fileCount;
                        else visibleRows=maxRows;
                        centerRow = visibleRows / 2;
                    qsort(files, fileCount, MAX_NAME, compareFiles);  // ordinamento file
            }
            else if (btnGrid) showGrid = !showGrid;
            else if (btnNew) {
                    resetMatrix();
                    strcpy(fName,"default.png");
            }
            else if (btnClear) {
                    SaveUndoState(&editor);
                    resetMatrix();
                }

            else if (btnShtDx) // shift bin array right by 1
            {
                for (int row = 0; row < numRows; row++) // righe
                    {
                        // memorizza ultimo bit della riga
                        const Color temp = editor.current[row][numCols - 1];
                        for (int col = numCols-1; col > 0; col--) // colonne
                            editor.current[row][col] = editor.current[row][col-1];
                        // alla fine il "primo" bit prende il valore dell'ultimo
                        editor.current[row][0] = temp;
                    }
            }
            else if (btnShtSx) // shift bin array left by 1
            {
                for (int row = 0; row < numRows; row++) // righe
                    {
                        // memorizza primo bit della riga
                        const Color temp = editor.current[row][0];
                        for (int col = 0; col < numCols-1; col++) // colonne
                            editor.current[row][col] = editor.current[row][col+1];
                        // alla fine ultimo bit prende il valore del prim
                        editor.current[row][numCols-1] = temp;
                    }
            }
            else if (btnShtUp) // shift bin array up by 1
            {
                for (int col = 0; col < numCols; col++) // colonne
                    {
                        // memorizza prima riga
                        const Color temp = editor.current[0][col];
                            for (int row=0; row < numRows-1; row++) // righe
                                editor.current[row][col] = editor.current[row+1][col];
                        // ultima riga prende valori della prima
                        editor.current[numRows - 1][col] = temp;
                    }
            }
            else if (btnShtDn) // shift bin array down by 1
             {
                    for (int col = 0; col < numCols; col++) // colonne
                        {
                            // memorizza stato ultima riga
                            const Color temp = editor.current[numRows - 1][col];
                            for (int row = numRows -1;  row > 0; row--) // righe
                                editor.current[row][col] = editor.current[row-1][col];
                            // prima riga prende valore ultima riga
                            editor.current[0][col] = temp;
                        }
                }

            else if (btnRotate) // rotate matrix clockwise : works only for square matrix e.g 8x8, 16x16 and so on...
             {
                    // trasposizione  matrice binaria
                    for (int row = 0; row < numRows; row++) {
                        for (int col = row + 1 ; col < numCols ; col++) { 
                           Color temp = editor.current[col][row];
                           editor.current[col][row] =  editor.current[row][col];
                           editor.current[row][col] = temp;
                        }
                    }
                    // poi ruota di 90° in senso orario
                    for (int row = 0; row < numRows; row++) {
                        for (int col = 0,k = numCols -1; col<k; col++, k--) { 
                           Color temp = editor.current[row][col];
                           editor.current[row][col] =  editor.current[row][k];
                           editor.current[row][k] = temp;
                        }
                    }
            }

                else if (btnVmirr) // vertical mirror
                {
                    copyMatrix(&editor);
                    for (int row = 0; row < numRows; row++)
                        for (int col = 0; col < numCols; col++) {
                            editor.current[row][numCols -1 - col] = editor.mirror[row][col];
                    }
                }

                else if (btnHmirr) // horizontal mirror
                {
                    copyMatrix(&editor);
                    for (int row = 0; row < numRows; row++)
                        for (int col = 0; col < numCols; col++) {
                            editor.current[numRows -1 - row][col] = editor.mirror[row][col];
                    }
                }

                //---------------------------------------------------------------------
                //  file LIbrary management
                //----------------------------------------------------------------------
                isOverLib = CheckCollisionPointRec(mousePos, libraryPos); 
                // Scroll con tastiera per sopstarsi tra i files (solo se si e' fuori dalla zona di editing)
                 if (!isEditing) {
                    if (fileCount >= visibleRows) 
                    {
                        selectedIndex  = -(int)GetMouseWheelMove() + selectedIndex;  

                        if (IsKeyPressed(KEY_HOME)) selectedIndex = 0;
                        if (IsKeyPressed(KEY_END)) selectedIndex = fileCount -1;

                        // tasti con il repeat rate attivo -----------------------------
                        if (UpdateKeyRepeater(&repeater, KEY_DOWN, deltaTime)) selectedIndex++;
                        else if (UpdateKeyRepeater(&repeater, KEY_UP, deltaTime)) selectedIndex--;
                        else if (UpdateKeyRepeater(&repeater, KEY_PAGE_DOWN, deltaTime)) selectedIndex += 9;
                        else if (UpdateKeyRepeater(&repeater, KEY_PAGE_UP, deltaTime)) selectedIndex -= 9;
                        // -------------------------------------------------------------
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
                            if (selectedIndex >= 0 && selectedIndex < fileCount) {
                                strcpy(fName,files[selectedIndex]);
                                // carica file .png
                                loadPngToGrid(fName);
                                }
                            }
                        // checks
                        if (selectedIndex < 0) selectedIndex=0;
                        if (selectedIndex > fileCount-1) selectedIndex=fileCount-1;
                    }
                }
        }       
//----------------------------------------------------------------------------------
// Draw
//----------------------------------------------------------------------------------
    BeginTextureMode(target);
        ClearBackground(BG_COLOR);
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawRectangle(spriteGridPos.x-34,spriteGridPos.y-36, numCols*cellSize + 68,numRows*cellSize + 68, GRID_BG_COLOR);  //sfondo checkerboard compreso intestazioni riga/colonna
        DrawRectangleLines(spriteGridPos.x-34,spriteGridPos.y-36, numCols*cellSize +68,numRows*cellSize + 68, BORDER_COLOR); //bordo attorno al rettangolo qui sopra!
        DrawText(TextFormat("%s", TOOL_SHORT_NAME), 38, 14, 20, FG_COLOR); 
        DrawText(TextFormat("version %s", TOOL_VERSION), 58, 38, 10, GRAY); 

        //----------------------------------------------------------------------
        // draw sprite and grid matrix inside scissor & camera2d area
        //----------------------------------------------------------------------
    BeginScissorMode((int)drawingArea.x, (int)drawingArea.y, (int)drawingArea.width, (int)drawingArea.height);     
        BeginMode2D(camera);
        //drawCheckerboard(); // just to emulate a transparent background
        DrawTexture(checkerboard, spriteGridPos.x, spriteGridPos.y, WHITE);

        // Draw SPRITE AREA
        drawSprite(); // disegna immagine
        if (isDebug) showSimilColors(currentColor);  // enable just to debug content of matrix
        //----------------------------------------------------------------------
        // Draw cursor moving when inside the sprite grid
        //----------------------------------------------------------------------
        DrawRectangleRec((Rectangle){ spriteGridPos.x + (py*cellSize), spriteGridPos.y + (px*cellSize), 
                          cellSize, 
                          cellSize},
                          Fade(GetReadableTextColor(currentColor), 0.5f));
        //----------------------------------------------------------------------
        // Draw crosshair cursor (when grid is off)
        // ---------------------------------------------------------------------
        if (!showGrid) {
            //vertical
            DrawLineEx((Vector2){ spriteGridPos.x + (py*cellSize) + (cellSize/2), spriteGridPos.y }, 
                       (Vector2){ spriteGridPos.x + (py*cellSize) + (cellSize/2), spriteGridPos.y + (numRows*cellSize)  },
                       1, BORDER_COLOR);
            // horizontal
            DrawLineEx((Vector2){ spriteGridPos.x, spriteGridPos.y  + (px*cellSize) + cellSize/2 }, 
                       (Vector2){ spriteGridPos.x + (numCols*cellSize) , spriteGridPos.y  + (px*cellSize) + (cellSize/2) },
                       1, BORDER_COLOR);

            DrawRectangleLinesEx(drawingArea,1,BORDER_COLOR);

            }
        // grid over sprite (if want grid below sprite, just  move line  up)
        else  drawGridLines(BORDER_COLOR);
        

        EndMode2D();
    EndScissorMode();

        //----------------------------------------------------------------------
        // draw rows and columns headers.
        //----------------------------------------------------------------------
        for (int row = 0; row < numRows; row++)
        {
            DrawTextEx(font, TextFormat("%02d",row),(Vector2){spriteGridPos.x - 18,(spriteGridPos.y + 3) + (row * cellSize)}, font.baseSize, -1, FG_COLOR); //sinistra
            DrawTextEx(font, TextFormat("%02d",row),(Vector2){spriteGridPos.x + numCols*cellSize +8,spriteGridPos.y + 4 + (row * cellSize)}, font.baseSize, -1, FG_COLOR);//destra
        }
        for (int col = 0; col < numCols; col++)
        {
            DrawTextEx(font,TextFormat("%02d",col),(Vector2){spriteGridPos.x + 2 + (col * cellSize),spriteGridPos.y -20},font.baseSize,-1,FG_COLOR);//sopra
            DrawTextEx(font,TextFormat("%02d",col),(Vector2){spriteGridPos.x + 4 + (col * cellSize),spriteGridPos.y + numRows*cellSize+8} ,font.baseSize,-1,FG_COLOR);//sotto
        }

        //----------------------------------------------------------------------
        // draw sprite miniature and colors info
        //----------------------------------------------------------------------
        drawThumbnail();


        //----------------------------------------------------------------------
        // Draw color selection bar
        //-----------------------------------------------------------------------
        for (int i = 0; i < MAX_COLORS_COUNT; i++) {
            colors[i] = GetDynamicPastel(baseColors[i], pastelSlider);
            DrawRectangleRec(colorsRecs[i], colors[i]);
            DrawRectangleLinesEx(colorsRecs[i],1,BORDER_COLOR);
            // passando sopra il colore evidenzialo..
            if (i == colorMouseHover) {
                //DrawRectangleRec(colorsRecs[colorMouseHover], Fade(WHITE, 0.3f));
                DrawRectangleLinesEx(colorsRecs[i],2,BLACK);
            }
        }
        
        // slider contrasto
        DrawText(TextFormat("DynaBright: %.f",pastelSlider),40,550,10,FG_COLOR);
        GuiSlider((Rectangle){ 40,564, 103, 16 }, "", "", &pastelSlider, 0.0f, 192.0f);


        //display cursor position and selectedIndex color info 
        // draw current color under mouse cursor
        DrawRectangle(colorsInfoPos.x, colorsInfoPos.y, 23,23,currentColor);
        DrawRectangleLines(colorsInfoPos.x, colorsInfoPos.y, 23,23,GRID_COLOR);
        DrawTextEx(font,TextFormat("rgba(%i, %i, %i, %i)",currentColor.r, currentColor.g,currentColor.b,currentColor.a),(Vector2){colorsInfoPos.x,colorsInfoPos.y + 28},font.baseSize,-1,FG_COLOR);
        
        DrawLine(colorsInfoPos.x,colorsInfoPos.y+52,colorsInfoPos.x+128,colorsInfoPos.y+52,GRID_COLOR);

        // Draw x,y for current cell, zoom value
        DrawTextEx(boldfont, TextFormat("x: %02i, y: %02i",px,py),(Vector2){colorsInfoPos.x+32, colorsInfoPos.y+4},boldfont.baseSize,-1,BLACK);
        DrawTextEx(boldfont, TextFormat("x1: %02i, y1: %02i",px1,py1),(Vector2){colorsInfoPos.x, colorsInfoPos.y+64},boldfont.baseSize,-1,BLACK);
        DrawTextEx(boldfont, TextFormat("Scale: %ix",miniatureScale),(Vector2){miniaturePos.x+12, miniaturePos.y+102},boldfont.baseSize,-1,BLACK);

        // draw selected color
        DrawRectangle(colorsInfoPos.x+104, colorsInfoPos.y + 60, 23, 23, selectedColor);
        DrawRectangleLines(colorsInfoPos.x+104, colorsInfoPos.y + 60, 23, 23, GRID_COLOR);
        DrawTextEx(font,TextFormat("rgba(%i, %i, %i, %i)",selectedColor.r, selectedColor.g,selectedColor.b,selectedColor.a),(Vector2){colorsInfoPos.x,colorsInfoPos.y + 88},font.baseSize,-1,FG_COLOR);

        // -----------------------------------------------------------------
        //  print file list / library 
        //------------------------------------------------------------------
        DrawText("Library",libraryPos.x, 14, 20, FG_COLOR);
        DrawRectangleRec(libraryPos,GRID_BG_COLOR);
        DrawRectangleLinesEx(libraryPos,1,BORDER_COLOR);

            scrollOffset = selectedIndex - centerRow;
            if (scrollOffset < 0) scrollOffset = 0;
            int maxOffset = fileCount - visibleRows;
            if (maxOffset < 0) maxOffset = 0;
            if (scrollOffset > maxOffset) scrollOffset = maxOffset;

                for (int i = 0; i < visibleRows; ++i) {

                    if (i!=0) DrawLine(libraryPos.x+1, libraryPos.y + (1+i*rowHeight), libraryPos.x + libraryPos.width-1, libraryPos.y +(1+i*rowHeight),GRID_COLOR);

                    int fileIndex = scrollOffset + i;
                    if (fileIndex > fileCount) break;

                    if (fileIndex == selectedIndex) DrawRectangle(libraryPos.x+1,libraryPos.y +(i*rowHeight) +1 ,libraryPos.width-2,rowHeight-1, isOverLib?ON_COLOR:GRID_COLOR);
                        DrawText(TextFormat("%04i",fileIndex + 1),libraryPos.x + 6, libraryPos.y +(i*rowHeight)+6,10,FG_COLOR);
                        // print truncate too long filenames (max. 88 chars)
                        const char *fileNameTrunc = TextSubtext(GetFileName(files[fileIndex]),0,20); 
                        DrawText(TextFormat("%s",fileNameTrunc), libraryPos.x+38, libraryPos.y +(i*rowHeight)+6,10,BLACK);
                    
                    }   
            //vertical divider between index and filename 
            DrawLine(libraryPos.x+34,libraryPos.y,libraryPos.x+34,libraryPos.y + libraryPos.height,GRID_COLOR);
        
        //------------------------------------------------------------------            
        GuiLabel((Rectangle){ keyinfoPos.x+2, keyinfoPos.y -20, 160, 20 }, "Work file: ['S'] to save.");
        if (GuiTextBox((Rectangle){ keyinfoPos.x, keyinfoPos.y, 160, 28 }, fName, 256, fnameEditMode)) fnameEditMode = !fnameEditMode;


        // Draw toolbar and messages
        int btnWidth = 32;
        int btnHeight = 32;
        GuiSetStyle(BUTTON, BORDER_WIDTH, 1);
        // define button style
        GuiSetStyle(BUTTON, TEXT_COLOR_NORMAL,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_NORMAL,0xECF1F1FF);
        GuiSetStyle(BUTTON, TEXT_COLOR_FOCUSED,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_FOCUSED,0xECF1F1FF);
        GuiSetStyle(BUTTON, TEXT_COLOR_PRESSED,0x000000FF);
        GuiSetStyle(BUTTON, BASE_COLOR_PRESSED,0x0CA1A6FF);
        // define TOGGLE style
        GuiSetStyle(TOGGLE, TEXT_COLOR_NORMAL,0x000000FF);
        GuiSetStyle(TOGGLE, BASE_COLOR_NORMAL,0xECF1F1FF);
        GuiSetStyle(TOGGLE, TEXT_COLOR_PRESSED,0x000000FF);
        GuiSetStyle(TOGGLE, BASE_COLOR_PRESSED,0x0CA1A6FF);

        //btnGrid = GuiButton((Rectangle){toolbarPos.x, toolbarPos.y, btnWidth, btnHeight }, "#50#");
        GuiToggle((Rectangle){ toolbarPos.x, toolbarPos.y , 32, 32 }, "#97#", &showGrid);
        btnNew = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y, btnWidth, btnHeight }, "#218#");
        btnClear = GuiButton((Rectangle){toolbarPos.x+72, toolbarPos.y, btnWidth, btnHeight }, "#109#");

        GuiToggle((Rectangle){ toolbarPos.x, toolbarPos.y + 36 , 32, 32 }, "#23#", &isDrawing);
            if (isDrawing) {
                isColorRepl=false;
                isFloodFill=false;
                isDrawHmirr=false;
                isDrawVmirr=false;
            }
        GuiToggle((Rectangle){ toolbarPos.x, toolbarPos.y + 72 , 32, 32 }, "#29#", &isFloodFill);
            if (isFloodFill) {
                isDrawing=false;
                isColorRepl=false;
                isDrawHmirr=false;
                isDrawVmirr=false;
            }
        GuiToggle((Rectangle){ toolbarPos.x+72, toolbarPos.y + 72 , 32, 32 }, "#25#", &isColorRepl);
            if (isColorRepl) {
                isDrawing=false;
                isFloodFill=false;
                isDrawHmirr=false;
                isDrawVmirr=false;
            }


        GuiToggle((Rectangle){ toolbarPos.x + 36, toolbarPos.y + 36 , 32, 32 }, "#85#", &isDrawHmirr);
            if (isDrawHmirr) {
                isDrawing=false;
                isDrawVmirr=false;
                isFloodFill=false;
                isColorRepl=false;
            }

        btnShtUp = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 72, btnWidth, btnHeight }, "#121#");

        GuiToggle((Rectangle){ toolbarPos.x+72, toolbarPos.y + 36 , 32, 32 }, "#83#", &isDrawVmirr);
            if (isDrawVmirr) {
                isDrawing=false;
                isDrawHmirr=false;
                isFloodFill=false;
                isColorRepl=false;
            }

        btnShtSx = GuiButton((Rectangle){toolbarPos.x, toolbarPos.y + 108, btnWidth, btnHeight }, "#118#");
        btnRotate = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 108, btnWidth, btnHeight }, "#77#");
        btnShtDx = GuiButton((Rectangle){toolbarPos.x +72, toolbarPos.y + 108, btnWidth, btnHeight }, "#119#");

        btnHmirr = GuiButton((Rectangle){toolbarPos.x , toolbarPos.y + 144, btnWidth, btnHeight }, "#41#");
        btnShtDn = GuiButton((Rectangle){toolbarPos.x +36, toolbarPos.y + 144, btnWidth, btnHeight }, "#120#");
        btnVmirr = GuiButton((Rectangle){toolbarPos.x +72, toolbarPos.y + 144, btnWidth, btnHeight }, "#40#");

        GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+40, 140, 20 }, "Mousewheel to zoom in/out.");
        GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+60, 140, 20 }, "WinKey + mouse left to move.");
        GuiLabel((Rectangle){ keyinfoPos.x, keyinfoPos.y+80, 140, 20 }, "['Q'] to quit program.");

        btnQuit = GuiButton((Rectangle){screenWidth-34,14,20,20}, "#128#");

        EndDrawing();
    }
    UnloadRenderTexture(target);
    UnloadFont(font);
    UnloadFont(boldfont);
    CloseWindow();
    return 0;
}
