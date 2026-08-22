#ifndef UAV_H_
#define UAV_H_


#include "euler_angle.hpp"
#include <raylib.h>


typedef struct {
    // Translational acceleration with respect to IMU's local axes.
    Vector3 acceleration;
    // Angular velocity with respect to IMU's local axes.
    Vector3 angular_velocity;
} IMUMeasurements;


struct MotionData {
    EulerAngle gyroscope;
    EulerAngle accelerometer;
};

EulerAngle angular_velocity_to_euler_rates(Vector3 angular_velocity, EulerAngle uav_angle);

struct Uav{
    Vector3 x; // position
    Vector3 u; // speed
    Matrix basis;
    EulerAngle angle;

    Uav();
    MotionData update_angle(IMUMeasurements imu_ned, float dt);
    void reset();
};


#endif // UAV_H_
