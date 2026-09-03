#include "uav.hpp"
#include "raymath.h"
#include "util.hpp"
#include <cassert>

EulerAngle angular_velocity_to_euler_rates(Vector3 angular_velocity, EulerAngle uav_angle) {
    float th = uav_angle.pitch;
    float phi = uav_angle.roll;

    // Since IMU measures angular velocity against it's own axes, which are dependent on UAV orientation (Euler angles),
    // the following transformation should be applied to get the euler angle rates from angular velocity
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
    // print_matrix(B);
    Vector3 euler =  B*angular_velocity;
    // Now euler contains [psi, th, phi], thus swapping is required.
    return {
        euler.z, // roll
        euler.y, // pitch
        euler.x  // yaw
    };
}
MotionData Uav::update_angle(IMUMeasurements imu_ned, float filter_alpha, float dt) {

    EulerAngle euler_rates = angular_velocity_to_euler_rates(imu_ned.angular_velocity, angle);

    EulerAngle gyroscope_estimate = angle + euler_rates * dt;

    EulerAngle accelerometer_estimate = {
        atan2f(imu_ned.acceleration.y, imu_ned.acceleration.z),
        atan2f(imu_ned.acceleration.x, sqrtf(powf(imu_ned.acceleration.y, 2) + powf(imu_ned.acceleration.z, 2))),
        gyroscope_estimate.yaw, // Yaw could not be estimated from accelerometer data, since
                                // Yawing doesn't affect gravity
    };

    
    // Apply complementary filter: angle(t + 1) = a * (angle(t) + euler_rates(t)*dt) + (1 - a)*(accelerometer_estimate)
    angle = filter_alpha*gyroscope_estimate + (1 - filter_alpha)*accelerometer_estimate;

    return {
        .gyroscope = gyroscope_estimate,
        .accelerometer = accelerometer_estimate
    };
}

Uav::Uav(): x(Vector3Zero()), u(Vector3Zero()), basis(MatrixIdentity()), angle() {}

void Uav::reset() {
    x = u = Vector3Zero();
    angle = EulerAngle{};
    basis = MatrixIdentity();
}
