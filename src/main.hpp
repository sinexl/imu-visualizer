#ifndef MAIN_H_
#define MAIN_H_

#include "euler_angle.hpp"
#include "raylib.h"

typedef struct {
    // Translational acceleration with respect to IMU's local axes.
    Vector3 acceleration;
    // Angular velocity with respect to IMU's local axes.
    Vector3 angular_velocity;
} IMUMeasurements;

typedef struct {
    Vector3 x; // position
    Vector3 u; // speed
    Matrix basis;
    EulerAngle angle;
} Uav;

#define decomp(v) (v).x, (v).y, (v).z

Uav uav_new();
void uav_reset(Uav*);

#endif // MAIN_H_
