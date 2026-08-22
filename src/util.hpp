#ifndef UTIL_H_
#define UTIL_H_

#include <raylib.h>
#define GRAVITATIONAL_ACCELERATION 9.80665f

// Raylib does this in a dumb way: Instead of A*x, they define x*A which yields the same result mathematically makes no sense
Vector3 operator*(Matrix A, Vector3 x);
void print_matrix(Matrix x);

#define decomp(v) (v).x, (v).y, (v).z

#endif // UTIL_H_
