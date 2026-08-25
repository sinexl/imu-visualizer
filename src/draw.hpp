#ifndef DRAW_H_
#define DRAW_H_
#include "raylib.h"
#include <imgui.h>

void draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color);
void draw_vector(Vector3 start_pos, Vector3 vec, float thickness, Color color);
void draw_basis(Vector3 start_pos, Matrix basis);
const auto PRETTY_BUTTON_PADDING = ImVec2(12, 6);

struct ButtonStyle {
    ImVec4 main;
    ImVec4 hover;
    ImVec4 active;
};
bool pretty_button(const char* text, float rounding = 4.0f);
void push_button_style(ImVec4 main ,ImVec4 hover, ImVec4 active);
void push_button_style(ButtonStyle style);
void pop_button_style();

#endif // DRAW_H_
