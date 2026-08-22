#include "draw.hpp"
#include "raylib.h"
#include "raymath.h"
#include "util.hpp"
#include <assert.h>
#include <stdio.h>

void write_seq(const char* text, int* padding_x, int* padding_y, int font_size, Color color, Font font) {
    int size = MeasureText(text, font_size);
    DrawTextEx(font, text, {static_cast<float>(*padding_x), static_cast<float>(*padding_y)}, font_size, 1, color);
    *padding_x += 10 + size;
}

void draw_ui(Uav uav, EulerAngle gyro_estimate, EulerAngle accelerometer_estimate, Font font, int font_size) {
    const int border_padding = 20;

    int padding_y = border_padding;
    int padding_x = border_padding;
    write_seq("X", &padding_x, &padding_y, font_size, RED, font);
    write_seq("Y", &padding_x, &padding_y, font_size, GREEN, font);
    write_seq("Z", &padding_x, &padding_y, font_size, BLUE, font);
    DrawTextEx(font,
               TextFormat("%.3f, %.3f, %.3f", RAD2DEG*uav.x.x, RAD2DEG*uav.x.y, RAD2DEG*uav.x.z),
               {static_cast<float>(padding_x), static_cast<float>(padding_y)}, font_size, 1, WHITE);
    padding_y += 10 + font_size;

    padding_x = border_padding;
    write_seq("Roll", &padding_x, &padding_y, font_size, RED, font);
    write_seq("Pitch", &padding_x, &padding_y, font_size, GREEN, font);
    write_seq("Yaw", &padding_x, &padding_y, font_size, BLUE, font);
    DrawTextEx(font,
               TextFormat("%.3f, %.3f, %.3f", RAD2DEG*uav.angle.roll, RAD2DEG*uav.angle.pitch, RAD2DEG*uav.angle.yaw),
               {static_cast<float>(padding_x), static_cast<float>(padding_y)}, font_size, 1, WHITE);

    padding_y += 10 + font_size;
    padding_x = border_padding;
    write_seq("Gyroscope:", &padding_x, &padding_y, font_size, WHITE, font);
    DrawTextEx(font,
               TextFormat("%.3f, %.3f, %.3f", RAD2DEG*gyro_estimate.roll, RAD2DEG*gyro_estimate.pitch, RAD2DEG*gyro_estimate.yaw),
               {static_cast<float>(padding_x), static_cast<float>(padding_y)}, font_size, 1, WHITE);
    padding_y += 10 + font_size;
    padding_x = border_padding;

    write_seq("Accelerometer:", &padding_x, &padding_y, font_size, WHITE, font);
    DrawTextEx(font,
               TextFormat("%.3f, %.3f, %.3f", RAD2DEG*accelerometer_estimate.roll, RAD2DEG*accelerometer_estimate.pitch, RAD2DEG*accelerometer_estimate.yaw),
               {static_cast<float>(padding_x), static_cast<float>(padding_y)}, font_size, 1, WHITE);
}

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
