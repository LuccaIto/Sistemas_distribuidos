import os
import threading
import time
import zmq
from common import pack, unpack, now

PORT = int(os.getenv("REF_PORT", "5560"))
TIMEOUT = int(os.getenv("SERVER_TIMEOUT", "35"))
servers = {}
lock = threading.Lock()
ctx = zmq.Context()
sock = ctx.socket(zmq.REP)
sock.bind(f"tcp://0.0.0.0:{PORT}")
print(f"[reference] ouvindo na porta {PORT}", flush=True)

def cleanup():
    while True:
        time.sleep(5)
        cutoff = time.time() - TIMEOUT
        with lock:
            expired = [name for name, info in servers.items()
                       if info["last_seen"] < cutoff]
            for name in expired:
                print(f"[reference] {name} removido por timeout", flush=True)
                del servers[name]

threading.Thread(target=cleanup, daemon=True).start()

while True:
    try:
        message = unpack(sock.recv())
        action = message.get("action")
        name = message.get("name")
        with lock:
            if action == "register":
                if not name:
                    reply = {"ok": False, "error": "nome do servidor obrigatório"}
                else:
                    if name not in servers:
                        next_rank = max((item["rank"] for item in servers.values()), default=0) + 1
                        servers[name] = {
                            "rank": next_rank,
                            "peer_port": int(message.get("peer_port", 5651)),
                            "last_seen": time.time()
                        }
                    else:
                        servers[name]["last_seen"] = time.time()
                        servers[name]["peer_port"] = int(message.get("peer_port", servers[name]["peer_port"]))
                    reply = {"ok": True, "rank": servers[name]["rank"]}
            elif action == "heartbeat":
                if name in servers:
                    servers[name]["last_seen"] = time.time()
                    reply = {"ok": True}
                else:
                    reply = {"ok": False, "error": "servidor não cadastrado"}
            elif action == "list":
                reply = {"ok": True, "servers": [
                    {"name": server_name, "rank": info["rank"],
                     "peer_port": info["peer_port"]}
                    for server_name, info in servers.items()
                ]}
            else:
                reply = {"ok": False, "error": "ação desconhecida"}
        reply["timestamp"] = now()
        sock.send(pack(reply))
    except Exception as exc:
        print(f"[reference] erro: {exc}", flush=True)
        sock.send(pack({"ok": False, "error": str(exc), "timestamp": now()}))
