#pragma once

template<typename T>
class Vector2D
{
    public:
        Vector2D() : x(T{}), y(T{}) {}
        Vector2D(T x, T y) : x(x), y(y) {}

        Vector2D operator+(const Vector2D& other) const
        {
            return { x + other.x, y + other.y };
        }

        Vector2D operator-(const Vector2D& other) const
        {
            return { x - other.x, y - other.y };
        }

        Vector2D operator*(T scalar) const
        {
            return { x * scalar, y * scalar };
        }

        Vector2D& operator+=(const Vector2D& other)
        {
            x += other.x;
            y += other.y;
            return *this;
        }

        T x;
        T y;
};
