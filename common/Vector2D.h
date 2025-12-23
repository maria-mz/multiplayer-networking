#ifndef VECTOR_H_
#define VECTOR_H_

template<typename T>
class Vector2D
{
    public:
        Vector2D() : x(T{}), y(T{}) {}
        Vector2D(T x, T y) : x(x), y(y) {}

        T x;
        T y;
};

#endif