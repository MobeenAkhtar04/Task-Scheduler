#include "worker_pool.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

WorkerPool* pool_ptr = nullptr;

void handle_signal(int sig) {
    std::cout << "[RUNTIME] Caught signal " << sig << ", shutting down\n";
    if (pool_ptr) pool_ptr->stop();
}

int main(int argc, char* argv[]) {
    int port = 9000;
    int num_threads = 4;

    if (argc >= 2) port = std::atoi(argv[1]);
    if (argc >= 3) num_threads = std::atoi(argv[2]);

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::cout << "[RUNTIME] Starting on port " << port
              << " with " << num_threads << " threads\n";

    WorkerPool pool(num_threads, port);
    pool_ptr = &pool;
    pool.run();

    return 0;
}
