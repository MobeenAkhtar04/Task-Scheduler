import socket
import json
import time
import threading
import queue
import logging
import asyncio
import websockets

logging.basicConfig(level=logging.INFO, format='%(asctime)s [COORDINATOR] %(message)s')
log = logging.getLogger(__name__)

NODES = [
    {"host": "worker1", "port": 9000},
    {"host": "worker2", "port": 9000},
    {"host": "worker3", "port": 9000},
    {"host": "worker4", "port": 9000},
]

# --- WebSocket broadcast machinery ---

ws_clients = set()
ws_loop = None

def broadcast_event(event: dict):
    """Thread-safe: schedule a broadcast onto the ws event loop."""
    if ws_loop is not None and ws_loop.is_running():
        asyncio.run_coroutine_threadsafe(_broadcast(json.dumps(event)), ws_loop)

async def _broadcast(message: str):
    targets = list(ws_clients)
    if targets:
        await asyncio.gather(*[c.send(message) for c in targets], return_exceptions=True)

async def ws_handler(websocket):
    ws_clients.add(websocket)
    log.info("Dashboard client connected")
    try:
        await websocket.wait_closed()
    finally:
        ws_clients.discard(websocket)
        log.info("Dashboard client disconnected")

async def _run_ws_server():
    async with websockets.serve(ws_handler, "0.0.0.0", 8765):
        log.info("WebSocket server listening on port 8765")
        await asyncio.Future()  # run forever

def start_ws_server():
    global ws_loop
    ws_loop = asyncio.new_event_loop()
    asyncio.set_event_loop(ws_loop)
    ws_loop.run_until_complete(_run_ws_server())

# --- Scheduler ---

class Job:
    def __init__(self, job_id, payload, retries=0):
        self.job_id = job_id
        self.payload = payload
        self.retries = retries
        self.max_retries = 3

class Coordinator:
    def __init__(self):
        self.job_queue = queue.Queue()
        self.node_index = 0
        self.lock = threading.Lock()
        self.running = True

    def enqueue(self, job_id, payload):
        job = Job(job_id, payload)
        self.job_queue.put(job)
        log.info(f"Enqueued {job_id}")
        broadcast_event({"event": "enqueued", "job_id": job_id, "payload": payload})

    def next_node(self):
        with self.lock:
            node = NODES[self.node_index % len(NODES)]
            self.node_index += 1
            return node

    def send_job(self, job, node):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(5)
            s.connect((node["host"], node["port"]))
            msg = json.dumps({"job_id": job.job_id, "payload": job.payload})
            s.sendall(msg.encode())
            response = s.recv(1024).decode()
            s.close()
            data = json.loads(response)
            if data.get("status") == "ok":
                log.info(f"Job {job.job_id} completed on {node['host']}")
                broadcast_event({"event": "completed", "job_id": job.job_id, "worker": node["host"]})
                return True
            else:
                log.warning(f"Job {job.job_id} failed on {node['host']}: {data.get('error')}")
                broadcast_event({"event": "failed", "job_id": job.job_id, "worker": node["host"], "error": data.get("error")})
                return False
        except Exception as e:
            log.error(f"Job {job.job_id} error on {node['host']}: {e}")
            broadcast_event({"event": "failed", "job_id": job.job_id, "worker": node["host"], "error": str(e)})
            return False

    def dispatch(self, job):
        node = self.next_node()
        log.info(f"Dispatching {job.job_id} → {node['host']}")
        broadcast_event({"event": "dispatched", "job_id": job.job_id, "worker": node["host"]})
        success = self.send_job(job, node)
        if not success:
            if job.retries < job.max_retries:
                job.retries += 1
                delay = 2 ** job.retries
                log.warning(f"Retrying {job.job_id} in {delay}s (attempt {job.retries}/{job.max_retries})")
                broadcast_event({"event": "retry", "job_id": job.job_id, "attempt": job.retries, "delay": delay})
                time.sleep(delay)
                self.job_queue.put(job)
            else:
                log.error(f"Job {job.job_id} exceeded max retries, dropping")
                broadcast_event({"event": "dropped", "job_id": job.job_id})

    def run(self):
        log.info("Coordinator started")
        threads = []
        while self.running:
            try:
                job = self.job_queue.get(timeout=1)
                t = threading.Thread(target=self.dispatch, args=(job,))
                t.start()
                threads.append(t)
            except queue.Empty:
                continue

        for t in threads:
            t.join()

if __name__ == "__main__":
    # Start WebSocket server in background thread
    ws_thread = threading.Thread(target=start_ws_server, daemon=True)
    ws_thread.start()
    time.sleep(0.5)  # give the loop a moment to start

    c = Coordinator()

    def load_jobs():
        i = 0
        while True:
            c.enqueue(f"JOB-{i:04d}", {"task": "process", "data": f"chunk_{i}"})
            i += 1
            time.sleep(1)

    loader = threading.Thread(target=load_jobs)
    loader.start()

    try:
        c.run()
    except KeyboardInterrupt:
        c.running = False
        log.info("Shutting down")
