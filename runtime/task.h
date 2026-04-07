#pragma once

#include <string>

// Abstract base class for all executable tasks.
class Task {
public:
    virtual ~Task() = default;
    virtual void execute() = 0;
    virtual const std::string& job_id() const = 0;
};

// Concrete task: simulates processing a job payload.
class ProcessingTask : public Task {
public:
    ProcessingTask(std::string job_id, std::string payload);

    void execute() override;
    const std::string& job_id() const override;

private:
    std::string job_id_;
    std::string payload_;
};
