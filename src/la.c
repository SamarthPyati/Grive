#include "la.h"

Vec2 vec2_one() {
    return (Vec2){.x = 1, .y = 1};
}

Vec2 vec2_zero() {
    return (Vec2){.x = 0, .y = 0};
}

double vec2_norm(Vec2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
} 

Vec2 vec2c(float x) {
    return (Vec2){.x = x, .y = x};
}

Vec2 vec2s(float x, float y) {
    return (Vec2){x, y};
}

Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

double vec2_dot(Vec2 a, Vec2 b) {
    return (double){a.x * b.x + a.y * b.y};
}

Vec2 vec2_mul(Vec2 a, Vec2 b) {
    return (Vec2){a.x * b.x, a.y * b.y};
}

Vec2 vec2_div(Vec2 a, Vec2 b) {
    return (Vec2){a.x / b.x, a.y / b.y};
}

