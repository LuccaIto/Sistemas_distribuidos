# Sistema de troca de mensagens — Sistemas Distribuídos

Protótipo de troca de mensagens entre bots e servidores, desenvolvido com Python, ZeroMQ, Docker Compose e MessagePack.

## Tecnologias e escolhas

- **Python 3.12:** implementação dos bots, servidores, serviço de referência e proxy.
- **ZeroMQ:** comunicação entre processos.
  - REQ/REP para requisições e respostas;
  - PUB/SUB para publicações nos canais;
  - XSUB/XPUB no proxy de distribuição.
- **MessagePack:** serialização binária de todas as mensagens trocadas pela rede.
- **JSON:** persistência local dos dados de cada servidor. JSON é usado somente em disco, não como protocolo de comunicação.
- **Docker Compose:** inicialização dos serviços e dos dois bots de teste.

## Como executar

Na pasta raiz do projeto:

```bash
docker compose up --build
```

Para encerrar:

```bash
docker compose down
```

Para encerrar e apagar também os dados persistidos nos volumes:

```bash
docker compose down -v
```

## Estrutura

```text
ProjetoSD/
├── docker-compose.yml
├── README.md
└── python/
    ├── cliente/
    │   ├── Dockerfile
    │   ├── requirements.txt
    │   ├── common.py
    │   ├── cliente.py
    │   └── proxy.py
    └── servidor/
        ├── Dockerfile
        ├── requirements.txt
        ├── common.py
        ├── reference.py
        └── servidor.py
```

## Serviços iniciados

| Serviço | Responsabilidade |
|---|---|
| `broker` | Proxy PUB/SUB, com XSUB na porta 5557 e XPUB na 5558 |
| `reference` | Cadastro dos servidores, atribuição de rank, lista de ativos e heartbeat |
| `server1` | Atendimento aos bots, canais, publicações, histórico, relógios e eleição |
| `server2` | Segundo servidor independente, com as mesmas responsabilidades |
| `bot1` e `bot2` | Bots de teste que fazem login, consultam canais, publicam e recebem mensagens |

Cada servidor mantém seus próprios dados no volume Docker correspondente (`server1_data` ou `server2_data`). Os arquivos não são compartilhados.

## Formato das mensagens

As mensagens de rede são serializadas com MessagePack. As mensagens de aplicação incluem:

- `timestamp`: instante de envio;
- `logical_clock`: contador lógico do processo que envia a mensagem;
- `action`: operação solicitada, quando aplicável;
- demais campos necessários à operação.

O relógio lógico é incrementado antes do envio. Ao receber uma mensagem, o processo compara o contador recebido com o seu contador local e mantém o maior valor. Antes de enviar a resposta, o servidor incrementa seu contador.

## Funcionalidades implementadas

### Parte 1 — Login e canais

- Login do bot usando seu nome, sem senha.
- Criação de canais e listagem dos canais existentes.
- Persistência dos logins e canais em arquivo local de cada servidor.
- Resposta de sucesso ou erro para as requisições.

### Parte 2 — Publicação e inscrição

- Proxy PUB/SUB separado do broker REQ/REP.
- Inscrição dos bots em até três canais.
- Publicação de mensagens nos tópicos correspondentes aos canais.
- Retorno do status da publicação ao bot.
- Persistência das mensagens publicadas, com usuário, canal, conteúdo, timestamp e relógio lógico.

### Parte 3 — Relógios e heartbeat

- Relógio lógico nos bots e servidores.
- Serviço de referência que atribui rank e mantém a lista de servidores disponíveis.
- Heartbeat enviado a cada dez requisições de clientes recebidas pelo servidor.
- Tentativa de sincronização do relógio físico a cada quinze requisições.

### Parte 4 — Eleição e sincronização

- Eleição do servidor de maior rank entre os servidores que respondem.
- Armazenamento do nome do coordenador em cada servidor.
- Publicação do coordenador eleito no tópico `servers`.
- Sincronização física inspirada no algoritmo de Berkeley: o coordenador coleta amostras dos servidores disponíveis e calcula a média; os demais consultam o horário do coordenador.
- O serviço de referência não fornece a hora no heartbeat; sua função é manter o cadastro e a disponibilidade dos servidores.

## Observações

