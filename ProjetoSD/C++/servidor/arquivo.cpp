#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

const std::string ARQUIVO = "data.json";

void inicializar() {
    if (!std::filesystem::exists(ARQUIVO)) {
        json dados = {
            {"logins", json::array()},
            {"canais", {"geral"}}
        };
        
        std::ofstream f(ARQUIVO);
        f << dados.dump(4);
    }
}

json carregar() {
    std::ifstream f(ARQUIVO);
    json dados;
    f >> dados;
    return dados;
}

void salvar(const json& dados) {
    std::ofstream f(ARQUIVO);
    f << dados.dump(4);
}