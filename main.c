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
    float x, y, z;
} Data;
    
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

    size_t pos = 0;
    static char line[128] = {0};
    Data recent; 
    while (!WindowShouldClose()) {
        char c;
        while (read(fd, &c, 1) == 1) {
            if (c == '\n') {
                line[pos] = '\0';
                Data read = {0};
                if (sscanf(line, "%f, %f, %f", &read.x, &read.y, &read.z) == 3) {
                    recent = read; 
                }
                pos = 0;
            }
            else if (c != '\r' && pos <= sizeof(line) - 2) {
                line[pos] = c;
                pos++;
            }
        }

        printf("%f, %f, %f\n", recent.x, recent.y, recent.z);
        UpdateCamera(&camera, CAMERA_FREE);
        
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

            DrawModel(uav, Vector3Zero(), 1.0, WHITE);

            EndMode3D();
        }
        EndDrawing();
    }

    CloseWindow();
    if (close(fd) == -1) fprintf(stderr, "Error: could not close serial port: %s\n", strerror(errno));
} 
