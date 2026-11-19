#include <iostream>
#include <zmq.hpp>

int main(){
    zmq::context_t contextoProxy(1);

    // Socket Xsub (porta 5557) -> recebe as msgs publicadas pelos servidores (PUB)
    zmq::socket_t entradaPublicacoesXsub(contextoProxy, zmq:: socket_type::xsub);
    entradaPublicacoesXsub.bind("tcp://*:5557");

    
    // Socket Xpub (porta 5557) -> manda as msgs para os clientes (SUB)
    zmq::socket_t saidaAssinantesXpub(contextoProxy, zmq::socket_type::xpub);
    saidaAssinantesXpub.bind("tcp://*:5558");

    std::cout << " \n -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_- \n";
    std::cout << " _____ PROXY PUB/SUB ATIVO _____ \n";
    std::cout << "      XSUB(Publishers): 5557 \n";
    std::cout << "      XPUB(Assisnantes): 5558 \n";
    std::cout << "  -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_- \n";

    zmq::proxy(entradaPublicacoesXsub, saidaAssinantesXpub);
}