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
        command tipo    = get<0>(op);
        int chave       = get<1>(op);
        int versao_input= get<2>(op);

        if (tipo == INC) {
            int versao_base = arvore.versions.size() - 1;
            arvore.inserir(&arvore, chave, versao_base);
        }
        else if (tipo == REM) {
            int versao_base = arvore.versions.size() - 1;
            arvore.remover(&arvore, chave, versao_base);
        }
        else if (tipo == SUC) {
            int max_version_idx = arvore.versions.size() - 1;
            int ver_to_use;
            if (versao_input >= 0 && versao_input <= max_version_idx) {
                ver_to_use = versao_input;
            } else {
                ver_to_use = max_version_idx;
            }

            // ** Usamos a raiz materializada da versão ver_to_use **
            Node* raiz_da_versao = arvore.versions[ver_to_use];
            fout << "SUC " << chave << " " << versao_input << "\n";

            Node* suc = arvore.sucessor(raiz_da_versao, chave, ver_to_use);
            if (suc != nullptr) {
                int val = arvore.get_key(suc, ver_to_use);
                fout << val << "\n";
            } else {
                fout << "infinito\n";
            }
        }
        else if (tipo == IMP) {
            int max_version_idx = arvore.versions.size() - 1;
            int ver_to_use;
            if (versao_input >= 0 && versao_input <= max_version_idx) {
                ver_to_use = versao_input;
            } else {
                ver_to_use = max_version_idx;
            }

            fout << "IMP " << versao_input << "\n";
            auto impressoes = arvore.DFS(ver_to_use);
            if (impressoes.empty()) {
                fout << "\n";
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
