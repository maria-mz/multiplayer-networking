#ifndef TS_QUEUE_H_
#define TS_QUEUE_H_

#include <deque>


template <typename T>
class TSQueue
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

#endif