/*******************************************************************************************
* Keybindings for rmPlayer
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"        // Required for: Lerp()
#include <string.h>

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST); 
    InitWindow(screenWidth, screenHeight, "rmPlayer keybindings");
    SetExitKey(KEY_Q);       // Disable KEY_ESCAPE to close window, X-button still works
    // Setting up the camera
    Camera2D cam = {
        .offset = {0, 0},
        .target = {0, 0},
        .rotation = 0,
        .zoom = 1
    };

    // Loading text file from resources/text_file.txt
    const char *fileName = "keybindings.txt";
    char *text = LoadFileText(fileName);

    // Loading all the text lines
    int lineCount = 0;
    char **lines = LoadTextLines(text, &lineCount);

    // Stylistic choises
    int fontSize = 20;
    int textTop = 25 + fontSize; // Top of the screen from where the text is rendered
    int wrapWidth = screenWidth - 20;

    // Wrap the lines as needed
    for (int i = 0; i < lineCount; i++)
    {
        int j = 0;
        int lastSpace = 0;          // Keeping track of last valid space to insert '\n'
        int lastWrapStart = 0;      // Keeping track of the start of this wrapped line.

        while (j <= strlen(lines[i]))
        {
            if (lines[i][j] == ' ' || lines[i][j] == '\0')
            {
                char before = lines[i][j];
                // Making a C Style string by adding a '\0' at the required location so that we can use the MeasureText function
                lines[i][j] = '\0';

                // Checking if the text has crossed the wrapWidth, then going back and inserting a newline
                if (MeasureText(lines[i] + lastWrapStart, fontSize) > wrapWidth)
                {
                    lines[i][lastSpace] = '\n';

                    // Since we added a newline the place of wrap changed so we update our lastWrapStart
                    lastWrapStart = lastSpace + 1;
                }

                if(before != '\0') lines[i][j] = ' ';  // Resetting the space back
                lastSpace = j; // Since we encountered a new space we update our last encountered space location
            }

            j++;
        }
    }

    // Calculating the total height so that we can show a scrollbar
    int textHeight = 0;

    for (int i = 0; i < lineCount; i++)
    {
        Vector2 size = MeasureTextEx(GetFontDefault(), lines[i], (float)fontSize, 2);
        textHeight += (int)size.y + 10;
    }

    // fai riapparire finestra dopo caricamento iniziale
    ClearWindowState(FLAG_WINDOW_HIDDEN);

    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        float scroll = GetMouseWheelMove();
        cam.target.y -= scroll*fontSize*1.5f;   // Choosing an arbitrary speed for scroll

        if (cam.target.y < 0) cam.target.y = 0;  // Snapping to 0 if we go too far back

        // Ensuring that the camera does not scroll past all text
        if (cam.target.y > textHeight - screenHeight + textTop)
            cam.target.y = (float)textHeight - screenHeight + textTop;
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode2D(cam);
                // Going through all the read lines
                for (int i = 0, t = textTop; i < lineCount; i++)
                {
                    // Each time we go through and calculate the height of the text to move the cursor appropriately
                    Vector2 size;
                    if(strcmp(lines[i], "")){
                        // Fix for empty line in the text file
                        size = MeasureTextEx( GetFontDefault(), lines[i], (float)fontSize, 2);
                    }else{
                        size = MeasureTextEx( GetFontDefault(), " ", (float)fontSize, 2);
                    }

                    DrawText(lines[i], 10, t, fontSize, BLACK);

                    // Inserting extra space for real newlines,
                    // wrapped lines are rendered closer together
                    t += (int)size.y + 10;
                }
            EndMode2D();

            // Header displaying which file is being read currently
            DrawRectangle(0, 0, screenWidth, textTop - 10, SKYBLUE);
            DrawText(TextFormat("File: %s", fileName), 10, 10, fontSize, BLACK);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTextLines(lines, lineCount);
    UnloadFileText(text);

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
