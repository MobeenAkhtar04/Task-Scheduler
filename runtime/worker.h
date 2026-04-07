#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "task.h"

// Encapsulates a single worker thread. Shares the pool's queue, mutex,
// condition variable, and running flag by reference — owned by WorkerPool.
class Worker {
public:
    Worker(int id,
           std::queue<std::unique_ptr<Task>>& queue,
           std::mutex& mutex,
           std::condition_variable& cv,
           std::atomic<bool>& running);

    ~Worker();

    Worker(const Worker&)            = delete;
    Worker& operator=(const Worker&) = delete;
    Worker(Worker&&)                 = delete;
    Worker& operator=(Worker&&)      = delete;

    void start();
    void stop();   // blocks until the thread exits

private:
    void run();

    int                                  id_;
    std::thread                          thread_;
    std::queue<std::unique_ptr<Task>>&   queue_;
    std::mutex&                          mutex_;
    std::condition_variable&             cv_;
    std::atomic<bool>&                   running_;
};
