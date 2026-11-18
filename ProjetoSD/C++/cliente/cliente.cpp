#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <zmq.hpp>
#include <msgpack.hpp>

// Função para gerar timestamp no formato ISO 8601 (equivalente ao .isoformat() do Python)
std::string obter_timestamp_iso(){
    auto agora = std::chrono::system_clock::now();
    auto tempo_t = std::chrono::system_clock::to_time_t(agora);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(agora.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&tempo_t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Função auxiliar para enviar mensagens e receber respostas
void enviar(zmq::socket_t& socket, std::map<std::string, std::string>& msg_map){
    std::cout << "\n" << std::string(40, '=') << "\n";
    std::cout << "[ENVIANDO]\n";
    for (auto const& [chave, valor] : msg_map) {
        std::cout << chave << ": " << valor << "\n";
    }

    // Serializa o map para MessagePack (equivalente ao msgpack.packb)
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, msg_map);

    // Envia via ZeroMQ
    socket.send(zmq::buffer(sbuf.data(), sbuf.size()), zmq::send_flags::none);

    // Recebe a resposta do servidor
    zmq::message_t resposta_msg;
    auto resultado_recv = socket.recv(resposta_msg, zmq::recv_flags::none);

    // Desserializa a resposta (equivalente ao msgpack.unpackb)
    msgpack::object_handle oh = msgpack::unpack(
        static_cast<const char*>(resposta_msg.data()), 
        resposta_msg.size()
    );
    msgpack::object obj_resposta = oh.get();

    std::cout << "[RECEBIDO]\n";
    // Exibe o objeto MessagePack recebido
    std::cout << obj_resposta << std::endl;
}

int main() {
    // Inicializa o contexto ZeroMQ e o socket REQ
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);
    
    // Conecta ao mesmo endereço do Python/Java/C++
    socket.connect("tcp://localhost:5555");

    std::string usuario = "bot_cpp";

    // 1. LOGIN
    std::map<std::string, std::string> login = {
        {"timestamp", obter_timestamp_iso()},
        {"tipo", "LOGIN"},
        {"usuario", usuario}
    };
    enviar(socket, login);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 2. LISTAR
    std::map<std::string, std::string> listar1 = {
        {"timestamp", obter_timestamp_iso()},
        {"tipo", "LIST_CHANNELS"}
    };
    enviar(socket, listar1);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 3. CRIAR CANAL
    std::map<std::string, std::string> novo_canal = {
        {"timestamp", obter_timestamp_iso()},
        {"tipo", "CREATE_CHANNEL"},
        {"canal", "jogos"}
    };
    enviar(socket, novo_canal);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 4. LISTAR NOVAMENTE
    std::map<std::string, std::string> listar2 = {
        {"timestamp", obter_timestamp_iso()},
        {"tipo", "LIST_CHANNELS"}
    };
    enviar(socket, listar2);

    return 0;
}