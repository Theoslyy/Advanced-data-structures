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
    int ver;   // versao na qual a modificacao foi aplicada
    field campo; // campo a ser modificado
    int modChave;
    Node* modPointer;

    Mod() : ver(-1), campo(nenhum), modChave(0), modPointer(nullptr) {}
};

struct Node {
    Node *esq;    // ponteiro filho esquerdo
    Node *dir;    // ponteiro filho direito
    Node *pai;    // ponteiro pai!
    Mod mods;     // slot para uma modificação (fat node)
    int chave;    //
    bool isRoot;  
    int ver;      // versão do nó físico

    // como a árvore binária de busca já mantém ponteiros para o nó
    // esquerdo e direito, não precisamos do vetor de retorno, já temos os ponteiros,
    // então, basta anotar mudanças nesses ponteiros como mods
    Node() // essa forma de inicializar as coisa é tão bonita...
      : esq(nullptr), dir(nullptr), pai(nullptr),
        chave(0), mods(), isRoot(false), ver(0) {}
};

struct BST {
    Node *root;              // raiz da versão vers - 1. vers é a 'versão a ser criada' na próx mod.
    vector<Node*> nodes;     // Todos os nós criados, para gerenciamento de memória (opcional)
    vector<Node*> versions;  // -> não precisa ser uma tupla de (int, node), o índice do vetor já dá em que versão estamos
    int vers;                // desnecessário, é só ver o tamanho do vetor versions, mas ajuda na organização.

    BST()
      : root(nullptr), vers(0) {
        versions.push_back(nullptr);  // Versão 0: árvore vazia (inicial)
    }

    // ------------------------------------------------------------------------
    // Aplica as modificações de um nó `atual` para materializar em `temp`
    void aplicaMods(Node *atual, Node *temp, int version_query){
        // Copia o estado base para o nó temporário
        temp->chave = atual->chave;
        temp->esq = atual->esq;
        temp->dir = atual->dir;
        temp->pai = atual->pai;
        temp->isRoot = atual->isRoot;

        // Se o slot de modificação do original_node for relevante para a versão_query, aplica-o
        if (atual->mods.ver != -1 && atual->mods.ver <= version_query) {
            switch (atual->mods.campo) {
                case esq: temp->esq = atual->mods.modPointer; break;
                case dir: temp->dir = atual->mods.modPointer; break;
                case pai: temp->pai = atual->mods.modPointer; break;
                case chave: temp->chave = atual->mods.modChave; break;
                default: break;
            }
        }
    }

    // ------------------------------------------------------------------------
    // Se o nó já tem um mod nessa versão, criamos cópia e aplicamos em cadeia.
    // Caso contrário, gravamos diretamente o Mod nele. Retorna o nó modificado (original ou cópia).
    Node* atualizaCadeia(Node *no, int versao_base, Mod modificacao_nova){
        // essa função só é chamada quando `no->mods.ver == versao_base`
        Node *no_novo = new Node();
        Node temp_original; // para materializar o estado do nó original

        // materializa o estado atual até versao_base
        aplicaMods(no, &temp_original, versao_base);

        // copia a parte física
        *no_novo = temp_original;
        no_novo->mods = Mod();          // limpa qualquer mod prévio
        no_novo->mods = modificacao_nova;   // grava o mod novo em no_novo
        no_novo->ver  = modificacao_nova.ver;
        nodes.push_back(no_novo);

        // Se no_novo agora é raiz, ajustar:
        if (no_novo->isRoot) {
            no_novo->pai = nullptr;
            root = no_novo;
        }

        // Agora ajusta qualquer ponteiro que apontava para `no` na versão anterior:
        if (temp_original.pai != nullptr) {
            Node tempPaiNo;
            aplicaMods(temp_original.pai, &tempPaiNo, versao_base);
            if (tempPaiNo.esq == no) {
                modificar(temp_original.pai, esq, NONE, no_novo, versao_base+1);
            }
            else if (tempPaiNo.dir == no) {
                modificar(temp_original.pai, dir, NONE, no_novo, versao_base+1);
            }
        }

        // Ajusta filhos do nó original, apontando pai para a cópia:
        if (temp_original.esq != nullptr) {
            modificar(temp_original.esq, pai, NONE, no_novo, versao_base+1);
        }
        if (temp_original.dir != nullptr) {
            modificar(temp_original.dir, pai, NONE, no_novo, versao_base+1);
        }

        return no_novo;
    }

    // ------------------------------------------------------------------------
    // Esta função decide se uma modificação é aplicada diretamente ou se uma cópia é necessária.
    // Retorna o nó (original ou uma nova cópia) que foi modificado e que agora pertence à versao_alvo.
    Node* modificar(Node* no, field campo, int valor_chave, Node* valor_no, int versao_alvo) {
        if (no == nullptr) return nullptr;

        // 1) Se o nó foi criado exatamente nesta versão, atualizamos diretamente
        if (no->ver == versao_alvo) {
            switch (campo) {
                case esq:    no->esq   = valor_no; break;
                case dir:    no->dir   = valor_no; break;
                case pai:    no->pai   = valor_no; break;
                case chave: no->chave = valor_chave; break;
                case raiz:
                    no->isRoot = valor_chave;
                    if (no->isRoot) no->pai = nullptr;
                    break;
                default:    break;
            }
            return no;
        }

        Mod m;
        m.ver        = versao_alvo;
        m.campo      = campo;
        m.modChave   = valor_chave;
        m.modPointer = valor_no;

        // 2) Se ainda não há mod nesta versão, gravamos
        if (no->mods.campo == nenhum || no->mods.ver < versao_alvo) {
            no->mods = m;
            // se virou raiz, ajusta:
            if (campo == raiz && valor_chave == 1) {
                root = no;
                no->pai = nullptr;
            }
            return no;
        }

        // 3) Já havia mod nesta mesma versão: copy‐on‐write
        //    (atualizaCadeia faz a cópia física, propaga os ponteiros e já
        //     aplica este novo Mod na cópia)
        return atualizaCadeia(no, no->mods.ver, m);
    }

    // ------------------------------------------------------------------------
    // Insere 'k' a partir de version_base, criando versão (vers+1) ao final.
    void inserir(int k, int version_base) {
        // 'version_base' é a versão da qual estamos partindo para criar a nova versão
        root = versions[version_base];

        Node *no_novo = new Node();
        no_novo->chave = k;
        no_novo->isRoot = false;
        no_novo->ver = version_base+1;
        nodes.push_back(no_novo);

        if (root == nullptr) { // Árvore base está vazia
            // O nó novo se torna a raiz da nova versão
            no_novo->isRoot = true;
            root = no_novo;
        } else {
            Node *no_atual = root;
            Node *no_anterior = nullptr;
            Node temp;

            // percorre a versão materializada até achar posição
            while (no_atual != nullptr) {
                aplicaMods(no_atual, &temp, version_base);
                no_anterior = no_atual;
                if (k < temp.chave)
                    no_atual = temp.esq;
                else
                    no_atual = temp.dir;
            }

            // no_anterior é o pai onde anexaremos no_novo
            Node tempPai;
            aplicaMods(no_anterior, &tempPai, version_base);

            Node *pai_modificado;
            if (k < tempPai.chave) {
                pai_modificado = modificar(no_anterior, esq, NONE, no_novo, version_base+1);
            } else {
                pai_modificado = modificar(no_anterior, dir, NONE, no_novo, version_base+1);
            }
            // Ajusta pai do novo nó
            modificar(no_novo, pai, NONE, pai_modificado, version_base+1);
        }

        versions.push_back(root);
        vers++;
    }

    // ------------------------------------------------------------------------
    Node *minimo(Node *no, int ver){
        if (no == nullptr) return nullptr;
        Node temp;
        aplicaMods(no, &temp, ver);
        Node* current_node = no;
        while (temp.esq != nullptr){
            current_node = temp.esq;
            aplicaMods(current_node, &temp, ver);
        }
        return current_node;
    }

    // ------------------------------------------------------------------------
    // Retorna o nodo que contém o menor valor > 'chave', na versão 'ver'.
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
    // Retorna o sucessor de um nó específico na versão 'ver'.
    // Materializa primeiro o nó 'no' em 'ver' para obter sua chave,
    // e então delega à versão que recebe (raiz, chave, ver).
    Node* sucessor(Node* no, int ver) {
        if (no == nullptr) return nullptr;
        Node temp;
        aplicaMods(no, &temp, ver);
        // usa a raiz da versão 'ver' e a chave materializada
        return sucessor(versions[ver], temp.chave, ver);
    }

    // ------------------------------------------------------------------------
    // Função auxiliar para transplantar subárvores: 'u' é substituído por 'v'
    // ------------------------------------------------------------------------
    // Transplanta o nó 'v' para o lugar de 'u' na nova versão (versao_base+1).
    // Retorna o novo nó 'vlinha' que ocupa a posição de 'u'.
    Node* transplantar(BST *tree, Node* u, Node* v, int versao_base) {
        Node tempU;
        aplicaMods(u, &tempU, versao_base);

        if (tempU.isRoot) {
            if (!v) {
                tree->root = nullptr;
                return nullptr;
            }
            Node *vlinha = new Node();
            Node tempV; aplicaMods(v, &tempV, versao_base);
            *vlinha = tempV;
            vlinha->mods = Mod();
            vlinha->ver  = versao_base + 1;
            vlinha->isRoot = true;
            vlinha->pai = nullptr;
            tree->nodes.push_back(vlinha);
            tree->root = vlinha;
            return vlinha;
        }

        Node *pai_u = tempU.pai;
        Node *vlinha = nullptr;
        if (v) {
            vlinha = new Node();
            Node tempV; aplicaMods(v, &tempV, versao_base);
            *vlinha = tempV;
            vlinha->mods = Mod();
            vlinha->ver  = versao_base + 1;
            vlinha->isRoot = false;
            vlinha->pai = pai_u;
            tree->nodes.push_back(vlinha);
        }

        Node tempPai; aplicaMods(pai_u, &tempPai, versao_base);
        if (tempPai.esq == u) {
            modificar(pai_u, esq, NONE, vlinha, versao_base + 1);
        } else {
            modificar(pai_u, dir, NONE, vlinha, versao_base + 1);
        }

        return vlinha;
    }

    // ------------------------------------------------------------------------
    // Remove a chave ‘chave’ a partir de versao_base, gerando versão (vers+1).
    void remover(int chave, int versao_base) {
        // 1) inicializa root para a versão base
        root = versions[versao_base];

        // 2) árvore vazia na versão base?
        if (!root) {
            versions.push_back(nullptr);
            vers++;
            return;
        }

        // 3) busca o nó-alvo na versão base
        Node* alvo = buscaVersao(root, chave, versao_base);
        if (!alvo) {
            versions.push_back(root);
            vers++;
            return;
        }

        // 4) materializa o alvo na versão base
        Node tempAlvo;
        aplicaMods(alvo, &tempAlvo, versao_base);

        // 5) caso 1: sem filho esquerdo → substitui por filho direito
        if (!tempAlvo.esq) {
            // transplantar retorna o nó físico que tomou o lugar de 'alvo'
            transplantar(this, alvo, tempAlvo.dir, versao_base);
        }
        // 6) caso 2: sem filho direito → substitui por filho esquerdo
        else if (!tempAlvo.dir) {
            transplantar(this, alvo, tempAlvo.esq, versao_base);
        }
        // 7) caso 3: dois filhos
        else {
            // 7a) encontra sucessor na subárvore direita
            Node* succ = minimo(tempAlvo.dir, versao_base);
            Node tempSucc;
            aplicaMods(succ, &tempSucc, versao_base);

            // 7b) se o sucessor não é filho direto de 'alvo', retira-o primeiro
            if (tempSucc.pai != alvo) {
                transplantar(this, succ, tempSucc.dir, versao_base);
                // o filho direito original de 'alvo' agora passa a ser filho direito de 'succ'
                modificar(succ, dir, NONE, tempAlvo.dir, versao_base+1);
                // ajusta pai do subtree direito antigo
                modificar(tempAlvo.dir, pai, NONE, succ, versao_base+1);
            }

            // 7c) finalmente, substitui 'alvo' por 'succ' (ou sua cópia)
            Node* succ_prime = transplantar(this, alvo, succ, versao_base);
            // reconecta o subtree esquerdo de 'alvo' em 'succ_prime'
            modificar(succ_prime, esq, NONE, tempAlvo.esq, versao_base+1);
            // ajusta pai do subtree esquerdo antigo
            modificar(tempAlvo.esq, pai, NONE, succ_prime, versao_base+1);
        }

        // 8) registra a nova raiz
        versions.push_back(root);
        vers++;
    }


    // ------------------------------------------------------------------------
    Node *busca(int k, int versao_query){
        if (versao_query < 0 || versao_query >= (int)versions.size()) {
            return nullptr;
        }
        return buscaVersao(versions[versao_query], k, versao_query);
    }

    Node* buscaVersao(Node* no, int k, int versao_query){
        if (no == nullptr) return nullptr;
        Node temp;
        aplicaMods(no, &temp, versao_query);
        if (k == temp.chave) return no;
        else if (k < temp.chave) return buscaVersao(temp.esq, k, versao_query);
        else return buscaVersao(temp.dir, k, versao_query);
    }

    // ------------------------------------------------------------------------
    int get_key(Node* node, int version_query){
        Node temp;
        aplicaMods(node, &temp, version_query);
        return temp.chave;
    }

    // ------------------------------------------------------------------------
    void DFS_REC(Node* node, int version_query, int profundidade, vector<pair<int,int>>& dfs_vector){
        if(node == nullptr) return;
        Node temp;
        aplicaMods(node, &temp, version_query);
        DFS_REC(temp.esq, version_query, profundidade + 1, dfs_vector);
        dfs_vector.push_back({temp.chave, profundidade});
        DFS_REC(temp.dir, version_query, profundidade + 1, dfs_vector);
    }

    vector<pair<int,int>> DFS(int version_query){
        if (version_query < 0 || version_query >= (int)versions.size())
            version_query = versions.size() - 1;
        Node* root_to_dfs = versions[version_query];
        vector<pair<int,int>> dfs_vector;
        DFS_REC(root_to_dfs, version_query, 0, dfs_vector);
        return dfs_vector;
    }

    void imprimir(Node *no){
        if (no != nullptr){
            imprimir(no->esq);
            cout << no->chave << " ";
            imprimir(no->dir);
        }
    }
};