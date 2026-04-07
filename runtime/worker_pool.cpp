#include "worker_pool.h"

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

WorkerPool::WorkerPool(int num_workers, int port)
    : port_(port), server_fd_(-1), running_(false) {
    for (int i = 0; i < num_workers; i++) {
        workers_.push_back(
            std::make_unique<Worker>(i, task_queue_, queue_mutex_, cv_, running_));
    }
}

WorkerPool::~WorkerPool() {
    stop();
}

void WorkerPool::run() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[WorkerPool] Failed to create socket\n";
        return;
    }

    // without SO_REUSEADDR you get "address already in use" on restart
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port_);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[WorkerPool] Bind failed on port " << port_ << "\n";
        return;
    }

    listen(server_fd_, 16);
    running_ = true;

    for (auto& worker : workers_) {
        worker->start();
    }

    std::cout << "[WorkerPool] Listening on port " << port_ << "\n";
    accept_loop();
}

void WorkerPool::stop() {
    running_ = false;
    cv_.notify_all();
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    for (auto& worker : workers_) {
        worker->stop();
    }
    std::cout << "[WorkerPool] Stopped\n";
}

void WorkerPool::accept_loop() {
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &len);
        if (client_fd < 0) {
            if (!running_) break;
            continue;
        }

        char buf[4096] = {};
        int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { close(client_fd); continue; }

        auto task = parse_task(std::string(buf, n));

        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.push(std::move(task));
        }
        cv_.notify_one();

        // send ack right away - coordinator is blocking on this
        // actual processing happens async in a worker thread
        std::string ack = "{\"status\":\"ok\"}";
        send(client_fd, ack.c_str(), ack.size(), 0);
        close(client_fd);
    }
}

// basic string parsing - could use a proper json lib but this works fine
std::unique_ptr<Task> WorkerPool::parse_task(const std::string& raw) {
    std::string job_id, payload;

    auto id_pos = raw.find("\"job_id\"");
    if (id_pos != std::string::npos) {
        auto start = raw.find('"', id_pos + 8) + 1;
        auto end   = raw.find('"', start);
        job_id = raw.substr(start, end - start);
    }

    auto pl_pos = raw.find("\"payload\"");
    if (pl_pos != std::string::npos) {
        auto start = raw.find('{', pl_pos);
        auto end   = raw.find('}', start) + 1;
        payload = raw.substr(start, end - start);
    }

    return std::make_unique<ProcessingTask>(std::move(job_id), std::move(payload));
}
