#include "util.hpp"
#include <cstdio>
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>


// Raylib does this in a dumb way: Instead of A*x, they define x*A which yields the same result mathematically makes no sense
Vector3 operator*(Matrix A, Vector3 x) {
    return Vector3Transform(x, A);
}

ImVec2 operator-(ImVec2 a, ImVec2 b) {
    return {
        a.x - b.x,
        a.y - b.y
    }; 
}



void print_matrix(Matrix x) {
    printf("{\n");
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m0, x.m4, x.m8, x.m12);
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m1, x.m5, x.m9, x.m13);
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m2, x.m6, x.m10, x.m14);
    printf("\t%.3f  %.3f  %.3f  %.3f\n", x.m3, x.m7, x.m11, x.m15);
    printf("}\n");
}
