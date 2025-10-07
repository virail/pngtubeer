#include "raylib.h"

int main() {
    const int screenWidth = 1920 * 0.25;
    const int screenHeight = 1080 * 0.25;
    InitWindow(screenWidth, screenHeight, "");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText(TextFormat("It works! %dx%d", screenWidth, screenHeight), 20, 20, 20, BLACK);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
