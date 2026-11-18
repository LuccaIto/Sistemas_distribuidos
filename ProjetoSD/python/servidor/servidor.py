import zmq
import msgpack
from datetime import datetime

#from persistencia 
#import inicializar, carregar, salvar

#inicializar()

context = zmq.Context()
socket = context.socket(zmq.REP)

socket.bind("tcp://*:5555")

print("="*50)
print("SERVIDOR INICIADO")
print("Escutando tcp://localhost:5555")
print("="*50)

while True:

    mensagem_binaria = socket.recv()
    mensagem = msgpack.unpackb(mensagem_binaria, raw=False)

    print("\n[RECEBIDO]")
    print(mensagem)

    dados = carregar()

    resposta = {
        "timestamp": datetime.now().isoformat(),
        "success": False,
        "message": "Operação inválida"
    }

    tipo = mensagem.get("tipo")

    # LOGIN
    if tipo == "LOGIN":

        usuario = mensagem["usuario"]

        dados["logins"].append({
            "usuario": usuario,
            "timestamp": mensagem["timestamp"]
        })

        salvar(dados)

        resposta = {
            "timestamp": datetime.now().isoformat(),
            "success": True,
            "message": f"{usuario} conectado."
        }

    # LISTAR CANAIS
    elif tipo == "LIST_CHANNELS":

        resposta = {
            "timestamp": datetime.now().isoformat(),
            "success": True,
            "canais": dados["canais"]
        }

    # CRIAR CANAL
    elif tipo == "CREATE_CHANNEL":

        canal = mensagem["canal"]

        if canal in dados["canais"]:

            resposta = {
                "timestamp": datetime.now().isoformat(),
                "success": False,
                "message": "Canal já existe."
            }

        else:

            dados["canais"].append(canal)
            salvar(dados)

            resposta = {
                "timestamp": datetime.now().isoformat(),
                "success": True,
                "message": f"Canal '{canal}' criado."
            }

    print("[ENVIADO]")
    print(resposta)

    socket.send(msgpack.packb(resposta))