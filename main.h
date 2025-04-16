//
// Created by eclipsedmango on 12/04/25.
//

#ifndef MAIN_H
#define MAIN_H

#include "raylib.h"
#include "raymath.h"
#include "vector"

#include <thread>

struct newRay {
    Vector2 rayOrigin = {};
    Vector2 newPos = {};
    Vector2 rayDirection = {};
    Vector2 finalPos = {};
    Vector2 startPos = {};

    float rayAngle = 0.0;
    float totalDistance = 1.0;

    bool drawRay = false;
    bool outOfBounds = false;
};

struct player {
    Vector2 pos = Vector2(1920 / 2, 1080 / 2);
    Vector2 velocity = {};

    Shader shader{};

    float speed = 500.0;
    float acceleration = 10.0;
    Rectangle* body = {};
};

struct object {
    Vector2 pos = {};
    Vector2 center = {};
    Vector2 size = {};
    Rectangle* body = {};
    Color color = {};
};

struct quadTree {
    Rectangle bounds = {};
    std::vector<object*> quadObjects;
    int capacity;

    bool divided = false;

    quadTree* nw = nullptr;
    quadTree* ne = nullptr;
    quadTree* sw = nullptr;
    quadTree* se = nullptr;

    quadTree(Rectangle rec, int cap);
    ~quadTree() = default;

    void insert(object* obj);
    void subdivide();
    void query(Rectangle range, std::vector<object*>& found);

    static int getQuad(const Vector2 &pos, bool allowInBetween);
    static bool isSameQuad(int a, int b);
};

inline player player;

inline int windowWidth = 1920;
inline int windowHeight = 1080;

inline std::vector<object> objects = {};
inline std::vector<newRay> rays = {};
inline std::vector<std::thread> threads;

inline bool runThreads = true;

int main();
void marchRay(newRay &ray, quadTree &quadTree);
void createRay(newRay newRay);
void processRayChunk(int i, int threadCount, quadTree &quadTree);

#endif //MAIN_H
