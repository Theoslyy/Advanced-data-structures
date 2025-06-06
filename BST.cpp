#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define NONE -10000

using namespace std;
struct Node; struct Mod;

enum field {
    esq, dir, pai, chave, raiz, nenhum
};

struct Mod {
    int ver;        // versão na qual a modificação foi aplicada
    field campo;    // campo a ser modificado
    int modChave;
    Node* modPointer;

    Mod() : ver(-1), campo(nenhum), modChave(0), modPointer(nullptr) {}
};

struct Node {
    Node *esq;      // ponteiro filho esquerdo
    Node *dir;      // ponteiro filho direito
    Node *pai;      // ponteiro pai!
    Mod mods;       // <- como há no máximo 1 mod por nó, uso só uma variável
    int chave;      //
    bool isRoot;

    Node()          // inicializa tudo em nulo/zero
      : esq(nullptr), dir(nullptr), pai(nullptr),
        chave(0), mods(), isRoot(false) {}
};

struct BST {
    Node *root;              // raiz da versão (vers-1); a próxima mod criará vers
    vector<Node*> nodes;     // todos os nós alocados, só para ter controle
    vector<Node*> versions;  // versions[i] = ponteiro para a raiz da versão i
    int vers;                // contador de versões (vai de 0 a até ~100)

    BST()
      : root(nullptr), vers(0) {
        versions.push_back(nullptr);  // versão 0 = árvore vazia
    }

    // ------------------------------------------------------------------------
    // Se o nó já tem um mod nessa versão, criamos cópia e aplicamos em cadeia.
    // Caso contrário, gravamos diretamente o Mod nele. Retorna o nó modificado (original ou cópia).
    Node* atualizaCadeia(BST* tree, Node *no, int versao_base, Mod modificacao_nova) {
        // essa função só é chamada quando `no->mods.ver == versao_base`
        Node *no_novo = new Node();
        Node temp_original; // para materializar o estado do nó original

        // Se já havia um mod exato para essa mesma versão, materializa primeiro:
        if (no->mods.campo != nenhum && no->mods.ver == versao_base) {
            aplicaMods(no, &temp_original, versao_base);
        } else {
            temp_original = *no; 
        }
        *no_novo = temp_original;
        no_novo->mods = Mod();              // limpa qualquer mod prévio
        no_novo->mods = modificacao_nova;   // grava o mod novo em no_novo

        tree->nodes.push_back(no_novo);

        // Se no_novo agora é raiz, ajustar:
        if (no_novo->isRoot) {
            no_novo->pai = nullptr;
            tree->root = no_novo;
        }

        // Agora ajusta qualquer ponteiro que apontava para `no` na versão anterior:
        if (temp_original.pai != nullptr) {
            Node tempPaiNo;
            aplicaMods(temp_original.pai, &tempPaiNo, versao_base);
            if (tempPaiNo.esq == no) {
                modificar(tree, temp_original.pai, tree->vers, esq, NONE, no_novo);
            }
            else if (tempPaiNo.dir == no) {
                modificar(tree, temp_original.pai, tree->vers, dir, NONE, no_novo);
            }
        }
        else if (no->isRoot) {
            tree->root = no_novo;
        }

        // Ajusta filhos do nó original, apontando pai para a cópia:
        if (temp_original.esq != nullptr) {
            modificar(tree, temp_original.esq, tree->vers, pai, NONE, no_novo);
        }
        if (temp_original.dir != nullptr) {
            modificar(tree, temp_original.dir, tree->vers, pai, NONE, no_novo);
        }

        return no_novo;
    }

    // ------------------------------------------------------------------------
    // Aplica um Mod em 'no', mas se já existe um Mod para essa mesma versão,
    // chama atualizaCadeia(...) para criar cópia em cadeia.
    // Retorna o nó modificado (original ou cópia).
    Node* modificar(BST *tree, Node *no, int versao_alvo, field campo, int valor_chave = NONE, Node* valor_no = nullptr) {
        if (no == nullptr) return nullptr;

        Mod modifica;
        modifica.ver = versao_alvo;
        modifica.campo = campo;
        modifica.modChave = valor_chave;
        modifica.modPointer = valor_no;

        // Se ainda não há mod nessa versão, grava diretamente:
        if (no->mods.campo == nenhum || no->mods.ver < versao_alvo) {
            no->mods = modifica;
            if (campo == raiz && valor_chave == 1) {
                tree->root = no;
                no->pai = nullptr; // raiz não tem pai
            }
            return no;
        }
        // Senão, já há um mod nesta mesma versão: copiamos em cadeia
        else {
            return atualizaCadeia(tree, no, no->mods.ver, modifica);
        }
    }

    // ------------------------------------------------------------------------
    // “Materializa” o estado de um nó na versão 'version_query'
    void aplicaMods(Node *atual, Node *temp, int version_query) {
        *temp = *atual; // copia estado base
        if ((atual->mods.campo != nenhum) && (atual->mods.ver <= version_query)) {
            switch (atual->mods.campo) {
                case esq:
                    temp->esq = atual->mods.modPointer;
                    break;
                case dir:
                    temp->dir = atual->mods.modPointer;
                    break;
                case pai:
                    temp->pai = atual->mods.modPointer;
                    break;
                case raiz:
                    temp->isRoot = atual->mods.modChave;
                    if (temp->isRoot) temp->pai = nullptr;
                    break;
                case chave:
                    temp->chave = atual->mods.modChave;
                    break;
                default:
                    break;
            }
        }
    }

    // ------------------------------------------------------------------------
    // Insere 'k' a partir de version_base, criando versão (vers+1) ao final.
    void inserir(BST *tree, int k, int version_base) {
        Node *raiz_base = tree->versions[version_base];
        tree->root = raiz_base;

        Node *no_novo = new Node();
        no_novo->chave = k;
        tree->nodes.push_back(no_novo);

        if (raiz_base == nullptr) {
            // Se vazio, novo nó vira raiz da nova versão
            Node *modificado = modificar(tree, no_novo, tree->vers, raiz, 1, nullptr);
            tree->root = modificado;
        } else {
            Node *no_atual = raiz_base;
            Node *no_anterior = nullptr;
            Node temp;

            // Percorre a versão “materializada” até achar posição:
            while (no_atual != nullptr) {
                aplicaMods(no_atual, &temp, version_base);
                no_anterior = no_atual;
                if (k < temp.chave) {
                    no_atual = temp.esq;
                } else {
                    no_atual = temp.dir;
                }
            }

            // “no_anterior” é o pai onde anexamos
            Node tempPai;
            aplicaMods(no_anterior, &tempPai, version_base);

            Node *pai_modificado;
            if (k < tempPai.chave) {
                pai_modificado = modificar(tree, no_anterior, tree->vers, esq, NONE, no_novo);
            } else {
                pai_modificado = modificar(tree, no_anterior, tree->vers, dir, NONE, no_novo);
            }

            // Se no_anterior era raiz na versão anterior, atualizar tree->root:
            if (no_anterior->isRoot) {
                tree->root = pai_modificado;
            }
            // Ajusta pai do no_novo
            modificar(tree, no_novo, tree->vers, pai, NONE, pai_modificado);
        }

        tree->versions.push_back(tree->root);
        tree->vers++;
    }

    // ------------------------------------------------------------------------
    Node *minimo(Node *no, int ver) {
        if (no == nullptr) return nullptr;
        Node no_temp;
        aplicaMods(no, &no_temp, ver);
        Node *current_node = no;
        while (no_temp.esq != nullptr) {
            current_node = no_temp.esq;
            aplicaMods(current_node, &no_temp, ver);
        }
        return current_node;
    }

    // ------------------------------------------------------------------------
    // Retorna o nó na versão 'ver' que contém o menor valor > 'chave'.
    // Começa a busca pela raiz passada em 'raiz'.
    Node* sucessor(Node* raiz, int chave, int ver) {
        Node* candidato = nullptr;
        Node* atual = raiz;
        Node temp;

        while (atual != nullptr) {
            aplicaMods(atual, &temp, ver);
            if (temp.chave > chave) {
                candidato = atual;
                atual = temp.esq;
            } else {
                atual = temp.dir;
            }
        }
        return candidato;
    }

    // ------------------------------------------------------------------------
    void transplantar(BST *tree, Node* u, Node* v, int versao_base) {
        Node tempU;
        aplicaMods(u, &tempU, versao_base);

        // se u for raiz, então v também será raiz
        if (tempU.isRoot) {
            if (v != nullptr) {
                modificar(tree, v, tree->vers, raiz, 1, nullptr);
            } else {
                tree->root = nullptr;
            }
        } else {
            Node tempPai;
            aplicaMods(tempU.pai, &tempPai, versao_base);
            Node* pai_u_modificado = modificar(tree,
                                               tempU.pai,
                                               tree->vers,
                                               pai,
                                               NONE,
                                               tempU.pai);
            if (tempPai.esq == u) {
                modificar(tree, pai_u_modificado, tree->vers, esq, NONE, v);
            } else if (tempPai.dir == u) {
                modificar(tree, pai_u_modificado, tree->vers, dir, NONE, v);
            }
            if (v != nullptr) {
                modificar(tree, v, tree->vers, pai, NONE, pai_u_modificado);
            }
        }
    }

    // ------------------------------------------------------------------------
    void remover(BST *tree, int chave, int version_base) {
        Node *raiz_base = tree->versions[version_base];
        tree->root = raiz_base;

        if (raiz_base == nullptr) {
            tree->versions.push_back(nullptr);
            tree->vers++;
            return;
        }

        Node *alvo = buscaVersao(raiz_base, chave, version_base);
        if (!alvo) {
            tree->versions.push_back(raiz_base);
            tree->vers++;
            return;
        }

        Node tempAlvo;
        aplicaMods(alvo, &tempAlvo, version_base);

        // CASO 1: sem filho esquerdo, mas com direito
        if (tempAlvo.esq == nullptr && tempAlvo.dir != nullptr) {
            transplantar(tree, alvo, tempAlvo.dir, version_base);
        }
        // CASO 2: sem filho direito, mas com esquerdo
        else if (tempAlvo.dir == nullptr && tempAlvo.esq != nullptr) {
            transplantar(tree, alvo, tempAlvo.esq, version_base);
        }
        // CASO 3: nó folha (sem filhos)
        else if (tempAlvo.esq == nullptr && tempAlvo.dir == nullptr) {
            if (tempAlvo.pai == nullptr) {
                // único nó da árvore → nova versão vazia
                tree->versions.push_back(nullptr);
                tree->vers++;
                return;
            } else {
                Node tempPai;
                aplicaMods(tempAlvo.pai, &tempPai, version_base);
                if (tempPai.esq == alvo) {
                    Node *pai_mod = modificar(tree, tempAlvo.pai, tree->vers, esq, NONE, nullptr);
                    if (tempAlvo.pai->isRoot) {
                        tree->root = pai_mod;
                    }
                } else {
                    Node *pai_mod = modificar(tree, tempAlvo.pai, tree->vers, dir, NONE, nullptr);
                    if (tempAlvo.pai->isRoot) {
                        tree->root = pai_mod;
                    }
                }
                tree->versions.push_back(tree->root);
                tree->vers++;
                return;
            }
        }
        // CASO 4: nó com dois filhos → substitua pelo sucessor
        else {
            Node *succ = minimo(tempAlvo.dir, version_base);
            Node tempSucc;
            aplicaMods(succ, &tempSucc, version_base);

            if (tempSucc.pai != alvo) {
                transplantar(tree, succ, tempSucc.dir, version_base);
                if (tempSucc.dir != nullptr) {
                    Node *mod_dir = modificar(tree,
                                              tempSucc.dir,
                                              tree->vers,
                                              pai,
                                              NONE,
                                              succ);
                    if (tempSucc.dir->isRoot) {
                        tree->root = mod_dir;
                    }
                }
            }

            transplantar(tree, alvo, succ, version_base);
            if (alvo->isRoot) {
                tree->root = succ;
            }

            Node *succ_e = modificar(tree, succ, tree->vers, esq, NONE, tempAlvo.esq);
            if (succ->isRoot) {
                tree->root = succ_e;
            }
            Node *succ_d = modificar(tree, succ_e, tree->vers, dir, NONE, tempAlvo.dir);
            if (succ_e->isRoot) {
                tree->root = succ_d;
            }

            if (tempAlvo.esq != nullptr) {
                Node *mod_esq = modificar(tree,
                                          tempAlvo.esq,
                                          tree->vers,
                                          pai,
                                          NONE,
                                          succ_d);
                if (tempAlvo.esq->isRoot) {
                    tree->root = mod_esq;
                }
            }
            if (tempAlvo.dir != nullptr && tempAlvo.dir != succ) {
                Node *mod_dir2 = modificar(tree,
                                           tempAlvo.dir,
                                           tree->vers,
                                           pai,
                                           NONE,
                                           succ_d);
                if (tempAlvo.dir->isRoot) {
                    tree->root = mod_dir2;
                }
            }
        }

        tree->versions.push_back(tree->root);
        tree->vers++;
    }

    // ------------------------------------------------------------------------
    Node *busca(BST* tree, int k, int versao) {
        if (versao < 0 || versao >= (int)tree->versions.size()) {
            return nullptr;
        }
        return buscaVersao(tree->versions[versao], k, versao);
    }

    Node* buscaVersao(Node* no, int k, int versao) {
        if (no == nullptr) return nullptr;
        Node temp;
        aplicaMods(no, &temp, versao);
        if (k == temp.chave) {
            return no;
        } else if (k < temp.chave) {
            return buscaVersao(temp.esq, k, versao);
        } else {
            return buscaVersao(temp.dir, k, versao);
        }
    }

    // ------------------------------------------------------------------------
    int get_key(Node* node, int version_query) {
        Node temp;
        aplicaMods(node, &temp, version_query);
        return temp.chave;
    }

    // ------------------------------------------------------------------------
    void DFS_REC(Node* node,
                 int version_query,
                 int profundidade,
                 vector<pair<int,int>>& dfs_vector) {
        if (node == nullptr) return;
        Node temp;
        aplicaMods(node, &temp, version_query);
        DFS_REC(temp.esq, version_query, profundidade + 1, dfs_vector);
        dfs_vector.push_back({ temp.chave, profundidade });
        DFS_REC(temp.dir, version_query, profundidade + 1, dfs_vector);
    }

    vector<pair<int,int>> DFS(int version_query) {
        if (version_query < 0 || version_query >= (int)versions.size()) {
            version_query = versions.size() - 1;
        }
        Node* root_to_dfs = versions[version_query];
        vector<pair<int,int>> dfs_vector;
        DFS_REC(root_to_dfs, version_query, 0, dfs_vector);
        return dfs_vector;
    }

    void imprimir(Node *no) {
        if (no != nullptr) {
            imprimir(no->esq);
            cout << no->chave << " ";
            imprimir(no->dir);
        }
    }
};
