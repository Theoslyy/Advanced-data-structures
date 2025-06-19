#include "parser.hpp" 
#include "BST.cpp"   
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <tuple> 

using namespace std;

int main() {
    string data_path;
    cin >> data_path;

    vector<operation> v = parser_text(data_path); 
    BST arvore;

    ofstream fout("out.txt"); 
    if (!fout.is_open()) {
        cerr << "Erro ao criar o arquivo de saida out.txt" << endl;
        return 1;
    }

    for (const auto& op : v) {
        command tipo = get<0>(op);
        int chave = get<1>(op);
        int versao_input = get<2>(op);

        if (tipo == INC) {
            // Chamada corrigida para o método de inserção definido em BST.cpp
            arvore.insertNode(chave); 
        }
        else if (tipo == REM) {
            // Chamada corrigida para o método de remoção definido em BST.cpp
            arvore.remover(chave); 
        }
        else if (tipo == SUC) {
            int max_version_idx = arvore.versions.size() - 1;
            int ver_to_use;

            fout << "SUC " << chave << " " << versao_input << "\n"; // Imprime a operação

            // Se não há versões, não há sucessor
            if (max_version_idx < 0) {
                 fout << "infinito\n";
                 continue;
            }

            // Define a versão real a ser usada na busca
            if (versao_input >= 0 && versao_input <= max_version_idx) {
                ver_to_use = versao_input;
            } else {
                ver_to_use = max_version_idx;
            }
            
            // Lógica do sucessor corrigida:
            // 1. Pega a raiz da versão correta
            Node* raiz_da_versao = arvore.versions[ver_to_use];
            Node* suc_node = nullptr;

            // 2. Procura o sucessor a partir da raiz daquela versão
            if (raiz_da_versao != nullptr) {
                suc_node = arvore.sucessor(raiz_da_versao, chave, ver_to_use);
            }

            if (suc_node != nullptr) {
                // Para obter a chave do nó sucessor, é preciso materializá-lo na versão correta
                Node temp_suc;
                arvore.applyVersion(suc_node, &temp_suc, ver_to_use);
                fout << temp_suc.nodeKey << "\n";
            } else {
                fout << "infinito\n";
            }
        }
        else if (tipo == IMP) {
            int max_version_idx = arvore.versions.size() - 1;
            int ver_to_use;

            fout << "IMP " << versao_input << "\n";

            // Se não há versões, imprime linha vazia
            if (max_version_idx < 0) {
                 fout << "\n";
                 continue;
            }

            // Define a versão real a ser usada
            if (versao_input >= 0 && versao_input <= max_version_idx) {
                ver_to_use = versao_input;
            } else {
                ver_to_use = max_version_idx;
            }

            auto impressoes = arvore.DFS(ver_to_use);

            if (!impressoes.empty()) {
                for (size_t i = 0; i < impressoes.size(); ++i) {
                    fout << impressoes[i].first << "," << impressoes[i].second; 
                    if (i < impressoes.size() - 1) {
                        fout << " "; 
                    }
                }
            }
            fout << "\n"; 
        }
    }

    fout.close();
    return 0;
}