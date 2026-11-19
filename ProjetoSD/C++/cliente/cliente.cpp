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

    auto statusEnvio = socketRequisicao.send(zmq::buffer(bufferBinario.data(), bufferBinario.size()), zmq::send_flags::none);
    (void)statusEnvio;

    zmq::message_t mensagemResposta;
    auto statusRecebimento = socketRequisicao.recv(mensagemResposta, zmq::recv_flags::none);
    (void)statusRecebimento;

    msgpack:: object_handle handleObjeto = msgpack::unpack(
        static_cast<const char*>(mensagemResposta.data()),
        mensagemResposta.size()
    );

    msgpack::object objetoRetorno = handleObjeto.get();

    std::cout << "\n [CLIENTE -_-_-_- RESPOSTA RECEBIDA] \n" << objetoRetorno << "\n";

}

void lerMensagemSub(zmq::socket_t & socketAssinatura){
    while(true){
        zmq::message_t parteTopico;
        if(!socketAssinatura.recv(parteTopico, zmq::recv_flags::dontwait)){
            break;
        }

        std::string topicoRecebido(static_cast<const char*>(parteTopico.data()), parteTopico.size());

        zmq::message_t parteDados;
        if(parteTopico.more()){
            auto statusPayload = socketAssinatura.recv(parteDados, zmq::recv_flags::none);
            (void)statusPayload;
        }

        std::string timestampRecebimento = gerarDataHora();

        try{
            msgpack::object_handle handleMsg = msgpack::unpack(
                static_cast<const char*>(parteDados.data()),
                parteDados.size()
            );

            std::map<std::string, std::string> pacoteRecebido;
            handleMsg.get().convert(pacoteRecebido);

            std::string canalMsg = pacoteRecebido.count("canal") ? pacoteRecebido["canal"] : topicoRecebido;
            std::string usuarioMsg = pacoteRecebido.count("usuario") ? pacoteRecebido["usuario"] : "desconhecido";
            std::string textoMsg = pacoteRecebido.count("mensagem") ? pacoteRecebido["mensagem"] : "";
            std::string timestampEnvio = pacoteRecebido.count("timestamp") ? pacoteRecebido["timestamp"] : "N/A";

            std::cout << "\n -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_- \n";
            std::cout << " [ SUB - MENSAGEM RECEBIDA ] \n";
            std::cout << " Canal:           " << canalMsg << "\n";
            std::cout << " Usuario:         " << usuarioMsg << "\n";
            std::cout << " Mensagem:        " << textoMsg << "\n";
            std::cout << " Timestamp Env:   " << timestampEnvio << "\n";
            std::cout << " Timestamp Rec:   " << timestampRecebimento << "\n";
            std::cout << " -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_- \n";
        }
        catch(const std::exception & erroDecodificacao){
            std::cerr << "\n[SUB - ERRO AO DECODIFICAR]: " << erroDecodificacao.what() << "\n";
        }
    }
}

int main(){
    zmq::context_t contextoZmq(1);
    zmq::socket_t socketRequisicao(contextoZmq, zmq::socket_type::req);
    socketRequisicao.connect("tcp://localhost:5555");

    std::string nomeUsuarioBot = "bot_cpp";

    // socket SUB -> recebe mensagens publicadas pelo proxy XPUB
    zmq::socket_t socketAssinatura(contextoZmq, zmq::socket_type::sub);
    socketAssinatura.connect("tcp://localhost:5558");

    // Assina os canais de interesse
    socketAssinatura.set(zmq::sockopt::subscribe, "geral");
    socketAssinatura.set(zmq::sockopt::subscribe, "jogos");

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

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

    // Loop que publica periodicamente e le mensagens pelo SUB
    std::string canais[] = {"geral", "jogos"};
    int indiceCanal = 0;
    int contadadorPub = 1;
    auto ultimaPublicacao = std::chrono::steady_clock::now() - std::chrono::seconds(3);

    std::cout << "\n [ BOT ATIVO ] Escutando SUB e publicando REQ";
    std::cout << "\n Aperte Ctrl+c para encerrar \n";

    while(true){
        lerMensagemSub(socketAssinatura);

        auto agora = std::chrono::steady_clock::now();
        auto decorrido = std::chrono::duration_cast<std::chrono::seconds>(agora - ultimaPublicacao).count();

        if(decorrido >= 3){
            std::string canalAtual = canais[indiceCanal % 2];
            std::map<std::string, std::string> requisicaoPublish = {
                {"timestamp", gerarDataHora()},
                {"tipo", "PUBLISH"},
                {"canal", canalAtual},
                {"usuario", nomeUsuarioBot},
                {"mensagem", "Mensagem #" + std::to_string(contadadorPub) + " do " + nomeUsuarioBot},
            };

            enviarRequisicao(socketRequisicao, requisicaoPublish);
            indiceCanal++;
            contadadorPub++;
            ultimaPublicacao = std::chrono::steady_clock::now();

            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            lerMensagemSub(socketAssinatura);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }





}