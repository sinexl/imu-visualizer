#ifndef UTIL_H_
#define UTIL_H_

#include "euler_angle.hpp"
#include <imgui.h>
#include <raylib.h>
// in m/s^2
#define GRAVITATIONAL_ACCELERATION 9.80665f

// Raylib does this in a dumb way: Instead of A*x, they define x*A which yields the same result mathematically makes no sense
Vector3 operator*(Matrix A, Vector3 x);
ImVec2 operator-(ImVec2 a, ImVec2 b);

// These functions return true if ANY element of structure is NaN.
bool isnan(Vector3 v);
bool isnan(EulerAngle e);
bool isnan(Matrix m);

void print_matrix(Matrix x);

#define decomp(v) (v).x, (v).y, (v).z

#endif // UTIL_H_
