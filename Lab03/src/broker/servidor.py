import zmq

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.connect("tcp://broker:5556")

tarefas = {}
id_atual = 1

while True:
    requisicao = socket.recv_json()
    print(f"Requisição recebida: {requisicao}", flush=True)
    
    acao = requisicao.get("acao")
    resposta = {}

    if acao == "adicionar":
        descricao = requisicao.get("tarefa")
        tarefas[str(id_atual)] = descricao
        resposta = {"status": "ok", "mensagem": f"Tarefa '{descricao}' adicionada (ID {id_atual})."}
        id_atual += 1
        
    elif acao == "remover":
        id_tarefa = str(requisicao.get("id"))
        if id_tarefa in tarefas:
            removida = tarefas.pop(id_tarefa)
            resposta = {"status": "ok", "mensagem": f"Tarefa '{removida}' removida."}
        else:
            resposta = {"status": "erro", "mensagem": "ID não encontrado."}
            
    elif acao == "listar":
        resposta = {"status": "ok", "tarefas": tarefas}
        
    else:
        resposta = {"status": "erro", "mensagem": "Ação inválida."}
        
    socket.send_json(resposta)

