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

    for (operation op : v) {
        command tipo = get<0>(op);
        int chave = get<1>(op);
        int versao_input = get<2>(op);

        if (tipo == INC) {
            // Insere na versão mais recente existente (arvore.versions.size() - 1).
            int versao_base = arvore.versions.size() - 1; 
            arvore.inserir(&arvore, chave, versao_base); 
        }
        else if (tipo == REM) {
            // Remove na versão mais recente existente (arvore.versions.size() - 1).
            int versao_base = arvore.versions.size() - 1;
            arvore.remover(&arvore, chave, versao_base); 
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

            Node* atual = arvore.busca(&arvore, chave, ver_to_use);
            Node* suc = arvore.sucessor(atual, ver_to_use);

            fout << "SUC " << chave << " " << versao_input << "\n"; // Imprime a versão ORIGINAL do input
            if (suc != nullptr) {
                int val = arvore.get_key(suc, ver_to_use);
                fout << val << "\n";
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
                        fout << " "; 
                    }
                }
                fout << "\n"; 
            }
        }
    }

    fout.close();
    return 0;
}