#include <gtest/gtest.h>

#include "common/thread_safe_queue.h"


TEST(ThreadSafeQueueTest, BasicFIFO)
{
    ThreadSafeQueue<int> tsQueue;

    tsQueue.push(1);
    tsQueue.push(2);
    tsQueue.push(3);

    ASSERT_EQ(tsQueue.pop(), 1);
    ASSERT_EQ(tsQueue.pop(), 2);
    ASSERT_EQ(tsQueue.pop(), 3);
}
