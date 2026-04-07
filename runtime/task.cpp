#include "task.h"

#include <chrono>
#include <cstdlib>
#include <thread>

ProcessingTask::ProcessingTask(std::string job_id, std::string payload)
    : job_id_(std::move(job_id)), payload_(std::move(payload)) {}

const std::string& ProcessingTask::job_id() const {
    return job_id_;
}

void ProcessingTask::execute() {
    // simulate doing actual work - just sleeping for now
    // TODO: replace with real processing logic based on payload type
    std::this_thread::sleep_for(std::chrono::milliseconds(200 + rand() % 300));
}
