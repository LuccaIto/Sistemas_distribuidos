#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>        // Inclui a biblioteca nlohmann/json para gerenciar estruturas JSON

using json = nlohmann::json;        // Cria um apelido simplificado para o tipo json da biblioteca

const std::string CAMINHO_BANCO_DE_DADOS = "data.json";


// Garante que o arquivo inicial exista no disco
void inicializarArmazenamento(){
    if(!std::filesystem::exists(CAMINHO_BANCO_DE_DADOS)){
        json estadoInicial = {
            {"logins", json::array()},
            {"canais", {"geral"}}
        };
        std::ofstream arquivoSaida(CAMINHO_BANCO_DE_DADOS);
        arquivoSaida << estadoInicial.dump(4);
    }
}

// Carrega o JSON atual do arquivo em disco
json carregarDados(){
    std::ifstream arquivoEntrada(CAMINHO_BANCO_DE_DADOS);
    json dadosCarregados;
    arquivoEntrada >> dadosCarregados;
    return dadosCarregados;
}

void salvarDados(const json & dadosSalvos){
    std::ofstream arquivoSaida(CAMINHO_BANCO_DE_DADOS);
    arquivoSaida << dadosSalvos.dump(4);
}
