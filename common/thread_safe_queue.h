#pragma once

#include <deque>
#include <optional>

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
