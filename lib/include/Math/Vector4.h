#ifndef __VECTOR4_H_
#define __VECTOR4_H_

#include "Vector2.h"

class Vector4
{
public:
    Vector4();
    Vector4(double x, double y, double z, double w);
    Vector4(Vector2 xy, Vector2 zw);

    double  Cross(Vector4 other);
    double  Dot(Vector4 v);
    Vector4 Lerp(Vector4 v, float alpha);

    Vector4 operator+(Vector4 const &vector);
    Vector4 operator-(Vector4 const &vector);
    Vector4 operator*(Vector4 const &vector);
    Vector4 operator/(Vector4 const &vector);
    Vector4 operator*(double const &number);
    Vector4 operator/(double const &number);
    bool    operator==(Vector4 const &vector);
    bool    operator!=(Vector4 const &vector);

    Vector2 XY, ZW;
};
#endif