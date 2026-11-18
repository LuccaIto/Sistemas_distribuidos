import json
import os

ARQUIVO = "data.json"

def inicializar():
    if not os.path.exists(ARQUIVO):
        with open(ARQUIVO, "w", encoding="utf-8") as f:
            json.dump({
                "logins": [],
                "canais": ["geral"]
            }, f, indent=4)

def carregar():
    with open(ARQUIVO, "r", encoding="utf-8") as f:
        return json.load(f)

def salvar(dados):
    with open(ARQUIVO, "w", encoding="utf-8") as f:
        json.dump(dados, f, indent=4)