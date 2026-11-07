#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define RIGHE 4
#define COLONNE 6
#define TOT_COLORI (RIGHE * COLONNE)

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 550; // Alzata leggermente per fare spazio alla GUI
    InitWindow(screenWidth, screenHeight, "Palette con Slider Luminosità - C99");

    // 1. Genera i 24 colori casuali iniziali (RAM)
    Color paletteOriginale[TOT_COLORI];
    for (int i = 0; i < TOT_COLORI; i++) {
        paletteOriginale[i] = (Color){ 
            (unsigned char)GetRandomValue(0, 255), 
            (unsigned char)GetRandomValue(0, 255), 
            (unsigned char)GetRandomValue(0, 255), 
            255 
        };
    }

    // Variabili per la griglia
    const float marginX = 50.0f;
    const float marginY = 80.0f;
    const float rectWidth = 100.0f;
    const float rectHeight = 60.0f;
    const float spacing = 15.0f;

    // Stato dell'applicazione
    float valoreLuminosita = 0.0f; // Slider: da -1.0 (scuro) a 1.0 (chiaro). 0.0 è originale.
    Color coloreLetto = BLANK;
    int indiceSelezionato = -1;
    bool coloreScelto = false;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();

        // Se l'utente clicca, controlliamo la griglia
        // Nota: Impediamo il clic sui rettangoli se il mouse si trova sopra la zona dello slider di Raygui
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mousePos.y < 460) {
            for (int r = 0; r < RIGHE; r++) {
                for (int c = 0; c < COLONNE; c++) {
                    float x = marginX + c * (rectWidth + spacing);
                    float y = marginY + r * (rectHeight + spacing);
                    Rectangle rectCorrente = { x, y, rectWidth, rectHeight };

                    if (CheckCollisionPointRec(mousePos, rectCorrente)) {
                        indiceSelezionato = r * COLONNE + c;
                        coloreScelto = true;
                        break;
                    }
                }
            }
        }

        // Se un colore è selezionato, calcoliamo il suo colore modificato in tempo reale.
        // In questo modo, se muovi lo slider MENTRE un colore è selezionato, i valori RGB si aggiornano!
        if (coloreScelto && indiceSelezionato != -1) {
            coloreLetto = ColorBrightness(paletteOriginale[indiceSelezionato], valoreLuminosita);
        }

        // --- DISEGNO ---
        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText("Modifica la luminosità e seleziona un colore", 50, 25, 22, DARKGRAY);

            // 2. Disegna i rettangoli applicando la luminosità al volo (senza modificare l'array)
            for (int r = 0; r < RIGHE; r++) {
                for (int c = 0; c < COLONNE; c++) {
                    float x = marginX + c * (rectWidth + spacing);
                    float y = marginY + r * (rectHeight + spacing);
                    int indice = r * COLONNE + c;

                    // Calcoliamo il colore modificato solo per il disegno a schermo di questo frame
                    Color coloreVisualizzato = ColorBrightness(paletteOriginale[indice], valoreLuminosita);

                    DrawRectangleRec((Rectangle){ x, y, rectWidth, rectHeight }, coloreVisualizzato);
                    
                    if (indice == indiceSelezionato) {
                        DrawRectangleLinesEx((Rectangle){ x - 3, y - 3, rectWidth + 6, rectHeight + 6 }, 3, BLACK);
                    }
                }
            }

            // 3. Interfaccia RAYGUI (In fondo allo schermo)
            // Disegniamo lo slider per regolare la luminosità
            GuiSlider((Rectangle){ 200, 480, 400, 20 }, "Scurisci", "Schiarisci", &valoreLuminosita, -1.0f, 1.0f);

            // 4. Mostra il colore finale calcolato
            if (coloreScelto) {
                DrawText("Colore letto:", 50, 420, 18, DARKGRAY);
                DrawRectangle(170, 410, 30, 30, coloreLetto);
                DrawRectangleLines(170, 410, 30, 30, BLACK);

                // Stampa i valori RGB effettivi (modificati dallo slider)
                DrawText(TextFormat("RGB Effettivo: (%d, %d, %d)", coloreLetto.r, coloreLetto.g, coloreLetto.b), 220, 415, 18, BLACK);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
