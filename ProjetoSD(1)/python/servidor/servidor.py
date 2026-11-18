import json
import os
import threading
import time
import zmq
from common import pack, unpack, now

NAME = os.getenv("SERVER_NAME", "server1")
CLIENT_PORT = int(os.getenv("CLIENT_PORT", "5555"))
PEER_PORT = int(os.getenv("PEER_PORT", "5651"))
REF_HOST = os.getenv("REF_HOST", "reference")
PUB_HOST = os.getenv("PUB_HOST", "broker")
REF = f"tcp://{REF_HOST}:5560"

DATA_DIR = "/data"
os.makedirs(DATA_DIR, exist_ok=True)
DATA_FILE = os.path.join(DATA_DIR, "dados.json")
data = {"users": [], "channels": ["geral"], "messages": []}
if os.path.exists(DATA_FILE):
    try:
        with open(DATA_FILE, encoding="utf-8") as file:
            saved = json.load(file)
            data.update(saved)
    except (OSError, json.JSONDecodeError) as exc:
        print(f"[{NAME}] não foi possível carregar os dados: {exc}", flush=True)

logical_clock = 0
physical_offset = 0.0
coordinator = None
rank = 0
message_count = 0
ctx = zmq.Context()
pub = ctx.socket(zmq.PUB)
pub.connect(f"tcp://{PUB_HOST}:5557")

def save():
    temporary = DATA_FILE + ".tmp"
    with open(temporary, "w", encoding="utf-8") as file:
        json.dump(data, file, ensure_ascii=False, indent=2)
    os.replace(temporary, DATA_FILE)

def physical_now():
    from datetime import datetime, timezone
    return datetime.fromtimestamp(time.time() + physical_offset, timezone.utc).isoformat()

def request(address, payload, timeout=2000):
    socket = ctx.socket(zmq.REQ)
    socket.setsockopt(zmq.RCVTIMEO, timeout)
    socket.setsockopt(zmq.SNDTIMEO, timeout)
    socket.connect(address)
    try:
        socket.send(pack({"timestamp": now(), **payload}))
        return unpack(socket.recv())
    finally:
        socket.close()

def ref_call(payload):
    return request(REF, payload)

def peer_address(name):
    # Cada servidor publica sua porta interna de comunicação no serviço de referência.
    servers = ref_call({"action": "list"}).get("servers", [])
    info = next((item for item in servers if item["name"] == name), None)
    if not info:
        raise RuntimeError(f"servidor {name} não encontrado")
    return f"tcp://{name}:{info.get('peer_port', 5651)}"

def elect_coordinator():
    global coordinator
    servers = ref_call({"action": "list"}).get("servers", [])
    alive = []
    for item in servers:
        try:
            if item["name"] == NAME:
                alive.append(item)
            else:
                answer = request(peer_address(item["name"]), {
                    "action": "election", "candidate": NAME
                }, timeout=1200)
                if answer.get("ok"):
                    alive.append(item)
        except (zmq.ZMQError, RuntimeError):
            continue
    if alive:
        winner = max(alive, key=lambda item: item["rank"])
        coordinator = winner["name"]
        if coordinator == NAME:
            # Pequeno atraso para permitir que a inscrição PUB/SUB seja estabelecida.
            time.sleep(0.2)
            pub.send_multipart([b"servers", pack({
                "coordinator": NAME, "timestamp": physical_now()
            })])
            print(f"[{NAME}] eleito coordenador (rank {rank})", flush=True)
        else:
            print(f"[{NAME}] coordenador: {coordinator}", flush=True)
    return coordinator

def sync_clock():
    global physical_offset, coordinator
    if not coordinator:
        elect_coordinator()
    if coordinator == NAME:
        servers = ref_call({"action": "list"}).get("servers", [])
        samples = [time.time() + physical_offset]
        for item in servers:
            if item["name"] == NAME:
                continue
            try:
                answer = request(peer_address(item["name"]), {"action": "clock_sample"})
                samples.append(float(answer["time"]))
            except (zmq.ZMQError, KeyError, RuntimeError):
                pass
        average = sum(samples) / len(samples)
        physical_offset = average - time.time()
    elif coordinator:
        try:
            answer = request(peer_address(coordinator), {"action": "get_time"})
            physical_offset = float(answer["time"]) - time.time()
        except (zmq.ZMQError, KeyError, RuntimeError):
            print(f"[{NAME}] coordenador indisponível; nova eleição", flush=True)
            coordinator = elect_coordinator()

def peer_loop():
    peer = ctx.socket(zmq.REP)
    peer.bind(f"tcp://0.0.0.0:{PEER_PORT}")
    while True:
        try:
            message = unpack(peer.recv())
            action = message.get("action")
            if action in ("election",):
                reply = {"ok": True, "timestamp": physical_now()}
            elif action == "clock_sample":
                reply = {"time": time.time() + physical_offset, "timestamp": physical_now()}
            elif action == "get_time":
                reply = {"time": time.time() + physical_offset, "timestamp": physical_now()}
            else:
                reply = {"ok": False, "error": "ação desconhecida", "timestamp": physical_now()}
            peer.send(pack(reply))
        except Exception as exc:
            print(f"[{NAME}] erro no canal entre servidores: {exc}", flush=True)
            peer.send(pack({"ok": False, "error": str(exc), "timestamp": now()}))

threading.Thread(target=peer_loop, daemon=True).start()
time.sleep(2)
registration = ref_call({
    "action": "register", "name": NAME, "peer_port": PEER_PORT
})
rank = int(registration.get("rank", 0))
time.sleep(3)  # aguarda o registro dos demais servidores antes da eleição inicial
elect_coordinator()

rep = ctx.socket(zmq.REP)
rep.bind(f"tcp://0.0.0.0:{CLIENT_PORT}")
print(f"[{NAME}] pronto: cliente={CLIENT_PORT}, peer={PEER_PORT}, rank={rank}", flush=True)

while True:
    try:
        req = unpack(rep.recv())
        logical_clock = max(logical_clock, int(req.get("logical_clock", 0)))
        message_count += 1
        action = req.get("action")
        reply = {"ok": True, "timestamp": physical_now()}

        if action == "login":
            user = req.get("user", "anonimo")
            data["users"].append({"user": user, "timestamp": physical_now()})
            save()
            reply["message"] = f"login realizado: {user}"
        elif action == "list_channels":
            reply["channels"] = list(data["channels"])
        elif action == "create_channel":
            channel = req.get("channel", "").strip()
            if not channel:
                reply.update(ok=False, error="nome do canal vazio")
            elif channel in data["channels"]:
                reply.update(ok=False, error="canal já existe")
            else:
                data["channels"].append(channel)
                save()
                reply["message"] = "canal criado"
        elif action == "publish":
            channel = req.get("channel", "")
            message = req.get("message", "")
            if channel not in data["channels"]:
                reply.update(ok=False, error="canal não existe")
            elif not message:
                reply.update(ok=False, error="mensagem vazia")
            else:
                logical_clock += 1
                item = {
                    "user": req.get("user"), "channel": channel, "message": message,
                    "timestamp": physical_now(), "logical_clock": logical_clock
                }
                data["messages"].append(item)
                save()
                pub.send_multipart([channel.encode(), pack(item)])
                reply["message"] = "publicação realizada"
        elif action == "history":
            reply["messages"] = list(data["messages"])
        else:
            reply.update(ok=False, error="ação desconhecida")

        # O servidor incrementa o relógio antes de enviar sua resposta.
        logical_clock += 1
        reply["logical_clock"] = logical_clock
        if message_count % 10 == 0:
            ref_call({"action": "heartbeat", "name": NAME})
        if message_count % 15 == 0:
            sync_clock()
        print(f"[{NAME}] requisição {action} recebida", flush=True)
        rep.send(pack(reply))
    except Exception as exc:
        print(f"[{NAME}] erro: {exc}", flush=True)
        try:
            rep.send(pack({
                "ok": False, "error": str(exc), "timestamp": physical_now(),
                "logical_clock": logical_clock
            }))
        except zmq.ZMQError:
            pass
