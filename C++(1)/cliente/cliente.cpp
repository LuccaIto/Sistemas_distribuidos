#include <iostream>
#include <string>
#include <map>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <zmq.hpp>
#include <msgpack.hpp>

std:: string gerarDataHora(){
    auto agora = std::chrono::system_clock::now();
    auto tempo = std::chrono::system_clock::to_time_t(agora);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(agora.time_since_epoch()) % 1000;

    std::stringstream fluxoTexto;
    fluxoTexto << std::put_time(std::localtime(&tempo), "%Y-%m-%dT%H:%M:%S");
    fluxoTexto << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return fluxoTexto.str();
}

void enviarRequisicao(zmq::socket_t & socketRequisicao, std::map<std::string, std::string> & dadosMensagem){
    std::cout << "\n-----------------------------------\n";
    std::cout << "\n [CLIENTE -_-_-_- ENVIANDO REQUISICAO] \n";
    for (const auto & [chave, valor] : dadosMensagem){
        std::cout << " " << chave << ": " << valor << "\n";
    }

    msgpack::sbuffer bufferBinario;
    msgpack::pack(bufferBinario, dadosMensagem);

    socketRequisicao.send(zmq::buffer(bufferBinario.data(), bufferBinario.size()), zmq::send_flags::none);

    zmq::message_t mensagemResposta;
    socketRequisicao.recv(mensagemResposta, zmq::recv_flags::none);

    msgpack:: object_handle handleObjeto = msgpack::unpack(
        static_cast<const char*>(mensagemResposta.data()),
        mensagemResposta.size()
    );

    msgpack::object objetoRetorno = handleObjeto.get();

    std::cout << "\n [CLIENTE -_-_-_- RESPOSTA RECEBIDA] \n" << objetoRetorno << "\n";

}

int main(){
    zmq::context_t contextoZmq(1);
    zmq::socket_t socketRequisicao(contextoZmq, zmq::socket_type::req);
    socketRequisicao.connect("tcp://localhost:5555");

    std::string nomeUsuarioBot = "bot_cpp";

    //executa o login
    std::map<std::string, std::string> requisicaoLogin = {
        {"timestamp", gerarDataHora()},
        {"tipo", "LOGIN"},
        {"usuario", nomeUsuarioBot},
    };
    enviarRequisicao(socketRequisicao, requisicaoLogin);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    //lista os canais
    std::map<std::string, std::string> requisicaoListarCanais1 = {
        {"timestamp", gerarDataHora()},
        {"tipo", "LIST_CHANNELS"},
    };
    enviarRequisicao(socketRequisicao, requisicaoListarCanais1);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // solicita a criacao de um novo canal
    std::map<std::string, std::string> requisicaoCriarCanal = {
        {"timestamp", gerarDataHora()},
        {"tipo", "CREATE_CHANNEL"},
        {"canal", "jogos"},
    };
    enviarRequisicao(socketRequisicao, requisicaoCriarCanal);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // lista os canais novamente para confirmar
    std::map<std::string, std::string> requisicaoListarCanais2 = {
        {"timestamp", gerarDataHora()},
        {"tipo", "LIST_CHANNELS"},
    };
    enviarRequisicao(socketRequisicao, requisicaoListarCanais2);
    std::this_thread::sleep_for(std::chrono::seconds(1));

}