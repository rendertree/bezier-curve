// Copyright (c) 2024 Wildan R Wijanarko
//
// This software is provided ‘as-is’, without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.

// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:

// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.

// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.

// 3. This notice may not be removed or altered from any source
// distribution.

#include "raylib.h"
#include <iostream>
#include <string>
#include <cmath>

#define RAYGUI_IMPLEMENTATION
#include "extras/raygui.h"

const int worldWidth  = 12220;
const int worldHeight = 12220;
const int gridSize    = 80;

using namespace std;
using rec  = Rectangle;
using str  = string;
using vec2 = Vector2;
using clr  = Color;

#define print(n, flag) if (flag) cout << (n) << endl; else cout << (n)

// Addition operator for vec2
inline vec2 operator +(vec2 a, vec2 b)
{
    return { a.x + b.x, a.y + b.y };
}

// Subtraction operator for vec2
inline vec2 operator -(vec2 a, vec2 b)
{
    return { a.x - b.x, a.y - b.y };
}

// Scale a vec2 by a given factor
inline vec2 vec2_scale(vec2 v, float scale)
{
    return { v.x * scale, v.y * scale };
}

// Convert a vec2 to a string
inline str vec2_to_str(vec2 v) 
{ 
    return "x: " + to_string((int)v.x) + " y: " + to_string((int)v.y); 
}

// Perform linear interpolation between two vec2 points
inline vec2 vec2_lerp(vec2 start, vec2 end, float alpha) 
{
    vec2 result;
    result.x = start.x + alpha * (end.x - start.x);
    result.y = start.y + alpha * (end.y - start.y);
    
    return result;
}

// Calculate the length (magnitude) of a vec2
inline float vec2_length(vec2 v)
{
    return sqrt(v.x * v.x + v.y * v.y);
}

// Rotate a vec2 by a specified angle (in radians)
inline vec2 vec2_rotate(vec2 v, float angle) 
{
    float cosTheta = std::cos(angle);
    float sinTheta = std::sin(angle);
    float dx = v.x * cosTheta - v.y * sinTheta;
    float dy = v.x * sinTheta + v.y * cosTheta;
    
    return { dx, dy };
}

// Calculate a rectangle with the same width as the input but with a modified height
inline rec get_rec_x1(rec rec)
{
    float fullArea = rec.width * rec.height;
    float bottomArea = fullArea - (fullArea * 0.9f);

    float y = rec.y + ((fullArea - bottomArea) / rec.width);

    return { rec.x, y, rec.width, bottomArea / rec.width };
}

// Calculate a rectangle with the same width as the input and a modified height
inline rec get_rec_x2(rec rec)
{
    float fullArea = rec.width * rec.height;
    float bottomArea = fullArea - (fullArea * 0.9f);

    return { rec.x, rec.y, rec.width, bottomArea / rec.width };
}

// Calculate a rectangle with the same height as the input but with a modified width
inline rec get_rec_y1(rec rec)
{
    float fullArea = rec.width * rec.height;
    float rightArea = fullArea - (fullArea * 0.9f);

    float x = rec.x + ((fullArea - rightArea) / rec.height);

    return { x, rec.y, rightArea / rec.height, rec.height };
}

// Calculate a rectangle with the same height as the input and a modified width
inline rec get_rec_y2(rec rec)
{
    float fullArea = rec.width * rec.height;
    float leftArea = fullArea * 0.1f;

    float x = rec.x;
    float width = leftArea / rec.height;

    return { x, rec.y, width, rec.height };
}

struct point
{
    point(float x_, float y_, int size_, clr color_, str name_) : 
    pos{ x_, y_ }, size{ size_ }, color{ color_ }, name{ name_ } {}
    inline void drawPos() const { DrawText(vec2_to_str(pos).c_str(), pos.x + 10, pos.y, 12, BLACK); };
    inline void draw() const { drawPos(); DrawCircle(pos.x, pos.y, size, color); }
    
    int  id;
    int  size;
    vec2 pos;
    str  name;
    clr  color;
};

struct cam2d : Camera2D
{
    cam2d() : ::Camera2D{ offset = { 0.0f, 0.0f }, target = { 0.0f, 0.0f }, rotation = 0.0f, zoom = 1.0f } {}

    vec2 get_mouse_dir()
    {
        vec2 dir = {};

        vec2 mousePos = GetMousePosition();
        vec2 worldMousePos = GetScreenToWorld2D(mousePos, *this);

        bool mouseRightButton = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

        if (CheckCollisionPointRec(worldMousePos, get_rec_y1(cRec)) && mouseRightButton)
        {
            dir.x += cameraSpeed;
        }
        if (CheckCollisionPointRec(worldMousePos, get_rec_y2(cRec)) && mouseRightButton)
        {
            dir.x -= cameraSpeed;
        }
        if (CheckCollisionPointRec(worldMousePos, get_rec_x1(cRec)) && mouseRightButton)
        {
            dir.y += cameraSpeed;
        }
        if (CheckCollisionPointRec(worldMousePos, get_rec_x2(cRec)) && mouseRightButton)
        {
            dir.y -= cameraSpeed;
        }

        return dir;
    }
    
    inline void update()
    {
        if (IsKeyDown(KEY_SPACE)) cameraSpeed = 3.5f;
        else cameraSpeed = 2.0f;

        // Update camera position (Using keyboard)
        if (IsKeyDown(KEY_W)) target.y -= cameraSpeed;
        if (IsKeyDown(KEY_S)) target.y += cameraSpeed;
        if (IsKeyDown(KEY_A)) target.x -= cameraSpeed;
        if (IsKeyDown(KEY_D)) target.x += cameraSpeed;

        vec2 mousePos = GetMousePosition();

        const bool mouseRightButton = IsMouseButtonDown(MOUSE_RIGHT_BUTTON);

        // Update camera position (Using mouse)
        if (vec2_length(get_mouse_dir()) > 0)
        {
            target = target + get_mouse_dir();
        }
        else if (mouseRightButton) 
        {
            target = mousePos;
        }

        // Smoothly move the camera towards the target
        float lerpFactor = 0.1f; // Adjust this value for the desired smoothness
        vec2 delta = target - GetScreenToWorld2D({(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2}, *this);
        offset = offset - vec2_scale(delta, lerpFactor);

        if ((GetMouseWheelMove() > 0.0f) && zoom < 3.0f) zoom += 0.1f;
        if ((GetMouseWheelMove() < 0.0f) && zoom > 0.0f) zoom -= 0.1f;

        cRec.x = target.x - (offset.x / zoom);
        cRec.y = target.y - (offset.y / zoom);

        cRec.width  = GetScreenWidth() / zoom;
        cRec.height = GetScreenHeight() / zoom;
    }

    inline ::Camera2D& begin() { ::BeginMode2D(*this); return (*this); }
    inline ::Camera2D& end() { ::EndMode2D(); return (*this); }

    float cameraSpeed = 2.0f;

    rec cRec;
};

inline vec2 bezier(vec2 p0, vec2 p1, vec2 p2, vec2 p3, float t)
{
    vec2 a = vec2_lerp(p0, p1, t);
    vec2 b = vec2_lerp(p1, p2, t);
    vec2 c = vec2_lerp(p2, p3, t);
    
    vec2 d = vec2_lerp(a, b, t);
    vec2 e = vec2_lerp(b, c, t);

    return vec2_lerp(d, e, t);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////                                                                                

struct gui_check_box { bool flag = 0; };

// Draw a simple button
bool gui_draw_button(const char* text, rec buttonRec) 
{
    vec2 mousePosition = GetMousePosition();
    bool isMouseOver = CheckCollisionPointRec(mousePosition, buttonRec);

    // Calculate the x and y coordinates for centered text
    int textX = buttonRec.x + (buttonRec.width - MeasureText(text, 11)) / 2;
    int textY = buttonRec.y + (buttonRec.height - 11) / 2;

    DrawRectangleRec(buttonRec, isMouseOver ? DARKBROWN : LIGHTGRAY);
    DrawText(text, textX, textY, 11, isMouseOver ? BLACK : DARKGRAY);

    return (isMouseOver && IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

static bool gui_draw_check_box(const char* text, vec2 pos, int size, gui_check_box* target) 
{
    rec recBox = { pos.x, pos.y, size, size };

    vec2 mousePosition = GetMousePosition();
    bool isMouseOver = CheckCollisionPointRec(mousePosition, recBox);

    if (isMouseOver && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        target->flag = !target->flag;
    }

    if (target->flag)
    {
        DrawRectangleRec(recBox, GRAY);
    }

    DrawRectangleLinesEx(recBox, 3, BLACK);

    const int fontSize = size / 3;

    DrawText(text, pos.x + 40, pos.y + 20, fontSize, BLACK);
}

/////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////// 

int main()
{
    int screenWidth = 940;
    int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Bézier curve");

    SetTargetFPS(120);

    point ball = { 100.0f * 1.5f, 200.0f * 2.0f, 20, BLUE, "Ball" };

    point p0 = { 100.0f * 1.5f, 200.0f * 2.0f, 20, GREEN, "p0" };
    point p1 = { 80.0f  * 1.5f, 100.0f * 2.0f, 20, GREEN, "p1" }; 
    point p2 = { 320.0f * 1.5f, 100.0f * 2.0f, 20, GREEN, "p2" };
    point p3 = { 300.0f * 1.5f, 200.0f * 2.0f, 20, GREEN, "p3" };

    point* points[] = { &p0, &p1, &p2, &p3 };

    float t = 0.0f; // Initialize t to 0.0f

    const vec2& posP0 = p0.pos;

    cam2d cam;

    bool isDragging = 0;

    int lockId = 0;

    bool isDebug = 0;

    bool isMoveAllMode0 = 0;
    bool isMoveAllMode1 = 0;

    const float updateTime = 0.084f;

    float timer = 0.0f;

    bool isBallPause = 0;

    bool forward = 1;

    ///////////////////////////////////
    ///////////////////////////////////
    gui_check_box checkBoxMode0;
    gui_check_box checkBoxMode1;
    gui_check_box checkBoxDebug;
    gui_check_box checkBoxGrid;
    gui_check_box checkBallPause;
    ///////////////////////////////////
    ///////////////////////////////////


    ///////////////////////////////////
    ///////////////////////////////////
    vec2 vGridPoints[20] = 
    {
        { 10.0f + 145.0f * 0, 12.0f + 142.0f * 0 },
        { 10.0f + 145.0f * 1, 12.0f + 142.0f * 0 },
        { 10.0f + 145.0f * 2, 12.0f + 142.0f * 0 },
        { 10.0f + 145.0f * 3, 12.0f + 142.0f * 0 },
        { 10.0f + 145.0f * 4, 12.0f + 142.0f * 0 },

        { 10.0f + 145.0f * 0, 12.0f + 142.0f * 1 },
        { 10.0f + 145.0f * 1, 12.0f + 142.0f * 1 },
        { 10.0f + 145.0f * 2, 12.0f + 142.0f * 1 },
        { 10.0f + 145.0f * 3, 12.0f + 142.0f * 1 },
        { 10.0f + 145.0f * 4, 12.0f + 142.0f * 1 },

        { 10.0f + 145.0f * 0, 12.0f + 142.0f * 2 },
        { 10.0f + 145.0f * 1, 12.0f + 142.0f * 2 },
        { 10.0f + 145.0f * 2, 12.0f + 142.0f * 2 },
        { 10.0f + 145.0f * 3, 12.0f + 142.0f * 2 },
        { 10.0f + 145.0f * 4, 12.0f + 142.0f * 2 },

        { 10.0f + 145.0f * 0, 12.0f + 142.0f * 3 },
        { 10.0f + 145.0f * 1, 12.0f + 142.0f * 3 },
        { 10.0f + 145.0f * 2, 12.0f + 142.0f * 3 },
        { 10.0f + 145.0f * 3, 12.0f + 142.0f * 3 },
        { 10.0f + 145.0f * 4, 12.0f + 142.0f * 3 },
    };

    ///////////////////////////////////
    ///////////////////////////////////

    for (int i = 0; i < 4; i++) points[i]->id = i;

    bool manualMode = 0;

    while (!WindowShouldClose())
    {
        /*********************************************************************************/
        /******************************Update Function************************************/
        /*********************************************************************************/

        cam.update();

        float deltaTime = 0.3f * GetFrameTime();

        vec2 mousePos = GetMousePosition();
        vec2 worldMousePos = GetScreenToWorld2D(mousePos, cam);

        if (!isBallPause && !manualMode)
        {
            if (forward)
            {
                if (t < 1.0f)
                {
                    // Use Bezier algorithm with t increasing from 0 to 1
                    t += deltaTime;
                }
                else
                {
                    // Object has reached the end of the path
                    t = 1.0f;
                    forward = 0;
                }
            }
            else
            {
                if (t > 0.0f && !isBallPause)
                {
                    // Use Bezier algorithm with t decreasing from 1 to 0
                    t -= deltaTime;
                }
                else
                {
                    // Object has returned to the starting point
                    t = 0.0f;
                    forward = 1;
                }
            }
        }

        isBallPause = checkBallPause.flag;
        isDebug = checkBoxDebug.flag;
        isMoveAllMode0 = checkBoxMode0.flag;
        isMoveAllMode1 = checkBoxMode1.flag;

        if (isMoveAllMode0)
        {
            for (auto& point : points)
            {
                float angle = 0.0f;
                if (angle < 360.0f)
                {
                    timer += deltaTime;
                    if (timer >= updateTime)
                    {
                        angle++;
                        point->pos = vec2_lerp(
                            point->pos, vec2_rotate(point->pos, angle), 
                            25.0f * GetFrameTime()
                        ); // vec2_rotate(point->pos, angle);

                        timer = 0.0f;
                    }
                    if (angle > 360.0f) angle = 0.0f;
                }
            }
        }
        if (isMoveAllMode1)
        {
            for (auto& point : points)
            {
                float angle = 0.0f;
                if (angle < 360.0f)
                {
                    timer += deltaTime;
                    if (timer >= updateTime)
                    {
                        angle++;
                        point->pos = vec2_rotate(point->pos, angle);
                        timer = 0.0f;
                    }
                    if (angle > 360.0f) angle = 0.0f;
                }
            }
        }

        // Use Bezier function to interpolate between control points
        vec2 newPos = bezier(p0.pos, p1.pos, p2.pos, p3.pos, t);

        // Update the object's position with the new calculated position
        ball.pos = newPos;

        vec2 a = vec2_lerp(p0.pos, p1.pos, t);
        vec2 b = vec2_lerp(p1.pos, p2.pos, t);
        vec2 c = vec2_lerp(p2.pos, p3.pos, t);      
        vec2 d = vec2_lerp(a, b, t);
        vec2 e = vec2_lerp(b, c, t);

        /*********************************************************************************/
        /*********************************Draw Function***********************************/
        /*********************************************************************************/
        
        BeginDrawing();

        ClearBackground(WHITE);

        /********************GRID*********************/
        /*********************************************/

        // Draw x and y axes
        DrawLineEx({ 0, (float)GetScreenHeight() / 2 }, { (float)GetScreenWidth(), (float)GetScreenHeight() / 2 },  6, RED);
        DrawLineEx({ (float)GetScreenWidth() / 2, 0 }, { (float)GetScreenWidth() / 2, (float)GetScreenHeight() }, 6, DARKGREEN);
        
        DrawText("x", 380, (float)GetScreenHeight() / 2, 32, RED);
        DrawText("y", (float)GetScreenWidth() / 2 + 10, 300, 32, DARKGREEN);

        // for (int i = 0; i < 20; i++)
        // {
        //     DrawCircle(vGridPoints[i].x, vGridPoints[i].y, 8.0f, PURPLE);
        // }

        // // Draw lines connecting the points
        // for (int i = 0; i < 19; i++) 
        // {
        //     for (int j = 0; j < 19; j++) 
        //     {
        //         DrawLineV(vGridPoints[i], vGridPoints[j + 1], RED);
        //     }
        // }

        if (checkBoxGrid.flag)
        {
            // Draw grid
            for (int x = -worldWidth / 2; x <= worldWidth / 2; x += gridSize)
            {
                DrawLine(x, -worldHeight / 2, x, worldHeight / 2, DARKGRAY);
            }
            for (int y = -worldHeight / 2; y <= worldHeight / 2; y += gridSize)
            {
                DrawLine(-worldWidth / 2, y, worldWidth / 2, y, DARKGRAY);
            }
        }

        /****************BEGIN CAMERA 2D******************/
        /*************************************************/
        cam.begin();

        for (int i = 0; i < 4; i++)
        {
            points[i]->draw();
            DrawText(points[i]->name.c_str(), points[i]->pos.x, points[i]->pos.y, 20, RED);

            int nextIndex = (i + 1) % 4; // Wrap around to the first point for the last connection
            DrawLine(points[i]->pos.x, points[i]->pos.y, points[nextIndex]->pos.x, points[nextIndex]->pos.y, GREEN);
        }

        for (float t = 0.0f; t <= 1.0f; t += 0.01f) 
        {
            vec2 point1 = bezier(p0.pos, p1.pos, p2.pos, p3.pos, t);
            t += 0.01f;
            vec2 point2 = bezier(p0.pos, p1.pos, p2.pos, p3.pos, t);
            DrawLineV(point1, point2, BLACK);
        }

        str ballPos = "x: " + to_string((int)ball.pos.x) + " y: " + to_string((int)ball.pos.x);
        DrawText(ballPos.c_str(), ball.pos.x - 30, ball.pos.y - 40, 14, BLACK);

        for (auto& point : points)
        {
            const float circleRadius = (isDragging) ? point->size * 3.0f : point->size; 
            if (CheckCollisionPointCircle(worldMousePos, point->pos, circleRadius) && 
                IsMouseButtonDown(MOUSE_LEFT_BUTTON) && !isDragging)
            {
                lockId = point->id;

                if (lockId == point->id) isDragging = 1;
            }
            else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                isDragging = 0;
                lockId = -1; // Reset the lockId when the mouse button is released
            }

            if (isDragging && point->id == lockId)
            {
                point->pos = worldMousePos;

                str p = point->name + ": " + vec2_to_str(point->pos);
                print(p, 1);
            }
        }

        DrawCircleV(worldMousePos, 8, BROWN);

        DrawCircleV(a, 12, PINK);
        DrawCircleV(b, 12, PINK);
        DrawCircleV(c, 12, PINK);
        DrawCircleV(d, 12, PINK);
        DrawCircleV(e, 12, PINK);

        DrawText("A", a.x, a.y, 14, BLACK);
        DrawText("B", b.x, b.y, 14, BLACK);
        DrawText("C", c.x, c.y, 14, BLACK);
        DrawText("D", d.x, d.y, 14, BLACK);
        DrawText("E", e.x, e.y, 14, BLACK);

        DrawLineV(a, b, PURPLE);
        DrawLineV(b, c, PURPLE);
        DrawLineV(d, e, PURPLE);

        ball.draw();

        if (isDebug)
        {
            DrawRectangleRec(get_rec_x1(cam.cRec), RED);
            DrawRectangleRec(get_rec_x2(cam.cRec), RED);
            DrawRectangleRec(get_rec_y1(cam.cRec), RED);
            DrawRectangleRec(get_rec_y2(cam.cRec), RED);
        }

        cam.end();

        /*********************************************/
        /*********************************************/

        /*********************************************************************************/
        /*************************************GUI*****************************************/
        /*********************************************************************************/

        // gui_draw_check_box("MODE 1",      { 20, 200 + 40 * 0 }, 35, &checkBoxMode0);
        // gui_draw_check_box("MODE 2",      { 20, 200 + 40 * 1 }, 35, &checkBoxMode1);
        // gui_draw_check_box("DEBUG MODE",  { 20, 200 + 40 * 2 }, 35, &checkBoxDebug);
        // gui_draw_check_box("SHOW GRID",   { 20, 200 + 40 * 3 }, 35, &checkBoxGrid);
        // gui_draw_check_box("PAUSE BALL",  { 20, 200 + 40 * 4 }, 35, &checkBallPause);

        checkBoxMode0.flag  = GuiCheckBox({ 20, 200 + 40 * 0, 20, 20 }, "MODE 1", checkBoxMode0.flag);
        checkBoxMode1.flag  = GuiCheckBox({ 20, 200 + 40 * 1, 20, 20 }, "MODE 2", checkBoxMode1.flag);
        checkBoxDebug.flag  = GuiCheckBox({ 20, 200 + 40 * 2, 20, 20 }, "DEBUG MODE", checkBoxDebug.flag);
        checkBoxGrid.flag   = GuiCheckBox({ 20, 200 + 40 * 3, 20, 20 }, "SHOW GRID", checkBoxGrid.flag);
        checkBallPause.flag = GuiCheckBox({ 20, 200 + 40 * 4, 20, 20 }, "PAUSE BALL", checkBallPause.flag);
        manualMode          = GuiCheckBox({ 20, 200 + 40 * 5, 20, 20 }, "Manual Mode", manualMode);

        if (manualMode) t = GuiSliderBar({ 80, 240 + 40 * 6, 120, 30 }, "MT Slider", to_string(t).c_str(), t, 0.0f, 1.0f);

        DrawText("Bézier curve", 20, 10, 24, BLACK);
        DrawText("by Wildan R Wijanarko", 45, 38, 12, BLACK);

        const bool isResetBall   = gui_draw_button("RESET BALL",   { 10 + 110 * 0,  65, 100, 30 });
        const bool isResetPoints = gui_draw_button("RESET POINTS", { 10 + 110 * 1,  65, 100, 30 });
        const bool isResetCamera = gui_draw_button("RESET CAMERA", { 10 + 110 * 2,  65, 100, 30 });
        
        /*****************************************************************************************/
        /*****************************************************************************************/

        if (isResetBall) 
        {
            print("Reset Button Pressed", 1);

            ball.pos = p0.pos;

            t = 0.0f;
        }
        if (isResetPoints)
        {
            print("Reset Points Pressed", 1);

            p0 = { 100.0f * 1.5f, 200.0f * 2.0f, 20, GREEN, "p0" };
            p1 = { 80.0f  * 1.5f, 100.0f * 2.0f, 20, GREEN, "p1" }; 
            p2 = { 320.0f * 1.5f, 100.0f * 2.0f, 20, GREEN, "p2" };
            p3 = { 300.0f * 1.5f, 200.0f * 2.0f, 20, GREEN, "p3" };
        }

        if (isResetCamera)
        {
            cam.zoom = 1.0f;
            cam.target = posP0;
            cam.offset = 
            { 
              GetScreenWidth() / 2.0f, 
              GetScreenHeight() / 2.0f 
            };
            cam.zoom = 1.0f;
        }

        DrawFPS(GetScreenWidth() - 100, 10);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
