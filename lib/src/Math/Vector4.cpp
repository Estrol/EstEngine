#include <Math/Vector4.h>

Vector4::Vector4()
{
    XY = Vector2();
    ZW = Vector2();
}

Vector4::Vector4(double x, double y, double z, double w)
{
    XY = Vector2(x, y);
    ZW = Vector2(z, w);
}

Vector4::Vector4(Vector2 xy, Vector2 zw)
{
    XY = xy;
    ZW = zw;
}

double Vector4::Cross(Vector4 other)
{
    return this->XY.Cross(other.XY) + this->ZW.Cross(other.ZW);
}

double Vector4::Dot(Vector4 v)
{
    return this->XY.Dot(v.XY) + this->ZW.Dot(v.ZW);
}

Vector4 Vector4::Lerp(Vector4 destination, float alpha)
{
    return (*this) * (1.0 - alpha) + (destination * alpha);
}

Vector4 Vector4::operator+(Vector4 const &vector)
{
    return { this->XY + vector.XY, this->ZW + vector.ZW };
}

Vector4 Vector4::operator-(Vector4 const &vector)
{
    return { this->XY - vector.XY, this->ZW - vector.ZW };
}

Vector4 Vector4::operator*(Vector4 const &vector)
{
    return { this->XY * vector.XY, this->ZW * vector.ZW };
}

Vector4 Vector4::operator/(Vector4 const &vector)
{
    return { this->XY / vector.XY, this->ZW / vector.ZW };
}

Vector4 Vector4::operator*(double const &number)
{
    return { this->XY * number, this->ZW * number };
}

Vector4 Vector4::operator/(double const &number)
{
    return { this->XY / number, this->ZW / number };
}

bool Vector4::operator==(Vector4 const &vector)
{
    return this->XY == vector.XY && this->ZW == vector.ZW;
}

bool Vector4::operator!=(Vector4 const &vector)
{
    return this->XY != vector.XY || this->ZW != vector.ZW;
}