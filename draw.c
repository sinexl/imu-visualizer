#include "draw.h"
#include "raylib.h"
#include "raymath.h"
#include <assert.h>

void write_seq(const char* text, int* padding_x, int* padding_y, int font_size, Color color, Font font) {
    int size = MeasureText(text, font_size);
    DrawTextEx(font, text, (Vector2){*padding_x, *padding_y}, font_size, 1, color);
    *padding_x += 10 + size;
}

void draw_ui(EulerAngle angle, Font font, int font_size) {
    const int border_padding = 20;

    int padding_y = border_padding;
    int padding_x = border_padding;
    EndMode3D();
    DrawTextEx(font,
               TextFormat("%f, %f, %f", RAD2DEG*angle.roll, RAD2DEG*angle.pitch, RAD2DEG*angle.yaw),
               (Vector2) {border_padding, border_padding}, font_size, 1, WHITE);
    padding_y += 10 + font_size;

    write_seq("X", &padding_x, &padding_y, font_size, RED, font);
    write_seq("Y", &padding_x, &padding_y, font_size, GREEN, font);
    write_seq("Z", &padding_x, &padding_y, font_size, BLUE, font);

    padding_x += 10;
    write_seq("Roll", &padding_x, &padding_y, font_size, RED, font);
    write_seq("Pitch", &padding_x, &padding_y, font_size, GREEN, font);
    write_seq("Yaw", &padding_x, &padding_y, font_size, BLUE, font);
}

void draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color)  {
    assert(fabs(Vector3Length(direction) - 1) < 1e-2);
    Vector3 end_pos = Vector3Add(start_pos, Vector3Scale(direction, len));
    DrawCylinderEx(start_pos, end_pos, thickness, thickness, 100, color);

    Vector3 arrow_displacement = Vector3Scale(direction, 10);

    DrawCylinderEx(Vector3Add(end_pos, arrow_displacement), end_pos, 0, thickness * 2, 100, color);
}
