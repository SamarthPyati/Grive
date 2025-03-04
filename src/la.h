/* LINEAR ALGEBRA LIBRARY */
#ifndef LA_H_
#define LA_H_

#include <math.h>

typedef struct {
    float x, y;
} Vec2;

Vec2 vec2_one();
Vec2 vec2_zero();

double vec2_norm(Vec2 v); 
double vec2_dot(Vec2 a, Vec2 b);
Vec2 vec2c(float x);
Vec2 vec2s(float x, float y);
Vec2 vec2_add(Vec2 a, Vec2 b);
Vec2 vec2_sub(Vec2 a, Vec2 b);
Vec2 vec2_mul(Vec2 a, Vec2 b);
Vec2 vec2_div(Vec2 a, Vec2 b);

#endif // LA_H_