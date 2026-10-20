#include "raylib.h"
#include <math.h>
#include <time.h>

// Struttura per definire la palette di colori del tema
typedef struct {
    Color screenBg;
    Color widgetBg;
    Color widgetBorder;
    Color hourMarkers;
    Color minuteMarkers;
    Color hourHand;
    Color minHand;
    Color secHand;
    Color textColor;
} ClockTheme;

// Definizione statica dei due temi disponibili
const ClockTheme DarkTheme = {
    .screenBg = (Color){ 29, 31, 42, 255},
    .widgetBg = (Color){ 29, 31, 42, 255 },
    .widgetBorder = (Color){ 48, 52, 70, 255 },
    .hourMarkers = WHITE,
    .minuteMarkers = (Color){ 100, 100, 100, 255 },
    .hourHand = WHITE,
    .minHand = LIGHTGRAY,
    .secHand = (Color){ 255, 59, 48, 255 }, // Arancione/Rosso Apple vivido
    .textColor = (Color){ 70, 70, 70, 255 }
};

const ClockTheme LightTheme = {
    .screenBg = (Color){ 240, 240, 243, 255 },
    .widgetBg = WHITE,
    .widgetBorder = (Color){ 210, 210, 215, 255 },
    .hourMarkers = (Color){ 20, 20, 20, 255 },
    .minuteMarkers = (Color){ 160, 160, 165, 255 },
    .hourHand = BLACK,
    .minHand = DARKGRAY,
    .secHand = (Color){ 255, 59, 48, 255 }, // Arancione/Rosso Apple vivido
    .textColor = (Color){ 140, 140, 145, 255 }
};

typedef struct {
    Vector2 position;
    Vector2 normal; 
} PathPoint;

// posizione tick lungo il perimtro di un rettangolo, quadrato
PathPoint GetRoundedRectPathPoint(Rectangle rect, float radius, float progress) 
{
    PathPoint result = { 0 };
    
    float w_straight = rect.width - 2.0f * radius;
    float h_straight = rect.height - 2.0f * radius;
    
    float len_top = w_straight;
    float len_corner1 = (PI / 2.0f) * radius;
    float len_right = h_straight;
    float len_corner2 = (PI / 2.0f) * radius;
    float len_bottom = w_straight;
    float len_corner3 = (PI / 2.0f) * radius;
    float len_left = h_straight;
    float len_corner4 = (PI / 2.0f) * radius;
    
    float total_perimeter = len_top + len_corner1 + len_right + len_corner2 + 
                            len_bottom + len_corner3 + len_left + len_corner4;
    
    float target_dist = progress * total_perimeter;
    target_dist = fmodf(target_dist + (len_top / 2.0f), total_perimeter);
    
    Vector2 c_top_right = { rect.x + rect.width - radius, rect.y + radius };
    Vector2 c_bot_right = { rect.x + rect.width - radius, rect.y + rect.height - radius };
    Vector2 c_bot_left  = { rect.x + radius, rect.y + rect.height - radius };
    Vector2 c_top_left  = { rect.x + radius, rect.y + radius };

    if (target_dist < len_top) {
        result.position.x = rect.x + radius + target_dist;
        result.position.y = rect.y;
        result.normal = (Vector2){ 0.0f, 1.0f };
        return result;
    }
    target_dist -= len_top;
    
    if (target_dist < len_corner1) {
        float angle = -PI/2.0f + (target_dist / len_corner1) * (PI/2.0f);
        result.position.x = c_top_right.x + cosf(angle) * radius;
        result.position.y = c_top_right.y + sinf(angle) * radius;
        result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
        return result;
    }
    target_dist -= len_corner1;
    
    if (target_dist < len_right) {
        result.position.x = rect.x + rect.width;
        result.position.y = rect.y + radius + target_dist;
        result.normal = (Vector2){ -1.0f, 0.0f };
        return result;
    }
    target_dist -= len_right;
    
    if (target_dist < len_corner2) {
        float angle = 0.0f + (target_dist / len_corner2) * (PI/2.0f);
        result.position.x = c_bot_right.x + cosf(angle) * radius;
        result.position.y = c_bot_right.y + sinf(angle) * radius;
        result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
        return result;
    }
    target_dist -= len_corner2;
    
    if (target_dist < len_bottom) {
        result.position.x = rect.x + rect.width - radius - target_dist;
        result.position.y = rect.y + rect.height;
        result.normal = (Vector2){ 0.0f, -1.0f };
        return result;
    }
    target_dist -= len_bottom;
    
    if (target_dist < len_corner3) {
        float angle = PI/2.0f + (target_dist / len_corner3) * (PI/2.0f);
        result.position.x = c_bot_left.x + cosf(angle) * radius;
        result.position.y = c_bot_left.y + sinf(angle) * radius;
        result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
        return result;
    }
    target_dist -= len_corner3;
    
    if (target_dist < len_left) {
        result.position.x = rect.x;
        result.position.y = rect.y + rect.height - radius - target_dist;
        result.normal = (Vector2){ 1.0f, 0.0f };
        return result;
    }
    target_dist -= len_left;
    
    float angle = PI + (target_dist / len_corner4) * (PI/2.0f);
    result.position.x = c_top_left.x + cosf(angle) * radius;
    result.position.y = c_top_left.y + sinf(angle) * radius;
    result.normal = (Vector2){ -cosf(angle), -sinf(angle) };
    
    return result;
}

// Funzione di utilità per interpolare fluidamente tra due colori (Transizione del tema)
Color BlendColors(Color from, Color to, float alpha) {
    return (Color){
        .r = (unsigned char)(from.r + (to.r - from.r) * alpha),
        .g = (unsigned char)(from.g + (to.g - from.g) * alpha),
        .b = (unsigned char)(from.b + (to.b - from.b) * alpha),
        .a = 255
    };
}

int main(void) 
{
    const int screenWidth = 196;
    const int screenHeight = 196;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT| FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "raylib - Orologio macOS Dinamico");

    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    
    // dimensioni orologio
    Rectangle clockRect = { 8,8,screenWidth-16,screenHeight-16 };

    //Rettangolo rounde che pareggia tutti i marker internamente...
    Rectangle clockRectInner = { 22,22,screenWidth-44,screenHeight-44 };
    
    // roundess globale orologio
    // 0.0f quadrato / max dipende da dimensioni..fare delle prove!
    float cornerRadius = 24.0f; 

    // Stato del tema
    bool isDarkMode = true;
    float themeTransition = 1.0f; // 1.0 significa che la transizione è terminata
    ClockTheme currentTheme = DarkTheme;

    SetTargetFPS(60);

    // fai riapparire finestra dopo averla nascosta durante il carimento iniziale
    // SetTargetFPS(60);
    SetWindowSize(screenWidth,screenHeight);
    SetWindowPosition(GetMonitorWidth(0) / 2 - screenWidth/2, GetMonitorHeight(0) / 2 - screenHeight/2);  
    //SetWindowPosition(8,40);  
    ClearWindowState(FLAG_WINDOW_HIDDEN);
    while (!WindowShouldClose()) 
    {
        // Rileva pressione tasto T per invertire il tema
        if (IsKeyPressed(KEY_T)) {
            isDarkMode = !isDarkMode;
            themeTransition = 0.0f; // Fa ripartire l'effetto transizione
        }

        // Gestione transizione fluida tra i temi
        if (themeTransition < 1.0f) {
            themeTransition += GetFrameTime() * 4.0f; // Velocità della transizione (0.25s)
            if (themeTransition > 1.0f) themeTransition = 1.0f;
            
            ClockTheme source = isDarkMode ? LightTheme : DarkTheme;
            ClockTheme target = isDarkMode ? DarkTheme : LightTheme;
            
            // Interpola ogni singolo colore del widget
            currentTheme.screenBg     = BlendColors(source.screenBg, target.screenBg, themeTransition);
            currentTheme.widgetBg     = BlendColors(source.widgetBg, target.widgetBg, themeTransition);
            currentTheme.widgetBorder = BlendColors(source.widgetBorder, target.widgetBorder, themeTransition);
            currentTheme.hourMarkers   = BlendColors(source.hourMarkers, target.hourMarkers, themeTransition);
            currentTheme.minuteMarkers = BlendColors(source.minuteMarkers, target.minuteMarkers, themeTransition);
            currentTheme.hourHand     = BlendColors(source.hourHand, target.hourHand, themeTransition);
            currentTheme.minHand      = BlendColors(source.minHand, target.minHand, themeTransition);
            currentTheme.secHand      = BlendColors(source.secHand, target.secHand, themeTransition);
            currentTheme.textColor    = BlendColors(source.textColor, target.textColor, themeTransition);
        }

        // Calcolo Tempo ad alta risoluzione
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm *timeInfo = localtime(&ts.tv_sec);
        
        int hours = timeInfo->tm_hour;
        int minutes = timeInfo->tm_min;
        float preciseSeconds = (float)timeInfo->tm_sec + ((float)ts.tv_nsec / 1000000000.0f);

        float angleSec = (preciseSeconds / 60.0f) * 2.0f * PI - PI / 2.0f;
        float angleMin = (((float)minutes + preciseSeconds / 60.0f) / 60.0f) * 2.0f * PI - PI / 2.0f;
        float angleHour = (((float)(hours % 12) + ((float)minutes + preciseSeconds / 60.0f) / 60.0f) / 12.0f) * 2.0f * PI - PI / 2.0f;

        Vector2 center = { clockRect.x + clockRect.width / 2.0f, clockRect.y + clockRect.height / 2.0f };

        BeginDrawing();
        
            ClearBackground(currentTheme.widgetBg); 

            // 1. Sfondo del widget
            float roundness = (cornerRadius * 2.0f) / clockRect.height;


            // 2. Disegno dei 60 marker (Radiali)
            for (int i = 0; i < 60; i++) 
            {
                float progress = (float)i / 60.0f;
                PathPoint pt = GetRoundedRectPathPoint(clockRect, cornerRadius, progress);
                
                float markerLen = 40.0f;
                float thickness = 2.0f;
                Color markerColor = currentTheme.minuteMarkers;
                
                Vector2 drawNormal;
                float dirX = center.x - pt.position.x;
                float dirY = center.y - pt.position.y;
                float len = sqrtf(dirX * dirX + dirY * dirY);
                
                if (len > 0.0f) {
                    drawNormal.x = dirX / len;
                    drawNormal.y = dirY / len;
                } else {
                    drawNormal = pt.normal;
                }

                if (i % 5 == 0) // marker dell ore : ogni 5 marker minuti fallo piu ciccio
                {
                    markerLen = 40.00f;
                    thickness = 4.0f;
                    markerColor = currentTheme.hourMarkers; 
                }
                // distanza dal bordo dei marker
                float edgeOffset = 2.0f;//cambiando offset cambia anche problema marker con andamento a onda
                Vector2 startPoint = {
                    pt.position.x + drawNormal.x * edgeOffset,
                    pt.position.y + drawNormal.y * edgeOffset
                };

                Vector2 endPoint = {
                startPoint.x + drawNormal.x * markerLen,
                startPoint.y + drawNormal.y * markerLen
                };
                DrawLineEx(startPoint, endPoint, thickness, markerColor);
                }
                // fake rectagle to align al hour markers
                DrawRectangleRounded(clockRectInner, roundness, 24, currentTheme.widgetBg);
                //DrawRectangleRoundedLines(clockRectInner, roundness, 24, currentTheme.widgetBorder);

                // 3. Lancette dell'orologio
                // Ore
                Vector2 hourEnd = { center.x + cosf(angleHour) * 54.0f, center.y + sinf(angleHour) * 54.0f };
                DrawLineEx(center, hourEnd, 6.5f, currentTheme.hourHand);
                // Minuti
                Vector2 minEnd = { center.x + cosf(angleMin) * 72.0f, center.y + sinf(angleMin) * 72.0f };
                DrawLineEx(center, minEnd, 4.5f, currentTheme.minHand);
                // Secondi
                Vector2 secEnd = { center.x + cosf(angleSec) * 82.0f, center.y + sinf(angleSec) * 82.0f };
                Vector2 secBack = { center.x - cosf(angleSec) * 20.0f, center.y - sinf(angleSec) * 20.0f };
                DrawLineEx(secBack, secEnd, 1.5f, currentTheme.secHand);
                // Perno centrale
                DrawCircleV(center, 7.0f, currentTheme.secHand);
                DrawCircleV(center, 4.0f, currentTheme.widgetBg);
                EndDrawing();
                }
CloseWindow();
return 0;
}