#ifndef EULER_ANGLE_H_
#define EULER_ANGLE_H_

#include <math.h>
struct EulerAngle {
    float roll, pitch, yaw;

    EulerAngle(float roll, float pitch, float yaw);
    EulerAngle();
};

float mod(float a, float b);

EulerAngle operator+(EulerAngle a, EulerAngle b);
EulerAngle operator*(EulerAngle a, float scalar);
EulerAngle operator*(float scalar, EulerAngle a);

#ifdef EULER_ANGLE_IMPLEMENTATION

EulerAngle::EulerAngle(float roll, float pitch, float yaw)
    : roll(mod(roll, 2*M_PI)), pitch(mod(pitch, 2*M_PI)), yaw(mod(yaw, 2*M_PI))
{ }
EulerAngle::EulerAngle() : EulerAngle(0, 0, 0) {}

EulerAngle operator+(EulerAngle a, EulerAngle b) {
    // mod is done by constructor
    return EulerAngle(a.roll  + b.roll,
                      a.pitch + b.pitch,
                      a.yaw   + b.yaw); 
}
EulerAngle operator*(EulerAngle a, float scalar) {
    return EulerAngle(a.roll  * scalar,
                      a.pitch * scalar,
                      a.yaw   * scalar);
}
EulerAngle operator*(float scalar, EulerAngle a) {
    return a * scalar;
}


float mod(float x, float m) { 
    return fmodf(x, m);
}
#endif // EULER_ANGLE_IMPLEMENTATION


#endif // EULER_ANGLE_H_
