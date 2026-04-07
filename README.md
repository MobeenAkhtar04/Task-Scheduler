# Multi-Threaded Task Scheduler

Distributed task scheduler built with a Python coordinator and a C++ worker runtime. Jobs get queued, dispatched round-robin across 4 workers, and processed concurrently using a thread pool. There's also a live web dashboard that shows everything happening in real time over WebSocket.

## How it works

The coordinator keeps a job queue and dispatches jobs to workers one at a time using round-robin. If a job fails it retries up to 3 times with exponential backoff (2s, 4s, 8s). Each worker is a C++ TCP server with a thread pool — threads sit on a condition variable waiting for jobs to come in.

```
coordinator (Python)
    |
    ├── worker1 (C++ thread pool)
    ├── worker2 (C++ thread pool)
    ├── worker3 (C++ thread pool)
    └── worker4 (C++ thread pool)
```

The coordinator also runs a WebSocket server on port 8765. Every time a job is enqueued, dispatched, completed, or fails, it broadcasts the event as JSON. The dashboard connects to that and shows everything live.

## Running

```bash
bash start.sh
```

Builds the Docker containers, waits for the WebSocket server to come up on port 8765, then opens the dashboard automatically in your browser. Press Ctrl+C to stop everything.

## Running locally (no Docker)

You'd need to start workers and the coordinator separately. The coordinator expects hostnames `worker1`–`worker4` so you'd have to change `NODES` in `coordinator/scheduler.py` to point at localhost with whatever ports you're using.

```bash
# build and run a worker
cd runtime
make
./worker 9000 4

# run the coordinator (in another terminal)
pip install websockets
python coordinator/scheduler.py
```

Then open `dashboard/index.html` in your browser.

## Project structure

```
├── start.sh                     # run this to start everything
├── dashboard/
│   └── index.html               # live dashboard, vanilla JS
├── coordinator/
│   └── scheduler.py             # coordinator + WebSocket server
├── runtime/
│   ├── main.cpp                 # entry point
│   ├── task.h / task.cpp        # Task base class, ProcessingTask
│   ├── worker.h / worker.cpp    # Worker class (one thread each)
│   ├── worker_pool.h / .cpp     # WorkerPool, owns the queue and workers
│   └── Makefile
└── docker/
    ├── Dockerfile.coordinator
    ├── Dockerfile.worker
    └── docker-compose.yml
```

## Ports

| container   | port | what it's for         |
|-------------|------|-----------------------|
| coordinator | 8765 | WebSocket (dashboard) |
| worker1     | 9001 | TCP jobs              |
| worker2     | 9002 | TCP jobs              |
| worker3     | 9003 | TCP jobs              |
| worker4     | 9004 | TCP jobs              |
# Task-Scheduler
