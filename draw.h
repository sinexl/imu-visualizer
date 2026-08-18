#ifndef DRAW_H_
#define DRAW_H_
#include "main.h"
#include "raylib.h"

void draw_ui(EulerAngle angle, Font font, int font_size);
void draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color);


#endif // DRAW_H_
