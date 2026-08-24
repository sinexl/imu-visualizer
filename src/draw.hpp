#ifndef DRAW_H_
#define DRAW_H_
#include "raylib.h"

void draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color);
void draw_vector(Vector3 start_pos, Vector3 vec, float thickness, Color color);
void draw_basis(Vector3 start_pos, Matrix basis);


#endif // DRAW_H_
