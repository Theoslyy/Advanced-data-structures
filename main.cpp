#include "parser.hpp" // Assumindo que parser.hpp existe e define 'operation' e 'command'
#include "BST.cpp"   // Inclui o arquivo BST.cpp com a implementação
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <tuple> // Para get<>

using namespace std;

int main() {
    string data_path;
    cin >> data_path;

    vector<operation> v = parser_text(data_path);
    BST arvore;

    ofstream fout("out.txt"); // Saída para o arquivo

    for (operation op : v) {
        command tipo = get<0>(op);
        int chave = get<1>(op);
        int versao_input = get<2>(op); // Versão fornecida no input, para SUC e IMP

        if (tipo == INC) {
            // Para persistência parcial, a operação sempre parte da última versão existente.
            int versao_base = arvore.versions.size() - 1;
            arvore.inserir(chave, versao_base);
        }
        else if (tipo == REM) {
            // Para persistência parcial, a operação sempre parte da última versão existente.
            int versao_base = arvore.versions.size() - 1;
            arvore.remover(chave, versao_base);
        }
        else if (tipo == SUC) {
            int max_version_idx = arvore.versions.size() - 1;
            int ver_to_use; // Versão real a ser usada na busca

            // Define a versão a ser usada para busca
            if (versao_input >= 0 && versao_input <= max_version_idx) {
                ver_to_use = versao_input;
            } else {
                ver_to_use = max_version_idx; // Usa a versão mais recente se a solicitada não existe
            }

            Node* atual = arvore.busca(chave, ver_to_use);
            Node* suc = arvore.sucessor(atual, ver_to_use);

            fout << "SUC " << chave << " " << versao_input << "\n"; // Imprime a versão ORIGINAL do input
            if (suc != nullptr) {
                // Para obter a chave de um nó em uma versão específica, usamos get_key
                fout << arvore.get_key(suc, ver_to_use) << "\n";
            } else {
                fout << "infinito\n";
            }
        }
        else if (tipo == IMP) {
            int max_version_idx = arvore.versions.size() - 1;
            int ver_to_use; // Versão real a ser usada na DFS

            // Define a versão a ser usada para impressão
            if (versao_input >= 0 && versao_input <= max_version_idx) {
                ver_to_use = versao_input;
            } else {
                ver_to_use = max_version_idx; // Usa a versão mais recente se a solicitada não existe
            }

            fout << "IMP " << versao_input << "\n"; // IMPRIME A VERSÃO ORIGINAL DO INPUT

            auto impressoes = arvore.DFS(ver_to_use);

            if (impressoes.empty()) {
                fout << "\n";  // Árvore vazia, imprime só a linha da operação e uma linha vazia
            } else {
                for (size_t i = 0; i < impressoes.size(); ++i) {
                    fout << impressoes[i].first << "," << impressoes[i].second;
                    if (i < impressoes.size() - 1) {
                        fout << " "; // Adiciona espaço entre os elementos
                    }
                }
                fout << "\n";
            }
        }
    }

    fout.close();
    return 0;
}