#include "worker.h"

#include <iostream>

Worker::Worker(int id,
               std::queue<std::unique_ptr<Task>>& queue,
               std::mutex& mutex,
               std::condition_variable& cv,
               std::atomic<bool>& running)
    : id_(id), queue_(queue), mutex_(mutex), cv_(cv), running_(running) {}

Worker::~Worker() {
    stop();
}

void Worker::start() {
    thread_ = std::thread(&Worker::run, this);
    std::cout << "[Worker " << id_ << "] Started\n";
}

void Worker::stop() {
    if (thread_.joinable()) thread_.join();
}

void Worker::run() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        // wait until there's something in the queue or we're shutting down
        cv_.wait(lock, [this] { return !queue_.empty() || !running_; });

        if (!running_ && queue_.empty()) break;

        std::unique_ptr<Task> task = std::move(queue_.front());
        queue_.pop();
        lock.unlock();

        // TODO: maybe track how long each job takes here
        std::cout << "[Worker " << id_ << "] Executing " << task->job_id() << "\n";
        task->execute();
        std::cout << "[Worker " << id_ << "] Done " << task->job_id() << "\n";
    }
    std::cout << "[Worker " << id_ << "] Exiting\n";
}
