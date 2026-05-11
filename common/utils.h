#pragma once

#include "SDL2/SDL.h"

#include <thread>
#include <chrono>
#include <deque>
#include <optional>

inline void sleepMs(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

inline bool iequals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (tolower(a[i]) != tolower(b[i]))
        {
            return false;
        }
    }
    return true;
}

inline bool hasIntersection(const SDL_Rect& a, const SDL_Rect& b)
{
    return a.x < b.x + b.w
        && a.x + a.w > b.x
        && a.y < b.y + b.h
        && a.y + a.h > b.y;
}

template <typename T>
class ThreadSafeQueue
{
    public:
        void push(T item)
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_deque.push_back(item);

            m_cv.notify_one();
        }

        T pop()
        {
            T item;

            std::unique_lock<std::mutex> lock(m_mtx);

            m_cv.wait(lock, [this]() { return !m_deque.empty(); });

            if (!m_deque.empty())
            {
                item = m_deque.front();
                m_deque.pop_front();
            }
            else
            {
                item = T{};
            }

            return item;
        }

        std::optional<T> tryPop()
        {
            T item;

            std::unique_lock<std::mutex> lock(m_mtx);

            if (!m_deque.empty())
            {
                item = m_deque.front();
                m_deque.pop_front();
                return item;
            }
            return std::nullopt;
        }

        bool front(T &item)
        {
            std::unique_lock<std::mutex> lock(m_mtx);
            if (!m_deque.empty())
            {
                item = m_deque.front();
                return true;
            }

            return false;
        }

        bool empty()
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            return m_deque.empty();
        }

        int size()
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            return m_deque.size();
        }

    private:
        std::deque<T> m_deque;
        std::mutex m_mtx;
        std::condition_variable m_cv;
};

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
