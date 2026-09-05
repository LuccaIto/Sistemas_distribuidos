import zmq
from time import sleep

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://broker:5555")
i = 0

while True:
    print("\n1. Adicionar tarefa")
    print("2. Remover tarefa")
    print("3. Listar tarefas")
    print("4. Sair")
    opcao = input("Escolha uma opcao: ")

    if opcao == "1":
        tarefa = input("Digite a descricao da tarefa: ")
        socket.send_json({"acao": "adicionar", "tarefa": tarefa})
        resposta = socket.recv_json()
        print(f">> {resposta['mensagem']}")
        
    elif opcao == "2":
        id_tarefa = input("Digite o ID da tarefa para remover: ")
        socket.send_json({"acao": "remover", "id": id_tarefa})
        resposta = socket.recv_json()
        print(f">> {resposta['mensagem']}")
        
    elif opcao == "3":
        socket.send_json({"acao": "listar"})
        resposta = socket.recv_json()
        tarefas = resposta.get("tarefas", {})
        print("\n--- SUAS TAREFAS ---")
        if not tarefas:
            print("Nenhuma tarefa.")
        else:
            for k, v in tarefas.items():
                print(f"[{k}] {v}")
        print("--------------------")
        
    elif opcao == "4":
        break
    else:
        print("Opcao invalida.")
