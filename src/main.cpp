#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>

static float Clamp(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

struct PlayerControls { int up, down, hit; };
struct PlayerState { float angleDeg = 35.0f; };

struct Ball {
    Vector2 pos{};
    Vector2 vel{};
    bool inPlay = false;
};

static void ResetServe(Ball& b, bool serveFromLeft, float groundY) {
    b.inPlay = false;
    b.pos = { serveFromLeft ? 170.0f : 810.0f, groundY - 40.0f };
    b.vel = { 0.0f, 0.0f };
}

static void LaunchHit(Ball& b, bool hitterIsLeft, float angleDeg, float speed) {
    constexpr float pi = 3.14159265358979323846f;
    float a = angleDeg * (pi / 180.0f);
    float vx = std::cos(a) * speed;
    float vy = -std::sin(a) * speed;
    b.vel = { hitterIsLeft ? vx : -vx, vy };
    b.inPlay = true;
}

int main() {
    const int W = 980;
    const int H = 560;

    InitWindow(W, H, "Tennis for Two");
    SetTargetFPS(120);

    // Oscilloscope-ish look
    const Color Trace = { 80, 255, 120, 255 };
    const Color Fade  = { 0, 0, 0, 40 }; // lower alpha => longer trails

    const float groundY = 420.0f;
    const float netX = W * 0.5f;
    const float netH = 80.0f;

    // Physics
    const float g = 900.0f;         // px/s^2
    const float bounce = 0.75f;     // energy kept
    const float friction = 0.995f;  // mild x damping

    // Fixed timestep
    const float dt = 1.0f / 240.0f;
    float acc = 0.0f;

    PlayerControls L{ KEY_E, KEY_D, KEY_F }; // Left: E/D angle, F hit
    PlayerControls R{ KEY_I, KEY_K, KEY_J }; // Right: I/K angle, J hit
    PlayerState pL{}, pR{};

    Ball ball{};
    bool serveFromLeft = true;
    ResetServe(ball, serveFromLeft, groundY);

    int scoreL = 0, scoreR = 0;

    BeginDrawing(); ClearBackground(BLACK); EndDrawing();

    while (!WindowShouldClose()) {
        float frameDt = GetFrameTime();

        auto updateAngle = [&](PlayerState& p, const PlayerControls& k) {
            float delta = 0.0f;
            if (IsKeyDown(k.up))   delta += 90.0f * frameDt;
            if (IsKeyDown(k.down)) delta -= 90.0f * frameDt;
            p.angleDeg = Clamp(p.angleDeg + delta, 10.0f, 80.0f);
        };

        updateAngle(pL, L);
        updateAngle(pR, R);

        // Hit rules:
        // - If not in play: only server can launch
        // - If in play: allow volley hits when ball is on your side & in the air
        if (!ball.inPlay) {
            if (serveFromLeft && IsKeyPressed(L.hit))  LaunchHit(ball, true,  pL.angleDeg, 520.0f);
            if (!serveFromLeft && IsKeyPressed(R.hit)) LaunchHit(ball, false, pR.angleDeg, 520.0f);
        } else {
            if (IsKeyPressed(L.hit) && ball.pos.x < netX - 20.0f && ball.pos.y < groundY - 10.0f)
                LaunchHit(ball, true, pL.angleDeg, 520.0f);

            if (IsKeyPressed(R.hit) && ball.pos.x > netX + 20.0f && ball.pos.y < groundY - 10.0f)
                LaunchHit(ball, false, pR.angleDeg, 520.0f);
        }

        // Simulate
        acc += frameDt;
        acc = std::min(acc, 0.25f);

        while (acc >= dt) {
            if (ball.inPlay) {
                ball.vel.y += g * dt;
                ball.pos.x += ball.vel.x * dt;
                ball.pos.y += ball.vel.y * dt;
                ball.vel.x *= friction;

                // Net collision (simple)
                float netTop = groundY - netH;
                if (std::fabs(ball.pos.x - netX) < 6.0f && ball.pos.y > netTop) {
                    ball.vel.x = -ball.vel.x * 0.6f;
                    ball.pos.x += (ball.pos.x < netX ? -2.0f : 2.0f);
                }

                // Ground bounce + dead-ball scoring
                if (ball.pos.y >= groundY) {
                    ball.pos.y = groundY;
                    if (ball.vel.y > 0) ball.vel.y = -ball.vel.y * bounce;

                    // If it's barely bouncing, end the rally and score
                    if (std::fabs(ball.vel.y) < 120.0f) {
                        ball.inPlay = false;

                        if (ball.pos.x < netX) scoreR++;
                        else scoreL++;

                        // Winner serves next
                        serveFromLeft = (ball.pos.x > netX);
                        ResetServe(ball, serveFromLeft, groundY);
                    }
                }

                // Out of bounds
                if (ball.pos.x < 0 || ball.pos.x > W) {
                    ball.inPlay = false;

                    if (ball.pos.x < 0) scoreR++;
                    else scoreL++;

                    serveFromLeft = (ball.pos.x > W);
                    ResetServe(ball, serveFromLeft, groundY);
                }
            }
            acc -= dt;
        }

        // Render
        BeginDrawing();
        DrawRectangle(0, 0, W, H, Fade);

        DrawLine(40, (int)groundY, W - 40, (int)groundY, Trace);
        DrawLine((int)netX, (int)(groundY - netH), (int)netX, (int)groundY, Trace);

        Vector2 drawPos = ball.pos;
        if (!ball.inPlay) drawPos = { serveFromLeft ? 170.0f : 810.0f, groundY - 40.0f };
        DrawCircleV(drawPos, 4.0f, Trace);

        auto drawAngle = [&](float x, float y, float angleDeg, bool left) {
            constexpr float pi = 3.14159265358979323846f;
            float a = angleDeg * (pi / 180.0f);
            Vector2 dir = { std::cos(a), -std::sin(a) };
            if (!left) dir.x = -dir.x;
            DrawLineV({ x, y }, { x + dir.x * 30.0f, y + dir.y * 30.0f }, Trace);
        };

        drawAngle(120.0f, groundY - 20.0f, pL.angleDeg, true);
        drawAngle(W - 120.0f, groundY - 20.0f, pR.angleDeg, false);

        DrawText(TextFormat("L: %d   R: %d", scoreL, scoreR), 20, 20, 20, Trace);
        DrawText("Left:  E/D angle, F hit    Right: I/K angle, J hit", 20, 45, 18, Trace);
        DrawText(serveFromLeft ? "Serve: LEFT (press F)" : "Serve: RIGHT (press J)", 20, 70, 18, Trace);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
