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

    // Create Rectangles.
    for (int i = 0; i < 100; ++i) {
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
    for (int i = 0; i < 629; ++i) {
        newRay ray = {};
        float angle = 0;
        ray.rayAngle = angle + (i * 0.01);
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
            if (rays[i].collided || rays[i].outOfBounds) {
                break;
            }
            marchRay(rays[i], quadtree);
        }

        DrawFPS(25, 25);
        EndDrawing();
    }

    return 0;
}

void createRay(newRay newRay) {
    newRay.rayOrigin = Vector2(windowWidth / 2.0, windowHeight / 2.0);
    newRay.rayDirection = Vector2Rotate(Vector2(1, 1), newRay.rayAngle);
    newRay.oldPos = newRay.rayOrigin;
    newRay.newPos = newRay.rayOrigin;

    rays.push_back(newRay);
}

void marchRay(newRay ray, quadTree quadTree) {
    int stepAmount = 120;

    for (int i = 0; i < stepAmount; ++i) {
        ray.newPos = Vector2Add(ray.oldPos, Vector2Scale(ray.rayDirection, 5.0));

        // Define a search range near the ray position.
        float searchRadius = 35.0f;
        Rectangle searchArea = Rectangle(ray.newPos.x - searchRadius, ray.newPos.y - searchRadius,
            searchRadius * 2,searchRadius * 2
        );

        // Ask the quadtree for nearby objects.
        std::vector<object*> nearby;
        quadTree.query(searchArea, nearby);

        // Check only those objects./
        for (int i = 0; i < nearby.size(); ++i) {
            if (CheckCollisionPointRec(ray.newPos, *nearby[i]->body)) {
                ray.collided = true;
                break;
            }
        }

        // for (int i = 0; i < objects.size(); ++i) {
        //     float dist = Vector2DistanceSqr(ray.newPos, objects[i].center);
        //     float maxDist = 35.0;
        //
        //     if (dist < maxDist * maxDist) {
        //         if (CheckCollisionPointRec(ray.newPos, *objects[i].body)) {
        //             ray.collided = true;
        //             break;
        //         }
        //     }
        // }

        if (Vector2Distance(ray.newPos, player.pos) <= 35.0) {
            if (CheckCollisionPointRec(ray.newPos, *player.body)) {
                // std::cout << "Hit!" << std::endl;
                ray.collided = true;
                break;
            }
        }

        if (ray.collided) {break;}

        ray.oldPos = ray.newPos;
    }

    if (!ray.collided) {
        ray.finalPos = ray.newPos;
    }

    DrawLineEx(ray.rayOrigin, ray.newPos, 1, GREEN);
    DrawRectangle(ray.finalPos.x - 2, ray.finalPos.y - 2, 4, 4, YELLOW);
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

    if (quadObjects.size() < capacity) {
        quadObjects.push_back(obj);
    } else {
        if (!divided) subdivide();

        ne->insert(obj);
        nw->insert(obj);
        se->insert(obj);
        sw->insert(obj);
    }
}

void quadTree::query(Rectangle range, std::vector<object*>& found) {
    if (!CheckCollisionRecs(bounds, range)) return;

    for (int i = 0; i < quadObjects.size(); ++i) {
        if (CheckCollisionPointRec(quadObjects[i]->center, range)) {
            found.push_back(quadObjects[i]);
        }
    }

    if (divided) {
        ne->query(range, found);
        nw->query(range, found);
        se->query(range, found);
        sw->query(range, found);
    }
}
