#include "main.hpp"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>
#include "draw.hpp"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#define WIDTH 1600
#define HEIGHT 900

// in m/s^2
#define GRAVITATIONAL_ACCELERATION 9.80665f

// Raylib does this in a dumb way: Instead of A*x, they define x*A which yields the same result mathematically makes no sense
Vector3 operator*(Matrix A, Vector3 x) {
    return Vector3Transform(x, A);
}

    
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

#define RESOURCES_DIR "./resources/"

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

#define decomp(v) (v).x, (v).y, (v).z

void uav_reset(Uav* uav) {
    memset(uav, 0, sizeof(Uav));
    uav->basis = MatrixIdentity();
}
Uav uav_new() {
    Uav u;
    uav_reset(&u);
    return u;
}

void parse_serial_async(int fd, size_t* parse_pos, char* line, size_t line_size, IMUMeasurements* imu) {
    char c;
    while (read(fd, &c, 1) == 1) {
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

int main() { 
    const char* path = "/dev/ttyACM0";

    int fd;
    if ((fd = open_serial(path)) == -1) return -1; 

    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WIDTH, HEIGHT, "IMU Visualizer");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    Camera camera = {};

    const int font_size = 40;

    UpdateCamera(&camera, CAMERA_FREE);
    camera.position   = (Vector3) { -90, 90, 0 };
    camera.target     = (Vector3) { 0, 0, 0 };
    camera.up         = (Vector3) { 0, 1, 0 };
    camera.fovy       = 80;
    camera.projection = CAMERA_PERSPECTIVE;

    Font font = LoadFontEx(RESOURCES_DIR"/Inter-4.1/extras/ttf/Inter-Regular.ttf", font_size, NULL, 0);
    Model model = LoadModel(RESOURCES_DIR"/plane.obj");
    Texture2D texture = LoadTexture(RESOURCES_DIR"/plane_diffuse.png");
    SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    DisableCursor();

    IMUMeasurements imu = {};
    Uav uav = uav_new();
    
    static char line[512] = { 0 };
    size_t parse_pos = 0;
    while (!WindowShouldClose()) {
        parse_serial_async(fd, &parse_pos, line, sizeof(line), &imu);

        if (IsKeyDown(KEY_R)) {
            uav_reset(&uav);
            imu = (IMUMeasurements) {};
        }

        if (IsKeyDown(KEY_Z)) {
            camera.target = (Vector3){0, 0, 0};
            camera.up = (Vector3){0, 1, 0};
        }

        if (IsKeyPressed(KEY_ESCAPE) && IsCursorHidden())
            EnableCursor();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !IsCursorHidden())
            DisableCursor();
        UpdateCamera(&camera, CAMERA_FREE);

        float dt = GetFrameTime();
        
        // Transform data in NED
        // TODO: do not multiply by - manually
        /* imu.acceleration.y *= -1;  */
        /* imu.acceleration.z *= -1;  */
        imu.angular_velocity.y *= -1;
        imu.angular_velocity.z *= -1;

        float th = uav.angle.pitch;
        float phi = uav.angle.roll;
        
        // Since IMU measures angular velocity against it's own axes, which are dependent on UAV orientation (Euler angles),
        // the following transformation should be applied to get the euler angle rates:a
        // dEuler/dt = B(Euler) * omega
        // This is 3-2-1 kinematic transformation matrix
        Matrix B = {
            0,        sinf(phi),          cosf(phi),           0,
            0,        cosf(th)*cosf(phi), -cosf(th)*sinf(phi), 0, 
            cosf(th), sinf(th)*sinf(phi), sinf(th)*cosf(phi),  0,
            0,        0,                  0,                   1
        }; 
        assert(fabs(th - PI/2) >= 1e-3 && "TODO: Deal with gimbal lock."); 
        assert(fabs(th + PI/2) >= 1e-3 && "TODO: Deal with gimbal lock."); 
        B *= (1/cos(th));
        print_matrix(B);
        imu.angular_velocity = B*imu.angular_velocity;
        // Now imu.angular_velocity contains [psi, th, phi], thus swapping is required.
        EulerAngle euler_rates = {
            .roll = imu.angular_velocity.z,
            .pitch = imu.angular_velocity.y,
            .yaw = imu.angular_velocity.x
        }; 
        printf("Euler rates: %f %f %f\n", euler_rates.roll, euler_rates.pitch, euler_rates.yaw);

        
        uav.angle.roll  = fmodf(uav.angle.roll + dt * euler_rates.roll, 2*PI);
        uav.angle.pitch = fmodf(uav.angle.pitch + dt * euler_rates.pitch, 2*PI);
        uav.angle.yaw   = fmodf(uav.angle.yaw + dt * euler_rates.yaw, 2*PI);

        uav.basis = MatrixRotateZYX((Vector3){uav.angle.roll, uav.angle.pitch, uav.angle.yaw}); 

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
        {
            ClearBackground(BLACK);

            DrawGrid(30, 10);


            rlPushMatrix();
            {
                //  North-East-Down (NED) system
                Matrix mat = MatrixRotateZYX((Vector3) {DEG2RAD*90, 0, 0});
                rlLoadIdentity();
                rlMultMatrixf(MatrixToFloat(mat));

                // This makes UAV point towards positive X initially. 
                Matrix model_offset = MatrixRotateZYX((Vector3) {-PI/2, 0, -PI/2});
                
                model.transform = model_offset * uav.basis;
                DrawModel(model, uav.x, 1.0, WHITE);

                draw_basis(uav.x, uav.basis);
                draw_vector(uav.x, uav.basis*imu.angular_velocity*50, 1, ORANGE);

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
