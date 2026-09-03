#include "draw.hpp"
#include "raylib.h"
#include "raymath.h"
#include <assert.h>
#include <cmath>

bool draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color)  {
    float vector_len = Vector3Length(direction);
    if (std::isnan(vector_len))
    {
        return false;
    }
    if (fabs(vector_len) < 1e-4) return true;  // zero length vector is not an error
    assert(fabs(vector_len - 1) < 1e-2);
    Vector3 end_pos = start_pos + direction * len;
    DrawCylinderEx(start_pos, end_pos, thickness, thickness, 100, color);


    DrawCylinderEx(end_pos + direction * 10, end_pos, 0, thickness * 2, 100, color);
    return true;
}


bool draw_vector(Vector3 start_pos, Vector3 vector, float thickness, Color color)  {
    return draw_arrow(start_pos, Vector3Normalize(vector), Vector3Length(vector), thickness, color);
}

bool draw_basis(Vector3 start_pos, Matrix basis) {
    bool a = draw_arrow(start_pos, { basis.m0, basis.m1,  basis.m2, }, 50, 1, RED);
    bool b = draw_arrow(start_pos, { basis.m4, basis.m5,  basis.m6  }, 50, 1, GREEN);
    bool c = draw_arrow(start_pos, { basis.m8, basis.m9,  basis.m10 }, 50, 1, BLUE);
    return a && b && c;
}

bool pretty_button(const char* text, float rounding)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, PRETTY_BUTTON_PADDING);

    bool clicked = ImGui::Button(text);

    ImGui::PopStyleVar(2);
    return clicked;
}

void push_button_style(ImVec4 main, ImVec4 hover, ImVec4 active) {
    ImGui::PushStyleColor(ImGuiCol_Button,        main);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  active);
}


void push_button_style(ButtonStyle style) {
    push_button_style(style.main, style.hover, style.active);
}

void pop_button_style() {
    ImGui::PopStyleColor(3);
}
