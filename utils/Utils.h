#ifndef UTILS_H_
#define UTILS_H_

#include <thread>
#include <chrono>

inline void sleepMs(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

#endif