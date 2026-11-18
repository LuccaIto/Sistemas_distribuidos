import os
import random
import threading
import time
import zmq
from common import pack, unpack, now

NAME = os.getenv("BOT_NAME", "bot")
HOST = os.getenv("SERVER_HOST", "server1")
PORT = int(os.getenv("SERVER_PORT", "5555"))
SUB_HOST = os.getenv("SUB_HOST", "broker")

ctx = zmq.Context()
req = ctx.socket(zmq.REQ)
req.connect(f"tcp://{HOST}:{PORT}")
logical_clock = 0

def call(action, **kwargs):
    """Envia uma requisição e aguarda exatamente uma resposta (REQ/REP)."""
    global logical_clock
    logical_clock += 1
    payload = {
        "action": action, "user": NAME, "timestamp": now(),
        "logical_clock": logical_clock, **kwargs
    }
    req.send(pack(payload))
    answer = unpack(req.recv())
    logical_clock = max(logical_clock, int(answer.get("logical_clock", 0)))
    print(f"[{NAME}] {action}: {answer}", flush=True)
    return answer

def listen():
    sub = ctx.socket(zmq.SUB)
    sub.connect(f"tcp://{SUB_HOST}:5558")
    sub.setsockopt(zmq.SUBSCRIBE, b"servers")
    # Os canais selecionados são inscritos pelo socket principal em subscribe_channels.
    while True:
        try:
            topic, raw = sub.recv_multipart()
            item = unpack(raw)
            print(
                f"[{NAME}] tópico={topic.decode(errors='replace')} "
                f"mensagem={item} recebida_em={now()}",
                flush=True
            )
        except Exception as exc:
            print(f"[{NAME}] erro Pub/Sub: {exc}", flush=True)

def main():
    threading.Thread(target=listen, daemon=True).start()
    time.sleep(2)
    while not call("login").get("ok"):
        time.sleep(2)

    channels = call("list_channels").get("channels", [])
    if len(channels) < 5:
        call("create_channel", channel=f"canal_{NAME}")
        channels = call("list_channels").get("channels", [])

    # Mantém até três inscrições, conforme o roteiro dos bots.
    # O bot assina os canais escolhidos. As publicações de "servers" são
    # recebidas pelo listener, e os canais são inscritos em um socket dedicado.
    channel_sub = ctx.socket(zmq.SUB)
    channel_sub.connect(f"tcp://{SUB_HOST}:5558")
    for channel in channels[:3]:
        channel_sub.setsockopt(zmq.SUBSCRIBE, channel.encode())
        print(f"[{NAME}] inscrito em {channel}", flush=True)
    def listen_channels():
        while True:
            try:
                topic, raw = channel_sub.recv_multipart()
                print(f"[{NAME}] canal={topic.decode()} mensagem={unpack(raw)} recebida_em={now()}", flush=True)
            except Exception as exc:
                print(f"[{NAME}] erro canal: {exc}", flush=True)
    threading.Thread(target=listen_channels, daemon=True).start()

    while True:
        channels = call("list_channels").get("channels", [])
        if not channels:
            time.sleep(1)
            continue
        channel = random.choice(channels)
        for i in range(10):
            call("publish", channel=channel, message=f"mensagem {i + 1} de {NAME}")
            time.sleep(1)

if __name__ == "__main__":
    main()
