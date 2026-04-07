#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "task.h"
#include "worker.h"

// Top-level manager. Owns the shared job queue, synchronisation primitives,
// and the pool of Worker objects. Also runs the TCP accept loop.
class WorkerPool {
public:
    WorkerPool(int num_workers, int port);
    ~WorkerPool();

    void run();   // bind socket, start workers, enter accept loop (blocks)
    void stop();  // signal shutdown and join everything

private:
    void accept_loop();
    std::unique_ptr<Task> parse_task(const std::string& raw);

    int port_;
    int server_fd_;
    std::atomic<bool> running_;

    // Shared state — references passed to each Worker.
    std::queue<std::unique_ptr<Task>> task_queue_;
    std::mutex                        queue_mutex_;
    std::condition_variable           cv_;

    std::vector<std::unique_ptr<Worker>> workers_;
};
