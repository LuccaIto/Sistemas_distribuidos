import zmq
from time import time, sleep

TOPIC = "time"

context = zmq.Context()
pub = context.socket(zmq.PUB)
pub.connect("tcp://proxy:5555")

while True:
    message = str(time())
    print(f"[{TOPIC}] message: {message}", flush=True)
    pub.send_string(f"{TOPIC} {message}")
    sleep(1)

pub.close()
context.close()
