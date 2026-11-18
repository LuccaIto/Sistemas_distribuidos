#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "arquivo.cpp"

using json = nlohmann::json;

std::string get_timestamp() {
    auto agora = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(agora);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(agora.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

int main() {
    inicializar();

    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    std::cout << "==================================================\n";
    std::cout << "SERVIDOR INICIADO\n";
    std::cout << "Escutando tcp://localhost:5555\n";
    std::cout << "==================================================\n";

    while (true) {
        zmq::message_t req_msg;
        socket.recv(req_msg, zmq::recv_flags::none);

        // Desserializa a mensagem binária do MessagePack
        json mensagem = json::from_msgpack(
            static_cast<const uint8_t*>(req_msg.data()), 
            req_msg.size()
        );

        std::cout << "\n[RECEBIDO]\n";
        std::cout << mensagem.dump(4) << std::endl;

        json dados = carregar();

        json resposta = {
            {"timestamp", get_timestamp()},
            {"success", false},
            {"message", "Operação inválida"}
        };

        std::string tipo = mensagem.value("tipo", "");

        if (tipo == "LOGIN") {
            std::string usuario = mensagem["usuario"];
            
            dados["logins"].push_back({
                {"usuario", usuario},
                {"timestamp", mensagem["timestamp"]}
            });
            salvar(dados);

            resposta["success"] = true;
            resposta["message"] = usuario + " conectado.";

        } else if (tipo == "LIST_CHANNELS") {
            resposta["success"] = true;
            resposta["canais"] = dados["canais"];

        } else if (tipo == "CREATE_CHANNEL") {
            std::string canal = mensagem["canal"];
            
            bool existe = false;
            for (const auto& c : dados["canais"]) {
                if (c == canal) {
                    existe = true;
                    break;
                }
            }

            if (existe) {
                resposta["success"] = false;
                resposta["message"] = "Canal já existe.";
            } else {
                dados["canais"].push_back(canal);
                salvar(dados);
                resposta["success"] = true;
                resposta["message"] = "Canal '" + canal + "' criado.";
            }
        }

        std::cout << "[ENVIADO]\n";
        std::cout << resposta.dump(4) << std::endl;

        // Serializa a resposta para MessagePack e envia de volta
        std::vector<uint8_t> packed = json::to_msgpack(resposta);
        socket.send(zmq::buffer(packed.data(), packed.size()), zmq::send_flags::none);
    }

    return 0;
}