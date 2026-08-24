#include "draw.hpp"
#include "raylib.h"
#include "raymath.h"
#include "util.hpp"
#include <assert.h>
#include <stdio.h>

void draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color)  {
    float vector_len = Vector3Length(direction);
    if (std::isnan(vector_len))
    {
        fprintf(stderr, "ERROR: draw_arrow: NaN vector: [%f %f %f]\n", decomp(direction));
        exit(-1);
    }
    if (fabs(vector_len) < 1e-4) return;  // zero length vector
    assert(fabs(vector_len - 1) < 1e-2);
    Vector3 end_pos = start_pos + direction * len;
    DrawCylinderEx(start_pos, end_pos, thickness, thickness, 100, color);


    DrawCylinderEx(end_pos + direction * 10, end_pos, 0, thickness * 2, 100, color);
}


void draw_vector(Vector3 start_pos, Vector3 vector, float thickness, Color color)  {
    draw_arrow(start_pos, Vector3Normalize(vector), Vector3Length(vector), thickness, color);
}

void draw_basis(Vector3 start_pos, Matrix basis) {
    draw_arrow(start_pos, { basis.m0, basis.m1,  basis.m2, }, 50, 1, RED);
    draw_arrow(start_pos, { basis.m4, basis.m5,  basis.m6  }, 50, 1, GREEN);
    draw_arrow(start_pos, { basis.m8, basis.m9,  basis.m10 }, 50, 1, BLUE);
}
