#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include "arquivo.cpp"
#include <thread>
#include <vector>

using json = nlohmann::json;        // Cria um apelido para a classe json


// Gera um bloco de data e hora atual 
std::string obterTimesstampAtual(){
    auto agora = std::chrono::system_clock::now();
    auto tempo = std::chrono::system_clock::to_time_t(agora);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(agora.time_since_epoch()) % 1000;       //extrai os milissegundos
    
    std::stringstream fluxoTexto;
    fluxoTexto << std::put_time(std::localtime(&tempo), "%Y-%m-%dT%H:%M:%S");       // Formata data e hora no formato padrão ISO
    fluxoTexto << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return fluxoTexto.str();
}

int main(){
    inicializarArmazenamento();

    zmq::context_t contextoZmq(1);

    zmq::socket_t socketResposta(contextoZmq, zmq::socket_type::rep);
    socketResposta.bind("tcp://*:5555");

    zmq::socket_t socketPublicacao(contextoZmq, zmq::socket_type::pub);
    socketPublicacao.connect("tcp://localhost:5557");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "\n-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-\n";
    std::cout << " ______SERVIDOR C++ ATIVO______ \n";
    std::cout << "\n-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-\n";

    while(true){
        zmq::message_t mensagemBin;
        socketResposta.recv(mensagemBin, zmq::recv_flags::none);

        try{
            const uint8_t* ponteiroBit = static_cast<const uint8_t*>(mensagemBin.data());
            json requisicaoCliente = json::from_msgpack(ponteiroBit, ponteiroBit + mensagemBin.size());

            std::cout << "\n [ SERVIDOR - REQUISICAO RECEBIDA ] \n";
            std::cout << requisicaoCliente.dump(4) << std::endl;

            json dadosArmazenados = carregarDados();

            json respostaServidor = {
                {"timestamp", obterTimesstampAtual()},
                {"success", false},
                {"message", "Comando ou operação errada"},
            };

            std:: string tipoOperacao = requisicaoCliente.value("tipo", "");

            if(tipoOperacao == "LOGIN"){
                std::string nomeUsuario = requisicaoCliente["usuario"];

                dadosArmazenados["logins"].push_back({
                    {"usuario", nomeUsuario},
                    {"timestamp", requisicaoCliente["timestamp"]},
                });
                salvarDados(dadosArmazenados);

                respostaServidor["success"] = true;
                respostaServidor["message"] = nomeUsuario + " Conectado com sucesso!! ";
            }
            else if(tipoOperacao == "LIST_CHANNELS"){
                respostaServidor["success"] = true;
                respostaServidor["message"] = " Canais recuperados com sucesso!! ";
                respostaServidor["canais"] = dadosArmazenados["canais"];
            }
            else if(tipoOperacao == "CREATE_CHANNEL"){
                std::string nomeCanalDesejado = requisicaoCliente["canal"];

                bool canalExistente = false;

                for(const auto & canalAtual : dadosArmazenados["canais"]){
                    if(canalAtual == nomeCanalDesejado){
                        canalExistente = true;
                        break;
                    }
                }

                if(canalExistente){
                    respostaServidor["success"] = false;
                    respostaServidor["message"] = " Canal já existe.. ";
                }
                else{
                    dadosArmazenados["canais"].push_back(nomeCanalDesejado);
                    salvarDados(dadosArmazenados);

                    respostaServidor["success"] = true;
                    respostaServidor["message"] = "Canal " + nomeCanalDesejado + " criado com sucesso!! ";
                }
            }
            else if(tipoOperacao == "PUBLISH"){
                std::string nomeCanal = requisicaoCliente.value("canal", "");
                std::string textoMsg = requisicaoCliente.value("mensagem", "");
                std::string nomeUsuario = requisicaoCliente.value("usuario", "");

                bool canalValido = false;
                for(const auto & canalAtual : dadosArmazenados["canais"]){
                    if(canalAtual == nomeCanal){
                        canalValido = true;
                        break;
                    }
                }

                if(nomeCanal.empty() || textoMsg.empty()){
                    respostaServidor["success"] = false;
                    respostaServidor["message"] = " Canal ou mensagem vazios.. ";
                }
                else if(!canalValido){
                    respostaServidor["success"] = false;
                    respostaServidor["message"] = " Canal não existe.. ";
                }
                else{
                    std::string tsEnvio = obterTimesstampAtual();
                    json pacotePublicacao = {
                        {"canal", nomeCanal},
                        {"mensagem", textoMsg},
                        {"usuario", nomeUsuario},
                        {"timestamp", tsEnvio},
                    };
                    std::vector<uint8_t> bitPublicacao = json::to_msgpack(pacotePublicacao);

                    auto sendTopico = socketPublicacao.send(zmq::buffer(nomeCanal), zmq::send_flags::sndmore);
                    auto sendPayload = socketPublicacao.send(zmq::buffer(bitPublicacao.data(), bitPublicacao.size()),
                                                                zmq::send_flags::none);
                    
                    (void)sendTopico;
                    (void)sendPayload;

                    dadosArmazenados["mensagens"].push_back(pacotePublicacao);
                    salvarDados(dadosArmazenados);

                    respostaServidor["success"] = true;
                    respostaServidor["message"] = "Mensagem publicada no canal " + nomeCanal + " com sucesso!! ";
                }
            }
        

            std::cout << "[ SERVIDOR - RESPOSTA ENVIADA] \n";
            std::cout << respostaServidor.dump(4) << std::endl;
            
            std::vector<uint8_t> bitResposta = json::to_msgpack(respostaServidor);
            socketResposta.send(zmq::buffer(bitResposta.data(), bitResposta.size()), zmq::send_flags::none);

        }
        catch(const std::exception & erroExecucao){
            std::cerr << "[ ERRO NO PROCESSAMENTO DO SERVIDOR ]" << erroExecucao.what() << std::endl;
        }
    }
    return 0;
}