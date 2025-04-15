#include <iostream>

#include "main.h"

int main() {
    // These should only be initialized here, so we can have menus.
    const int monitor = GetCurrentMonitor();
    SetConfigFlags(FLAG_MSAA_4X_HINT);

    constexpr float physicsDelta = 1.0f / 240.0f;

    InitWindow(windowWidth, windowHeight, "Rays");

    SetWindowMonitor(monitor);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    float physicsTimer = 0.0f;

    player.body = new Rectangle;
    player.body->height = 25;
    player.body->width = 25;
    player.body->x = player.pos.x;
    player.body->y = player.pos.y;

    std::cout << windowWidth << "\n";
    std::cout << windowWidth / 2.0 << "\n";

    float centerX = windowWidth / 2.0;
    float centerY = windowHeight / 2.0;

    // Create Rectangles.
    for (int i = 0; i < 500; ++i) {
        object* obj = new object;

        obj->pos.x = GetRandomValue(0, windowWidth);
        obj->pos.y = GetRandomValue(0, windowHeight);

        obj->size = Vector2(10, 10);

        obj->center.x = obj->pos.x + obj->size.x / 2.0;
        obj->center.y = obj->pos.y + obj->size.y / 2.0;

        obj->color = BLUE;

        obj->body = new Rectangle;
        obj->body->x = obj->pos.x;
        obj->body->y = obj->pos.y;
        obj->body->width = obj->size.x;
        obj->body->height = obj->size.y;

        objects.push_back(*obj);
    }

    //Create rays.
    constexpr int rayAmount = 1200;
    for (int i = 0; i < rayAmount; ++i) {
        newRay ray = {};
        ray.rayAngle = i * (PI * 2.0 / rayAmount);
        createRay(ray);
    }

    // Create QuadTree
    quadTree quadtree(Rectangle(0, 0, static_cast<float>(windowWidth), static_cast<float>(windowHeight)), 4);
    for (int i = 0; i < objects.size(); ++i) {
        quadtree.insert(&objects[i]);
    }

    // Create global game loop.
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (IsWindowResized()) {
            windowHeight = GetScreenHeight();
            windowWidth = GetScreenWidth();
        }

        int moveDirX = IsKeyDown(KEY_D) - IsKeyDown(KEY_A);
        int moveDirY = IsKeyDown(KEY_S) - IsKeyDown(KEY_W);

        // Update the physicsTimer.
        physicsTimer += deltaTime;

        // Physics loop that depends on the physicsDelta.
        while (physicsTimer > physicsDelta) {
            physicsTimer -= physicsDelta;

            player.velocity.x = Lerp(player.velocity.x, moveDirX * player.speed, physicsDelta * player.acceleration);
            player.velocity.y = Lerp(player.velocity.y, moveDirY * player.speed, physicsDelta * player.acceleration);

            player.pos = Vector2Add(player.pos, Vector2Scale(player.velocity, physicsDelta));

            player.body->x = player.pos.x - 12.5;
            player.body->y = player.pos.y - 12.5;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangle(player.pos.x - 12.5, player.pos.y - 12.5, 25, 25, RED);

        // Draw Rectangles.
        for (int i = 0; i < objects.size(); ++i) {
            DrawRectangleRec(*objects[i].body, BLUE);
        }

        for (int i = 0; i < rays.size(); ++i) {
            marchRay(rays[i], quadtree);
        }

        DrawText(TextFormat("%i", rays.size()), 25, 50, 26, WHITE);
        DrawFPS(25, 25);
        EndDrawing();
    }

    return 0;
}

void createRay(newRay newRay) {
    newRay.rayOrigin = Vector2(windowWidth / 2.0, windowHeight / 2.0);
    newRay.rayDirection = Vector2Rotate(Vector2(1, 1), newRay.rayAngle);
    newRay.newPos = newRay.rayOrigin;
    newRay.finalPos = Vector2(-1, -1);

    rays.push_back(newRay);
}

void marchRay(newRay &ray, quadTree &quadTree) {
    Vector2 movedPos = Vector2Add(ray.rayOrigin, ray.rayDirection);

    if (!quadTree::isSameQuad(quadTree::getQuad(player.pos, true), quadTree::getQuad(movedPos, false)) && ray.finalPos.x != -1) {
        DrawLineEx(ray.rayOrigin, ray.newPos, 1, GREEN);
        return;
    }

    int stepAmount = 350;
    bool collided = false;

    Vector2 oldPos = ray.rayOrigin;

    for (int i = 0; i < stepAmount; ++i) {
        ray.newPos = Vector2Add(oldPos, Vector2Scale(ray.rayDirection, 2));

        // Define a search range near the ray position.
        float searchRadius = 25.0f;
        Rectangle searchArea = Rectangle(ray.newPos.x - searchRadius, ray.newPos.y - searchRadius,
            searchRadius * 2,searchRadius * 2
        );

        // Ask the quadtree for nearby objects.
        std::vector<object*> nearby;
        quadTree.query(searchArea, nearby);

        // Check only those objects.
        for (int i = 0; i < nearby.size(); ++i) {
            if (CheckCollisionPointRec(ray.newPos, *nearby[i]->body)) {
                // std::cout << "Hit!" << std::endl;
                collided = true;
                break;
            }
        }

        if (Vector2Distance(ray.newPos, player.pos) <= 35.0) {
            if (CheckCollisionPointRec(ray.newPos, *player.body)) {
                // std::cout << "Hit!" << std::endl;
                collided = true;
                break;
            }
        }

        if (collided) {break;}

        oldPos = ray.newPos;
    }

    ray.finalPos = ray.newPos;
    DrawLineEx(ray.rayOrigin, ray.newPos, 1, GREEN);
}

quadTree::quadTree(Rectangle rec, int cap) {
    this->bounds = rec;
    this->capacity = cap;

    divided = false;

    nw = nullptr;
    ne = nullptr;
    sw = nullptr;
    se = nullptr;
}

void quadTree::subdivide() {
    float x = bounds.x;
    float y = bounds.y;
    float w = bounds.width / 2;
    float h = bounds.height / 2;

    ne = new quadTree(Rectangle(x + w, y, w, h), capacity);
    nw = new quadTree(Rectangle(x, y, w, h), capacity);
    se = new quadTree(Rectangle(x + w, y + h, w, h), capacity);
    sw = new quadTree(Rectangle(x, y + h, w, h), capacity);

    divided = true;
}

void quadTree::insert(object* obj) {
    if (!CheckCollisionPointRec(obj->center, bounds)) return;

    quadTree* stack[128];
    int stackIndex = 0;

    stack[stackIndex++] = this;

    while (stackIndex > 0) {
        quadTree* node = stack[--stackIndex];

        if (!CheckCollisionPointRec(obj->center, node->bounds)) continue;

        if (!node->divided) {
            if (node->quadObjects.size() < node->capacity) {
                node->quadObjects.push_back(obj);
                continue;
            }

            node->subdivide();
        }

        stack[stackIndex++] = node->ne;
        stack[stackIndex++] = node->nw;
        stack[stackIndex++] = node->se;
        stack[stackIndex++] = node->sw;
    }
}

void quadTree::query(Rectangle range, std::vector<object*>& found) {
    if (!CheckCollisionRecs(bounds, range)) return;

    quadTree* stack[64];
    int stackIndex = 0;

    stack[stackIndex++] = this;

    while (stackIndex > 0) {
        quadTree* node = stack[--stackIndex];

        if (!CheckCollisionRecs(node->bounds, range)) continue;

        for (int i = 0; i < node->quadObjects.size(); ++i) {
            if (CheckCollisionPointRec(node->quadObjects[i]->center, range)) {
                found.push_back(node->quadObjects[i]);
            }
        }

        if (node->divided) {
            stack[stackIndex++] = node->ne;
            stack[stackIndex++] = node->nw;
            stack[stackIndex++] = node->se;
            stack[stackIndex++] = node->sw;
        }
    }
}

bool quadTree::isSameQuad(const int a, const int b) {
    // Middle and same Quads.
    if (a == b) {return true;}
    if (a == 9 || b == 9) {return true;}

    // Top side Quads.
    if (a == 5 && b == 1 || a == 5 && b == 2) {return true;}
    if (a == 1 && b == 5 || a == 2 && b == 5) {return true;}

    // Bottom side Quads.
    if (a == 6 && b == 4 || a == 6 && b == 3) {return true;}
    if (a == 4 && b == 5 || a == 3 && b == 6) {return true;}

    // Left side Quads.
    if (a == 8 && b == 1 || a == 8 && b == 4) {return true;}
    if (a == 1 && b == 8 || a == 4 && b == 8) {return true;}

    // Ride side Quads.
    if (a == 7 && b == 2 || a == 7 && b == 3) {return true;}
    if (a == 2 && b == 7 || a == 3 && b == 7) {return true;}

    return false;
}

int quadTree::getQuad(const Vector2 &pos, bool allowInBetween) {
    const bool up = pos.y < windowHeight / 2.0;
    const bool left = pos.x < windowWidth / 2.0;

    if (allowInBetween) {
        const bool closeToHorz = abs(pos.y - windowHeight / 2.0) < 80;
        const bool closeToVert = abs(pos.x - windowWidth / 2.0) < 80;

        if (closeToHorz && closeToVert) {return 9;}
        if (closeToHorz && left) {return 8;}
        if (closeToHorz) {return 7;}
        if (closeToVert && up) {return 5;}
        if (closeToVert) {return 6;}
    }

    if (up && left) {return 1;}
    if (up) {return 2;}
    if (!left) {return 3;}
    return 4;
}
