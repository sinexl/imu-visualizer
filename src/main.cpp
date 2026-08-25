#define EULER_ANGLE_IMPLEMENTATION
#include "euler_angle.hpp"
#define SERIAL_IMPLEMENTATION
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

namespace Styles {
    const ButtonStyle BlueButton =  {
        {0.1f, 0.4f, 0.9f, 1.0f},  // main
        {0.2f, 0.5f, 1.0f, 1.0f},  // hover
        {0.05f, 0.3f, 0.75f, 1.0f} // active
    };
    const ButtonStyle GreenButton = {
        {0.1f, 0.55f, 0.25f, 1.0f},  // main
        {0.15f, 0.7f, 0.3f, 1.0f},    // hover
        {0.05f, 0.4f, 0.15f, 1.0f}    // active
    };
    const ButtonStyle GrayButton = {
        {0.35f, 0.35f, 0.35f, 1.0f},  // main
        {0.45f, 0.45f, 0.45f, 1.0f},  // hover
        {0.25f, 0.25f, 0.25f, 1.0f}   // active
    };
    const ButtonStyle RedButton = {
        {0.8f, 0.1f, 0.1f, 1.0f},  // main
        {1.0f, 0.2f, 0.2f, 1.0f},  // hover
        {0.6f, 0.05f, 0.05f, 1.0f} // active
    };
}

struct Settings {
    Settings(const Settings &) = delete;
    Settings &operator=(const Settings &) = delete;

    Settings(Settings&& other) = delete;
    Settings &operator=(Settings&& other) = delete;

    ~Settings() {
        close_port_if_open();
    }

    Settings() {}

    // Angle locking settings
    struct {
        bool roll, pitch, yaw;
    } lock = {false, false, false};
    EulerAngle saved_angles = {};
    // Rendering settings
    bool draw_model = true;
    bool camera_mode = false;
    // Filtering settings
    float filter_alpha = 0.92f;

    // Serial port settings
    SerialConfiguration serial_cfg = serial_cfg_default();
    SerialPort port = {0};
    bool is_port_opened = false;

    // TODO: signal error to the user in a better way (UI)
    void close_port_if_open() {
        if (is_port_opened)
            if (!serial_close(port)) {
                fprintf(stderr, "Error: could not close serial port: %s\n", serial_stringify_error(serial_get_last_error()));
                exit(-1);
            }
        is_port_opened = false;
    }
    void connect_to_port(const char* path) {
        close_port_if_open();

        if (!serial_open(path, &serial_cfg, &port)) {
            fprintf(stderr, "Error: Could not connect to serial port: %s\n", serial_stringify_error(serial_get_last_error()));
            exit(-1);
        }
        is_port_opened = true;
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

struct Ui {
    Settings& settings;
    IMUMeasurements& imu_measurements;

    Ui(Settings& settings, IMUMeasurements& measurements) :
        settings(settings), imu_measurements(measurements) { }
    void show_hud(Uav uav, MotionData estimate);
    void show_settings(Uav uav);
    void show_connection_button();


    bool enable_hud = true; 
private:
    void show_port_selection_popup();
    void show_port_configuration_popup();

    // TODO: Select from all available serial ports.
    static constexpr const char* const available_ports[] = { "/dev/ttyACM0", "/dev/ttyACM1" };
    int selected_port = 0;
    bool port_was_selected = false;

    // This struct contains information & helpers for serial port configuration combos
    struct Combo {
    private:
        template<typename T>
        struct EnumString {
            T const value;
            const char* const name;
        };
    public:
        using BaudRate = EnumString<SerialBaudRate>;
        using DataBits = EnumString<SerialDataBits>;
        using StopBits = EnumString<SerialStopBits>;
        using Parity   = EnumString<SerialParity>;

        template<typename T>
        // Selector for ImGui combos.
        static const char* selector(void* data, int idx) {
            auto* list = static_cast<const T*>(data);
            return list[idx].name;
        }

        int baud_rate_idx = 4; // default 115200
        static constexpr const BaudRate baud_rates[] = {
            { SERIALB_9600,   "9600"   },
            { SERIALB_19200,  "19200"  },
            { SERIALB_38400,  "38400"  },
            { SERIALB_57600,  "57600"  },
            { SERIALB_115200, "115200" },
            { SERIALB_230400, "230400" },
        };

        int data_bit_idx = 3; // default 8
        static constexpr const DataBits data_bits[] = {
            { SERIALDB_5, "5" },
            { SERIALDB_6, "6" },
            { SERIALDB_7, "7" },
            { SERIALDB_8, "8" },
        };

        int stop_bits_idx = 0; // default 1
        static constexpr const StopBits stop_bits[] = {
            { SERIALSB_1, "1" },
            { SERIALSB_2, "2" },
        };

        int parity_idx = 0; // default None
        static constexpr const Parity parity[] = {
            { SERIALP_NONE, "None" },
            { SERIALP_EVEN, "Even" },
            { SERIALP_ODD,  "Odd" },
        };
    } combo = {};
};


void Ui::show_hud(Uav uav, MotionData estimate) {
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



void Ui::show_settings(Uav uav) {
    ImGui::Begin("Settings");
    {
        ImGui::SeparatorText("UI");
        ImGui::Checkbox("Enable HUD", &enable_hud);
        ImGui::Checkbox("Draw Model", &settings.draw_model);
    }
    {
        ImGui::SeparatorText("Orientation");
        if (ImGui::Checkbox("Lock Roll", &settings.lock.roll)) settings.saved_angles.roll = uav.angle.roll;
        if (ImGui::Checkbox("Lock Pitch", &settings.lock.pitch)) settings.saved_angles.pitch = uav.angle.pitch; 
        if (ImGui::Checkbox("Lock Yaw", &settings.lock.yaw)) settings.saved_angles.yaw = uav.angle.yaw;
    }
    {
        ImGui::SeparatorText("Complementary Filter");
        ImGui::DragFloat("Complementary Constant", &settings.filter_alpha, 0.005f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_None);
    }

    ImGui::End();
}

void Ui::show_port_selection_popup() {
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

    if (ImGui::Button("Select")) {
        port_was_selected = true;
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::Button("Close"))
        ImGui::CloseCurrentPopup();
    // TODO: Refresh Available Ports
    if (ImGui::Button("Refresh")) {}
}


void Ui::show_port_configuration_popup() {
    ImGui::Text("Connect Data Source");
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 24.0f);
    if (ImGui::Button("X", ImVec2(24, 24)))
        ImGui::CloseCurrentPopup();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Spacing();
    if (ImGui::BeginTable("##serial_grid", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Baud Rate");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##baud_rate", &combo.baud_rate_idx,
                         Combo::selector<Combo::BaudRate>,
                         (void*)combo.baud_rates, IM_ARRAYSIZE(combo.baud_rates))) {
            settings.serial_cfg.baud_rate = combo.baud_rates[combo.baud_rate_idx].value;
        };

        ImGui::TableSetColumnIndex(1);
        ImGui::Text("Data Bits");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##data_bits", &combo.data_bit_idx,
                         Combo::selector<Combo::DataBits>,
                         (void*)combo.data_bits, IM_ARRAYSIZE(combo.data_bits))) {
            settings.serial_cfg.data_bits = combo.data_bits[combo.data_bit_idx].value;
        }

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Spacing();
        ImGui::Text("Stop Bits");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##stop_bits", &combo.stop_bits_idx,
                         Combo::selector<Combo::StopBits>,
                         (void*)combo.stop_bits, IM_ARRAYSIZE(combo.stop_bits))) {
            settings.serial_cfg.stop_bits = combo.stop_bits[combo.stop_bits_idx].value;
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::Spacing();
        ImGui::Text("Parity");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##parity", &combo.parity_idx,
                    Combo::selector<Combo::Parity>,
                         (void*)combo.parity, IM_ARRAYSIZE(combo.parity))) {
            settings.serial_cfg.parity = combo.parity[combo.parity_idx].value;
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    push_button_style(Styles::BlueButton);
    if (pretty_button("Select Serial Port"))  {
        ImGui::OpenPopup("Select Serial Port");
    }
    pop_button_style();

    if (ImGui::BeginPopupModal("Select Serial Port", NULL, ImGuiWindowFlags_None))
    {
        show_port_selection_popup();
        ImGui::EndPopup();
    }

    float base_x = ImGui::GetWindowContentRegionMax().x - PRETTY_BUTTON_PADDING.x;
    const float margin = 20;
    if (port_was_selected) { 
        const char* conect = "Connect";
        ImGui::SameLine(base_x - ImGui::CalcTextSize(conect).x - margin);

        push_button_style(Styles::GreenButton);
        if (pretty_button(conect)) {
            settings.connect_to_port(available_ports[selected_port]);
            port_was_selected = false;
            ImGui::CloseCurrentPopup();
        } 
        pop_button_style(); 
    } else {
        const char* select = "Select port first";
        ImGui::SameLine(base_x - ImGui::CalcTextSize(select).x - margin);

        push_button_style(Styles::GrayButton);
        pretty_button(select);
        pop_button_style(); 
    }


}


void Ui::show_connection_button() {
    auto size = ImGui::GetMainViewport()->Size;
    size.x = 20;
    size.y = size.y - 80;
    ImGui::SetNextWindowPos(size, ImGuiCond_Always);
    ImGui::Begin("##hud_connect_button", nullptr, hud_flags);
    {
        ImGui::PushFont(NULL, 40.f);
        const float button_rounding = 8.0f;
        if (settings.is_port_opened) {
            push_button_style(Styles::BlueButton);
            if (pretty_button("Connected.", button_rounding)) {
                settings.close_port_if_open();
                // Reset only angular velocity so that UAV doesn't instantly lose
                // it's orientation
                imu_measurements.angular_velocity = {};
            }
            pop_button_style();

        } else {
            push_button_style(Styles::RedButton);
            if (pretty_button("Not Connected.", button_rounding))
                ImGui::OpenPopup("Port Configuration");
            pop_button_style();

        }

        ImGui::PopFont();

        if (ImGui::BeginPopupModal("Port Configuration", NULL, ImGuiWindowFlags_None)) {
            show_port_configuration_popup();
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
    Ui ui = {settings, imu};
    
    static char line[512] = { 0 };
    size_t parse_pos = 0;

    while (!WindowShouldClose()) {
        if (settings.is_port_opened) { 
            parse_serial_async(settings.port, &parse_pos, line, sizeof(line), &imu);
        }

        if (IsKeyDown(KEY_R)) {
            uav.reset();
            settings.saved_angles = EulerAngle{};
            imu = (IMUMeasurements)  {};
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
        {
            ImGui::DockSpaceOverViewport(
                ImGui::GetMainViewport()->ID,
                nullptr,
                ImGuiDockNodeFlags_PassthruCentralNode);

            ImGui::PushFont(NULL, 40.f);
            if (ui.enable_hud) {
                ui.show_hud(uav, estimate);
            }
            ImGui::PopFont();

            ui.show_settings(uav);
            ui.show_connection_button();

            ImGui::ShowDemoWindow(NULL);

        }
        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
} 
