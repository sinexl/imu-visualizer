#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>
#include "raylib.h"
#include "raymath.h"

#define WIDTH 800
#define HEIGHT 600

typedef struct {
    float roll, pitch, yaw;
} EulerAngle;

    
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

void draw_arrow(Vector3 start_pos, Vector3 direction, float len, float thickness, Color color)  {
    assert(fabs(Vector3Length(direction) - 1) <= 1e-2);
    Vector3 end_pos = Vector3Add(start_pos, Vector3Scale(direction, len));
    DrawCylinderEx(start_pos, end_pos, thickness, thickness, 100, color);

    Vector3 arrow_displacement = Vector3Scale(direction, 10);

    DrawCylinderEx(Vector3Add(end_pos, arrow_displacement), end_pos, 1, thickness * 2, 100, color);
}

int main() { 
    const char* path = "/dev/ttyACM0";

    int fd;
    if ((fd = open_serial(path)) == -1) return -1; 

    InitWindow(WIDTH, HEIGHT, "IMU Visualizer");
    SetTargetFPS(60);
    Camera camera = {0};

    camera.position   = (Vector3) { 0, 120, -120 };
    camera.target     = (Vector3) { 0, 0, 0 };
    camera.up         = (Vector3) { 0, 1, 0 };
    camera.fovy       = 80;
    camera.projection = CAMERA_PERSPECTIVE;

    Model uav = LoadModel("./resources/plane.obj");
    Texture2D texture = LoadTexture("./resources/plane_diffuse.png");
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    uav.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    DisableCursor();

    EulerAngle rate = {0}; 
    EulerAngle angle = {0}; 
    static char line[128] = { 0 };
    while (!WindowShouldClose()) {
        size_t parse_pos = 0;
        char c;
        while (read(fd, &c, 1) == 1) {
            if (c == '\n') {
                line[parse_pos] = '\0';
                EulerAngle read = {0};
                if (sscanf(line, "%f, %f, %f", &read.roll, &read.pitch, &read.yaw) == 3) {
                    read.pitch *= DEG2RAD;
                    read.yaw *= DEG2RAD;
                    read.roll *= DEG2RAD;
                    rate = read; 
                }
                parse_pos = 0;
            }
            else if (c != '\r' && parse_pos <= sizeof(line) - 2) {
                line[parse_pos] = c;
                parse_pos++;
            }
        }

        printf("%f, %f, %f\n", rate.roll, rate.pitch, rate.yaw);
        float dt = GetFrameTime();
        
        angle.roll += dt * rate.roll;
        angle.yaw += dt * rate.yaw;
        angle.pitch += dt * rate.pitch;

        uav.transform = MatrixRotateXYZ((Vector3){angle.roll, angle.pitch, angle.yaw});
        
        UpdateCamera(&camera, CAMERA_FREE);
        
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);


            
            DrawGrid(20, 10);


            DrawModel(uav, Vector3Zero(), 1.0, WHITE);
            draw_arrow(Vector3Zero(), (Vector3) {1.0, 0,   0  }, 50, 2, RED);
            draw_arrow(Vector3Zero(), (Vector3) {0,   1.0, 0  }, 50, 2, GREEN);
            draw_arrow(Vector3Zero(), (Vector3) {0,   0,   1.0}, 50, 2, BLUE);

            EndMode3D();
        }
        EndDrawing();
    }

    CloseWindow();
    if (close(fd) == -1) fprintf(stderr, "Error: could not close serial port: %s\n", strerror(errno));
} 
