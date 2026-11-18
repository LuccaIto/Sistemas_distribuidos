import zmq
import msgpack
from datetime import datetime
import time

context = zmq.Context()

socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

usuario = "bot_python"

def enviar(msg):

    print("\n"+"="*40)
    print("[ENVIANDO]")
    print(msg)

    socket.send(msgpack.packb(msg))

    resposta = msgpack.unpackb(socket.recv(), raw=False)

    print("[RECEBIDO]")
    print(resposta)

    return resposta

# LOGIN

login = {
    "timestamp": datetime.now().isoformat(),
    "tipo": "LOGIN",
    "usuario": usuario
}

enviar(login)

time.sleep(1)

# LISTAR

listar = {
    "timestamp": datetime.now().isoformat(),
    "tipo": "LIST_CHANNELS"
}

enviar(listar)

time.sleep(1)

# CRIAR

novo_canal = {
    "timestamp": datetime.now().isoformat(),
    "tipo": "CREATE_CHANNEL",
    "canal": "jogos"
}

enviar(novo_canal)

time.sleep(1)

# LISTAR NOVAMENTE

listar = {
    "timestamp": datetime.now().isoformat(),
    "tipo": "LIST_CHANNELS"
}

enviar(listar)