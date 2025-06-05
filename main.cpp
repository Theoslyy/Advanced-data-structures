#include "parser.hpp"
#include "BST.cpp"
#include <fstream>
#include <iostream>

using namespace std;

int main() {
    string data_path;
    cin >> data_path;

    vector<operation> v = parser_text(data_path);
    BST arvore;

    ofstream fout("out.txt"); // Saída conforme descrição

    for (operation op : v) {
        command tipo = get<0>(op);
        int chave = get<1>(op);
        int versao = get<2>(op);

        if (tipo == INC) {
            arvore.inserir(&arvore, chave, arvore.vers);
        }
        else if (tipo == REM) {
            arvore.remover(&arvore, chave, arvore.vers);
        }
        else if (tipo == SUC) {
            int max_version = arvore.versions.size() - 1;
            int ver = (versao < arvore.versions.size()) ? versao : max_version;

            Node* atual = arvore.busca(&arvore, chave, ver);
            Node* suc = arvore.sucessor(atual, ver);

            fout << "SUC " << chave << " " << versao << "\n";
            if (suc != nullptr) {
                int val = arvore.get_key(suc, ver);
                fout << val << "\n";
            } else {
                fout << "infinito\n";
            }
        }
        else if (tipo == IMP) {
            int max_version = arvore.versions.size() - 1;
            int ver = (versao < arvore.versions.size()) ? versao : max_version;

            fout << "IMP " << versao << "\n";
            vector<pair<int, int>> impressoes = arvore.DFS(ver);

            for (auto [chave, profundidade] : impressoes) {
                fout << chave << "," << profundidade << " ";
            }
            fout << "\n";
        }
    }

    fout.close();
    return 0;
}