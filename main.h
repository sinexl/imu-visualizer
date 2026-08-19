#ifndef MAIN_H_
#define MAIN_H_

#include "raylib.h"
typedef struct {
    float roll, pitch, yaw;
} EulerAngle;

typedef struct {
    Vector3 acceleration;
    EulerAngle rotation_rate;
} IMUMeasurements;

typedef struct {
    Vector3 x; // position
    Vector3 u; // speed
    Matrix basis;
    EulerAngle angle;
} Uav;

Uav uav_new();
void uav_reset(Uav*);

#endif // MAIN_H_
