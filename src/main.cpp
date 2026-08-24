#define EULER_ANGLE_IMPLEMENTATION
#include "euler_angle.hpp"
#define CSERIAL_IMPLEMENTATION
#include "serial.h"
#include "uav.hpp"
#include "util.hpp"

#include <imgui.h>
#include <rlImGui.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "draw.hpp"
#define WIDTH 1600
#define HEIGHT 900

#define RESOURCES_DIR "./resources/"

void parse_serial_async(SerialPort port, size_t* parse_pos, char* line, size_t line_size, IMUMeasurements* imu) {
    char c;
    while (serial_read(port, &c, 1) == 1) {
        if (c == '\n') {
            line[*parse_pos] = '\0';
            IMUMeasurements read = {};
            if (sscanf(line, "%f, %f, %f, %f, %f, %f",
                       &read.acceleration.x, &read.acceleration.y, &read.acceleration.z,
                       &read.angular_velocity.x, &read.angular_velocity.y, &read.angular_velocity.z
                ) == 6) {
                read.angular_velocity.y *= DEG2RAD;
                read.angular_velocity.z *= DEG2RAD;
                read.angular_velocity.x *= DEG2RAD;
                *imu = read; 
            }
            *parse_pos = 0;
        }
        else if (c != '\r' && *parse_pos <= line_size - 2) {
            line[*parse_pos] = c;
            (*parse_pos)++;
        }
    }
}

void camera_init(Camera* camera) {
    camera->position   =  { -90, 90, 0 };
    camera->target     =  { 0, 0, 0 };
    camera->up         =  { 0, 1, 0 };
    camera->fovy       = 80;
    camera->projection = CAMERA_PERSPECTIVE;
    UpdateCamera(camera, CAMERA_FREE);
}



struct Settings {
    struct {
        bool roll, pitch, yaw;
    } lock = {false, false, false};
    bool draw_model = true;
    bool camera_mode = false;
    float filter_alpha = 0.92f;

    SerialPort port = {0};
    bool port_open = false;

    EulerAngle saved_angles = {};


    // TODO: signal error to the user in a better way (UI)
    void close_port_if_open() {
        if (port_open)  
            if (!serial_close(port)) {
                fprintf(stderr, "Error: could not close serial port: %s\n", serial_stringify_error(serial_get_last_error()));
                exit(-1);
            }
        port_open = false;
    }
    void connect_to_port(const char* path) {
        close_port_if_open();

        if (!serial_open(path, &port)) {
            fprintf(stderr, "Error: Could not connect to serial port: %s\n", serial_stringify_error(serial_get_last_error()));
            exit(-1);
        }
        port_open = true; 
    }
};



const ImGuiWindowFlags hud_flags =
        ImGuiWindowFlags_NoDecoration       |
        ImGuiWindowFlags_NoDocking          |
        ImGuiWindowFlags_NoBackground       |
        ImGuiWindowFlags_NoMove             | 
        ImGuiWindowFlags_NoResize           |
        ImGuiWindowFlags_NoSavedSettings    |
        ImGuiWindowFlags_AlwaysAutoResize;

void imgui_show_hud(Uav uav, MotionData estimate) {
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    
    const float flags = hud_flags |  
        ImGuiWindowFlags_NoFocusOnAppearing | 
        ImGuiWindowFlags_NoNav              |
        ImGuiWindowFlags_NoInputs;
    ImGui::Begin("##hud_overlay", nullptr, flags );
    {
        if (ImGui::BeginTable("##hud_table", 4))
        {
            const float width = 160.f;
            const ImGuiTableFlags col_flags = ImGuiTableColumnFlags_WidthFixed;
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImVec4(0, 0, 0, 0));
            {
                ImGui::TableSetupColumn("Estimate");
                ImGui::TableSetupColumn("Roll",  col_flags, width);
                ImGui::TableSetupColumn("Pitch", col_flags, width);
                ImGui::TableSetupColumn("Yaw",   col_flags, width);

                ImGui::TableNextRow(ImGuiTableRowFlags_Headers);


                ImGui::TableNextColumn();
                ImGui::Text("Estimate");

                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Roll(φ)");

                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Pitch(θ)");

                ImGui::TableNextColumn();
                ImGui::TextColored(ImVec4(0, 0, 1, 1), "Yaw(ψ)");
            }
            ImGui::PopStyleColor();

            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Accelerometer");

                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * estimate.accelerometer.roll);
                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * estimate.accelerometer.pitch);
                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * estimate.accelerometer.yaw);
            }

            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Gyroscope");

                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * estimate.gyroscope.roll);
                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * estimate.gyroscope.pitch);
                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * estimate.gyroscope.yaw);
            }
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("Filtered");

                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * uav.angle.roll);
                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * uav.angle.pitch);
                ImGui::TableNextColumn(); ImGui::Text("%.3f", RAD2DEG * uav.angle.yaw);
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void imgui_show_settings(Settings& settings, Uav uav) {
    ImGui::Begin("Settings");
    if (ImGui::Checkbox("Lock Roll", &settings.lock.roll)) settings.saved_angles.roll = uav.angle.roll;
    if (ImGui::Checkbox("Lock Pitch", &settings.lock.pitch)) settings.saved_angles.pitch = uav.angle.pitch; 
    if (ImGui::Checkbox("Lock Yaw", &settings.lock.yaw)) settings.saved_angles.yaw = uav.angle.yaw;
    ImGui::Checkbox("Draw Model", &settings.draw_model);
    ImGui::DragFloat("Complementary Constant", &settings.filter_alpha, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);

    ImGui::End();

}

void imgui_show_port_selection_popup(Settings& settings) {
    // TODO: Select from all available serial ports.
    const char* available_ports[] = { "/dev/ttyACM0", "/dev/ttyACM1" };
    static int selected_port = 0; // Here we store our selection data as an index.
    const char* combo_preview_value = available_ports[selected_port];
    // Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
    if (ImGui::BeginCombo("Select Serial Port", combo_preview_value, ImGuiComboFlags_None))
    {
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F);

        for (int n = 0; n < IM_COUNTOF(available_ports); n++)
        {
            const bool is_selected = (selected_port == n);
            if (ImGui::Selectable(available_ports[n], is_selected))
                selected_port = n;
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Connect")) {
        settings.connect_to_port(available_ports[selected_port]);
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
    // TODO: Refresh Available Ports
    if (ImGui::Button("Refresh")) {}

}

void imgui_show_port_configuration_popup(Settings& settings) {
    if (ImGui::Button("Connect to the serial port."))
        ImGui::OpenPopup("Port Selection");

    if (ImGui::BeginPopupModal("Port Selection", NULL, ImGuiWindowFlags_None)){
        imgui_show_port_selection_popup(settings);
        ImGui::EndPopup();
    }


    if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
}

void imgui_show_connection_button(Settings& settings) {
    auto size = ImGui::GetMainViewport()->Size;
    size.x = 20;
    size.y = size.y - 80;
    ImGui::SetNextWindowPos(size, ImGuiCond_Always);
    ImGui::Begin("##hud_connect_button", nullptr, hud_flags);
    {
        ImGui::PushFont(NULL, 40.f);

        if (!settings.port_open) {

            push_button_style(
                {0.8f, 0.1f, 0.1f, 1.0f},  // main
                {1.0f, 0.2f, 0.2f, 1.0f},  // hover
                {0.6f, 0.05f, 0.05f, 1.0f} // active
            );
            if (pretty_button("Not Connected."))
                ImGui::OpenPopup("Port Configuration");
            pop_button_style();

        } else {
            push_button_style(
                {0.1f, 0.4f, 0.9f, 1.0f},  // main
                {0.2f, 0.5f, 1.0f, 1.0f},  // hover
                {0.05f, 0.3f, 0.75f, 1.0f} // active
            );
            if (pretty_button("Connected."))
                settings.close_port_if_open();

            pop_button_style();
        }

        ImGui::PopFont();

        if (ImGui::BeginPopupModal("Port Configuration", NULL, ImGuiWindowFlags_None)) {
            imgui_show_port_configuration_popup(settings);
            ImGui::EndPopup();
        }

    }
    ImGui::End();
}

int main() {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIDTH, HEIGHT, "IMU Visualizer");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    Camera camera = {};
    camera_init(&camera);


    Model model = LoadModel(RESOURCES_DIR"/plane.obj");
    Texture2D texture = LoadTexture(RESOURCES_DIR"/plane_diffuse.png");
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;


    rlImGuiSetup(true);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImFont* imfont = io.Fonts->AddFontFromFileTTF(RESOURCES_DIR"/Inter-4.1/extras/ttf/Inter-Regular.ttf", 18.f);
    io.FontDefault = imfont;

    IMUMeasurements imu = {};
    Uav uav = {};

    Settings settings = {};
    
    static char line[512] = { 0 };
    size_t parse_pos = 0;

    while (!WindowShouldClose()) {
        if (settings.port_open) { 
            parse_serial_async(settings.port, &parse_pos, line, sizeof(line), &imu);
        }

        if (IsKeyDown(KEY_R)) {
            uav.reset();
            settings.saved_angles = EulerAngle{};
            imu = (IMUMeasurements) {};
        }

        if (IsKeyDown(KEY_Z)) {
            camera.target = {0, 0, 0};
            camera.up = {0, 1, 0};
        }
        if (IsKeyPressed(KEY_ESCAPE) && IsCursorHidden()) {
            EnableCursor();
            settings.camera_mode = false; 
        }
        if (IsKeyPressed(KEY_F) && !IsCursorHidden()) {
            DisableCursor();
            settings.camera_mode = true; 
        }

        if (settings.camera_mode)
            UpdateCamera(&camera, CAMERA_FREE);

        float dt = GetFrameTime();
        

        // Transform data in NED
        // TODO: do not multiply by - manually
        // imu.acceleration.y *= -1;
        // imu.acceleration.z *= -1;
        // imu.angular_velocity.y *= -1;
        // imu.angular_velocity.z *= -1;
        const Matrix NED = MatrixRotateX(PI);
        IMUMeasurements imu_ned = imu;
        imu_ned.angular_velocity = NED*imu.angular_velocity;
        
        MotionData estimate = uav.update_angle(imu_ned, settings.filter_alpha, dt);
        if (settings.lock.pitch) uav.angle.pitch = settings.saved_angles.pitch;
        if (settings.lock.yaw)   uav.angle.yaw   = settings.saved_angles.yaw;
        if (settings.lock.roll)  uav.angle.roll  = settings.saved_angles.roll;
        uav.basis = MatrixRotateZYX({uav.angle.roll, uav.angle.pitch, uav.angle.yaw});
        // TODO: do not add "g" to the acceleration.z manually and/or add ability to turn it off.
        // subtract "g" to the measured acceleration in Z axis because IMUs usually measure "true" acceleration
        // which includes gravitational acceleration
        /* a.z += 1; */
        /* printf("imu = [%f %f %f], a = [%f %f %f]\n", decomp(imu.acceleration), decomp(a)); */
        // u(t + dt) = u(t) + a(t)dt
        // x(t + dt) = x(t) + u(t)dt
        /* uav.u = Vector3Add(uav.u, Vector3Scale(a, dt)); */
        /* uav.x = Vector3Add(uav.x, Vector3Scale(uav.u, dt)); */
        


        BeginDrawing();
        BeginMode3D(camera);
        // 3D
        {
            ClearBackground(BLACK);

            DrawGrid(30, 10);

            rlPushMatrix();
            {
                //  North-East-Down (NED) system
                Matrix mat = MatrixRotateZYX({DEG2RAD*90, 0, 0});
                rlLoadIdentity();
                rlMultMatrixf(MatrixToFloat(mat));

                // This makes UAV point towards positive X initially. 
                Matrix model_offset = MatrixRotateZYX({-PI/2, 0, -PI/2});
                
                model.transform = model_offset * uav.basis;
                if (settings.draw_model)
                    DrawModel(model, uav.x, 1.0, WHITE);

                draw_basis(uav.x, uav.basis);
                draw_vector(uav.x, uav.basis*imu.angular_velocity, 1, ORANGE);

            }
            rlPopMatrix();

        }
        EndMode3D();

        rlImGuiBegin();

        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID,
                                        nullptr,
                                        ImGuiDockNodeFlags_PassthruCentralNode);

        ImGui::PushFont(NULL, 40.f);
        imgui_show_hud(uav, estimate);
        ImGui::PopFont();

        imgui_show_settings(settings, uav);
        imgui_show_connection_button(settings);

        ImGui::ShowDemoWindow(NULL);
        
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    settings.close_port_if_open();
} 
