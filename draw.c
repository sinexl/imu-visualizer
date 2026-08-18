#include "draw.h"
#include "raylib.h"
#include <assert.h>


void draw_ui(EulerAngle angle) {
    const int font_size = 25;
    const int border_padding = 20;

    int padding_y = border_padding;
    int padding_x = border_padding;
    EndMode3D();
    DrawText(TextFormat("%f, %f, %f", RAD2DEG*angle.roll, RAD2DEG*angle.pitch, RAD2DEG*angle.yaw), border_padding, border_padding, font_size, BLACK);
    padding_y += 10 + font_size;

    const char* text;
    int size; 

    text = "X";
    size = MeasureText(text, font_size); 
    DrawText(text, padding_x, padding_y, font_size, RED);
    padding_x += 5 + size;

    text = "Y";
    size = MeasureText(text, font_size); 
    DrawText(text, padding_x, padding_y, font_size, GREEN);
    padding_x += 5 + size;

    text = "Z";
    size = MeasureText(text, font_size); 
    DrawText(text, padding_x, padding_y, font_size, BLUE);
    padding_x += 5 + size;
}

void draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color)  {
    assert(fabs(Vector3Length(direction) - 1) <= 1e-2);
    Vector3 end_pos = Vector3Add(start_pos, Vector3Scale(direction, len));
    DrawCylinderEx(start_pos, end_pos, thickness, thickness, 100, color);

    Vector3 arrow_displacement = Vector3Scale(direction, 10);

    DrawCylinderEx(Vector3Add(end_pos, arrow_displacement), end_pos, 0, thickness * 2, 100, color);
}
