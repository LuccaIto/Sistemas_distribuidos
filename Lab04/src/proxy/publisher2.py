import zmq
import random
from time import sleep

TOPIC = "dice"

context = zmq.Context()
pub = context.socket(zmq.PUB)
pub.connect("tcp://proxy:5555")

while True:
    message = str(random.randint(1, 6))
    print(f"[{TOPIC}] message: {message}", flush=True)
    pub.send_string(f"{TOPIC} {message}")
    sleep(1)

pub.close()
context.close()
