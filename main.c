#include "main.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>
#include "draw.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define WIDTH 1600
#define HEIGHT 900

// in m/s^2
#define GRAVITATIONAL_ACCELERATION 9.80665f
    
//partially taken from https://stackoverflow.com/questions/6947413/how-to-open-read-and-write-from-serial-port-in-c
int open_serial(const char* path) {
    int fd = open(path, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "Error: could not open %s: %s\n", path, strerror(errno));
        return -1; 
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "Error: could not retrieve serial port info: %s\n", strerror(errno));
    }
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "Error: could not update tty: %s\n", strerror(errno));
        return -1;
    }
    return fd; 
}

void print_matrix(Matrix x) {
    printf("{\n");
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m0, x.m4, x.m8, x.m12);
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m1, x.m5, x.m9, x.m13);
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m2, x.m6, x.m10, x.m14);
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m3, x.m7, x.m11, x.m15);
    printf("}\n");
}

void draw_basis(Vector3 start_pos, Matrix basis) {
    draw_arrow(start_pos, (Vector3) {basis.m0, basis.m1,  basis.m2, }, 50, 1, RED);
    draw_arrow(start_pos, (Vector3) {basis.m4, basis.m5,  basis.m6  }, 50, 1, GREEN);
    draw_arrow(start_pos, (Vector3) {basis.m8, basis.m9,  basis.m10 }, 50, 1, BLUE);
}


int main() { 
    const char* path = "/dev/ttyACM0";

    int fd;
    if ((fd = open_serial(path)) == -1) return -1; 

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIDTH, HEIGHT, "IMU Visualizer");
    SetTargetFPS(120);
    SetExitKey(KEY_NULL);

    Camera camera = {0};

    const int font_size = 40;
    Font font = LoadFontEx("./resources/Inter-4.1/extras/ttf/Inter-Regular.ttf", font_size, NULL, 0);

    camera.position   = (Vector3) { -90, 90, 0 };
    camera.target     = (Vector3) { 0, 0, 0 };
    camera.up         = (Vector3) { 0, 1, 0 };
    camera.fovy       = 80;
    camera.projection = CAMERA_PERSPECTIVE;

    Model model = LoadModel("./resources/plane.obj");
    Texture2D texture = LoadTexture("./resources/plane_diffuse.png");
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    DisableCursor();

    IMUMeasurements imu = {0};
    Uav uav = {0};
    uav.basis = MatrixIdentity();
    static char line[1024] = { 0 };
    size_t parse_pos = 0;
    while (!WindowShouldClose()) {
        char c;
        while (read(fd, &c, 1) == 1) {
            if (c == '\n') {
                line[parse_pos] = '\0';
                IMUMeasurements read = {0}; 
                if (sscanf(line, "%f, %f, %f, %f, %f, %f",
                           &read.acceleration.x, &read.acceleration.y, &read.acceleration.z,
                           &read.rotation_rate.roll, &read.rotation_rate.pitch, &read.rotation_rate.yaw
                    ) == 6) {
                    read.rotation_rate.pitch *= DEG2RAD;
                    read.rotation_rate.yaw *= DEG2RAD;
                    read.rotation_rate.roll *= DEG2RAD;
                    imu = read; 

                }
                parse_pos = 0;
            }
            else if (c != '\r' && parse_pos <= sizeof(line) - 2) {
                line[parse_pos] = c;
                parse_pos++;
            }
        }

        if (IsKeyDown(KEY_R)) {
            uav.angle = (EulerAngle) { 0 };
            uav.x = Vector3Zero();
            uav.basis = MatrixIdentity();
        }
        if (IsKeyDown(KEY_Z)) {
            camera.target = (Vector3){0, 0, 0};
            camera.up = (Vector3){0, 1, 0};
        }

        if (IsKeyPressed(KEY_ESCAPE) && IsCursorHidden())
            EnableCursor();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !IsCursorHidden())
            DisableCursor();

        float dt = GetFrameTime();
        
        // TODO: do not multiply by - manually
        uav.angle.roll  = fmodf(uav.angle.roll + dt * imu.rotation_rate.roll, 2*PI);
        uav.angle.pitch = fmodf(uav.angle.pitch + (-dt) * imu.rotation_rate.pitch, 2*PI);
        uav.angle.yaw   = fmodf(uav.angle.yaw + (-dt) * imu.rotation_rate.yaw, 2*PI);
        uav.basis = MatrixRotateXYZ((Vector3){uav.angle.roll, uav.angle.pitch, uav.angle.yaw}); 


        UpdateCamera(&camera, CAMERA_FREE);
        BeginDrawing();
        BeginMode3D(camera);
        {
            ClearBackground(BLACK);

            DrawGrid(30, 10);


            rlPushMatrix();
            {
                //  North-East-Down (NED) system
                Matrix mat = MatrixRotateXYZ((Vector3) {DEG2RAD*90, 0, 0});
                rlLoadIdentity();
                rlMultMatrixf(MatrixToFloat(mat));

                // This makes UAV point towards positive X initially. 
                Matrix model_offset = MatrixRotateXYZ((Vector3) {-PI/2, PI/2, 0});
                
                model.transform = MatrixMultiply(model_offset, uav.basis);
                DrawModel(model, uav.x, 1.0, WHITE);

                draw_basis(uav.x, uav.basis);

            }
            rlPopMatrix();

        }
        EndMode3D();
        draw_ui(uav, font, font_size);
        EndDrawing();
    }

    CloseWindow();
    if (close(fd) == -1) { fprintf(stderr, "Error: could not close serial port: %s\n", strerror(errno)); return -1; }; 
} 
