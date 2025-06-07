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
    esq, dir, pai, chave, nenhum
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

    Node(int k = 0) // Construtor com chave padrão
    : esq(nullptr), dir(nullptr), pai(nullptr),
      chave(k), mods() {}

    // Construtor de cópia para criar um novo nó físico
    Node(const Node& other)
    : esq(other.esq), dir(other.dir), pai(other.pai),
      mods(), // A cópia começa sem modificações na sua própria Mod
      chave(other.chave) {}
};

struct BST {
    Node *root; // Raiz da *versão atualmente em construção*.
    vector<Node*> nodes; // Todos os nós criados, para gerenciamento de memória.
    vector<Node*> versions; // Raízes de todas as versões.
    int vers; // Índice da PRÓXIMA versão a ser criada (ou número total de versões).

    BST()
    :   root(nullptr), vers(0) {
        versions.push_back(nullptr);  // Versão 0: árvore vazia (inicial)
    }

    // Destrutor para liberar a memória alocada para os nós.
    ~BST() {
        for (Node* node : nodes) {
            delete node;
        }
    }

    // Materializa o estado de um nó 'original' para a 'version_query' e o copia para 'temp'.
    void aplicaMods(Node *original, Node *temp, int version_query){
        // Copia o estado base do nó 'original' para 'temp'
        temp->esq = original->esq;
        temp->dir = original->dir;
        temp->pai = original->pai;
        temp->chave = original->chave;

        // Se o nó original tem uma modificação relevante para a 'version_query', aplica-a
        // A modificação é relevante se a versão dela é menor ou igual à versão da consulta
        // e se é a última modificação do nó para essa versão.
        if (original->mods.campo != nenhum && original->mods.ver <= version_query) {
            switch (original->mods.campo){
                case esq: temp->esq = original->mods.modPointer; break;
                case dir: temp->dir = original->mods.modPointer; break;
                case pai: temp->pai = original->mods.modPointer; break;
                case chave: temp->chave = original->mods.modChave; break;
                default: break;
            }
        }
    }

    // Função que cria uma nova cópia de um nó e aplica modificações.
    // Esta função é chamada quando um nó já tem uma modificação para a versão atual,
    // e precisa ser copiado para uma nova modificação na mesma versão.
    Node* atualizaCadeia(Node *no_original, Mod nova_modificacao, int versao_da_nova_arvore){
        // 1. Criar nova cópia física deste nó
        Node *no_novo = new Node(*no_original); // Usa o construtor de cópia
        nodes.push_back(no_novo); // Adiciona ao vetor de todos os nós

        // 2. APLICAR A NOVA MODIFICAÇÃO no no_novo (a cópia)
        no_novo->mods = nova_modificacao; // A cópia agora tem a nova modificação

        // 3. Propagar a mudança para o PARENT (se existir)
        // Se o no_original não tinha pai, e agora a cópia é a nova raiz, atualiza tree->root
        Node temp_original_materialized;
        aplicaMods(no_original, &temp_original_materialized, versao_da_nova_arvore - 1); // Materializa o original para ver seu pai

        if (temp_original_materialized.pai == nullptr) { // no_original era a raiz
            root = no_novo; // no_novo é a nova raiz da versão atual
            no_novo->pai = nullptr; // A nova raiz não tem pai
        } else {
            Node temp_pai_original;
            aplicaMods(temp_original_materialized.pai, &temp_pai_original, versao_da_nova_arvore - 1); // Materializa o pai do original

            if (temp_pai_original.esq == no_original) {
                modificar(temp_original_materialized.pai, esq, NONE, no_novo, versao_da_nova_arvore);
            } else if (temp_pai_original.dir == no_original) {
                modificar(temp_original_materialized.pai, dir, NONE, no_novo, versao_da_nova_arvore);
            }
            // O pai do no_novo já é o pai do no_original (copiado no construtor de cópia)
            // A modificação do pai do no_novo é tratada pela chamada modificar acima.
        }

        // 4. Propagar a mudança para os CHILDREN (se existirem)
        // O pai dos filhos de 'no_original' agora deve ser 'no_novo' na nova versão.
        // Os filhos de no_novo já foram copiados do no_original no construtor de cópia.
        if (no_novo->esq != nullptr) {
            modificar(no_novo->esq, pai, NONE, no_novo, versao_da_nova_arvore);
        }
        if (no_novo->dir != nullptr) {
            modificar(no_novo->dir, pai, NONE, no_novo, versao_da_nova_arvore);
        }

        return no_novo; // Retorna a nova cópia do nó que foi modificada
    }


    // Aplica uma modificação a um nó. Se o nó já foi modificado na `versao_alvo`,
    // cria uma nova cópia e atualiza a cadeia.
    // Retorna o nó (original ou a nova cópia) que foi modificado.
    Node* modificar(Node *no, field campo, int valor_chave, Node* valor_no, int versao_alvo){
        if (no == nullptr) return nullptr;

        Mod nova_modificacao;
        nova_modificacao.ver = versao_alvo;
        nova_modificacao.campo = campo;
        nova_modificacao.modChave = valor_chave;
        nova_modificacao.modPointer = valor_no;

        // Se o nó ainda não tem modificação para esta versão ou a modificação existente
        // é de uma versão *anterior* à que estamos construindo (versao_alvo),
        // então podemos aplicar a nova modificação diretamente ao slot 'mods' do nó.
        if (no->mods.campo == nenhum || no->mods.ver < versao_alvo) {
            no->mods = nova_modificacao;
            return no; // Retorna o nó original, que foi modificado
        }
        else { // O nó já tem uma modificação para a 'versao_alvo'
               // Precisamos criar uma nova cópia do nó e aplicar a modificação a ela.
               // Isso é o que "atualiza a cadeia" de ponteiros.
            return atualizaCadeia(no, nova_modificacao, versao_alvo);
        }
    }

    void inserir(int k, int version_base){
        // 'version_base' é a versão da qual estamos partindo para criar a nova versão.
        // A nova versão terá o índice 'vers'.

        // Inicializa a raiz da nova versão com a raiz da versão base.
        // Todas as modificações subsequentes nesta operação de inserção
        // serão rastreadas a partir desta 'root' e `vers`.
        root = versions[version_base];

        Node *no_novo = new Node(k); // Cria o novo nó
        nodes.push_back(no_novo);    // Adiciona ao vetor de todos os nós

        if (root == nullptr) { // Se a árvore base está vazia, o novo nó é a raiz.
            root = no_novo; // O novo nó se torna a raiz
            // Não precisa chamar 'modificar' para a raiz aqui, pois é uma nova raiz física.
            // Apenas definimos o root da nova versão.
        } else {
            Node *no_atual_search = root; // Ponteiro para o nó real na estrutura
            Node *no_anterior_search = nullptr; // Guarda o nó real pai da posição de inserção

            while (no_atual_search != nullptr) {
                no_anterior_search = no_atual_search; // Guarda o nó real antes de avançar

                Node temp_materialized; // Materializa o nó atual para decidir o caminho
                aplicaMods(no_atual_search, &temp_materialized, version_base); // Materializa para 'version_base'

                if (k < temp_materialized.chave)
                    no_atual_search = temp_materialized.esq;
                else
                    no_atual_search = temp_materialized.dir;
            }

            // Agora, no_anterior_search é o nó pai onde o novo nó será anexado.
            // Precisamos modificar o ponteiro esquerdo/direito do 'no_anterior_search'
            // na `vers` (nova versão) para apontar para `no_novo`.
            // Essa modificação pode causar uma cópia de `no_anterior_search`.
            Node temp_anterior_materialized; // Materializa o nó pai
            aplicaMods(no_anterior_search, &temp_anterior_materialized, version_base);

            if (k < temp_anterior_materialized.chave) {
                // no_novo se torna filho esquerdo de no_anterior_search
                modificar(no_anterior_search, esq, NONE, no_novo, vers);
            } else {
                // no_novo se torna filho direito de no_anterior_search
                modificar(no_anterior_search, dir, NONE, no_novo, vers);
            }

            // Define o pai do no_novo para apontar para no_anterior_search (ou sua cópia)
            modificar(no_novo, pai, NONE, no_anterior_search, vers);
        }

        // A nova versão é registrada.
        versions.push_back(root);
        vers++; // Incrementa para a próxima versão
    }

    Node *minimo(Node *no, int ver){
        if (no == nullptr) return nullptr;
        Node temp;
        // Materializa o nó atual para acessar seus filhos corretamente
        aplicaMods(no, &temp, ver);

        Node* current_node_real = no; // Mantém o ponteiro para o nó real na estrutura
        while (temp.esq != nullptr){
            current_node_real = temp.esq; // Atualiza o ponteiro real para o filho esquerdo
            aplicaMods(current_node_real, &temp, ver); // Materializa o novo nó atual
        }
        return current_node_real; // Retorna o nó real que é o mínimo
    }

    Node* sucessor(Node* no, int ver) {
        if (no == nullptr) return nullptr;

        Node temp_no;
        aplicaMods(no, &temp_no, ver); // Materializa o nó 'no'

        // Caso 1: Se o nó tem um filho à direita, o sucessor é o mínimo da subárvore direita.
        if (temp_no.dir != nullptr) {
            return minimo(temp_no.dir, ver);
        }

        // Caso 2: Não tem filho à direita. O sucessor é o primeiro ancestral 'y' tal que 'no'
        // está na subárvore esquerda de 'y'.
        Node* y = temp_no.pai; // Ponteiro real para o pai
        Node* x = no; // Ponteiro real para o nó atual

        while (y != nullptr) {
            Node temp_y;
            aplicaMods(y, &temp_y, ver); // Materializa o pai

            if (x == temp_y.esq) { // Se x é o filho esquerdo de y, então y é o sucessor.
                return y;
            }
            x = y; // Sobe na árvore
            y = temp_y.pai; // Atualiza o pai para o pai materializado de y
        }
        return nullptr; // Não há sucessor (é o maior elemento ou árvore vazia)
    }

    // Função auxiliar para transplantar subárvores. 'u' é o nó a ser substituído, 'v' é o nó que o substitui.
    // Todas as modificações são feitas na 'versao_da_nova_arvore'.
    void transplantar(Node* u, Node* v, int versao_da_nova_arvore) {
        Node tempU;
        aplicaMods(u, &tempU, versao_da_nova_arvore - 1); // Materializa U na versão anterior para ver seu pai

        if (tempU.pai == nullptr) { // Se U era a raiz na versão anterior
            root = v; // V se torna a nova raiz da versão atual
            if (v != nullptr) {
                modificar(v, pai, NONE, nullptr, versao_da_nova_arvore); // Garante que pai de V é nulo
            }
        } else { // U não era a raiz
            Node tempPaiU;
            aplicaMods(tempU.pai, &tempPaiU, versao_da_nova_arvore - 1); // Materializa pai de U

            // Decide se U era filho esquerdo ou direito do seu pai e atualiza o ponteiro do pai
            if (tempPaiU.esq == u) {
                modificar(tempU.pai, esq, NONE, v, versao_da_nova_arvore);
            } else {
                modificar(tempU.pai, dir, NONE, v, versao_da_nova_arvore);
            }

            // Atualiza o pai de V para apontar para o pai de U
            if (v != nullptr) {
                modificar(v, pai, NONE, tempU.pai, versao_da_nova_arvore);
            }
        }
    }

    void remover(int chave, int versao_base) { // int versao)
        // 'versao_base' é a versão da qual estamos partindo para criar a nova versão.
        // A nova versão terá o índice 'vers'.

        // Inicializa a raiz da nova versão com a raiz da versão base.
        root = versions[versao_base];

        if (root == nullptr) { // Se a árvore base está vazia, não há o que remover.
            versions.push_back(nullptr); // Nova versão também vazia
            vers++;
            return;
        }

        Node* alvo = buscaVersao(root, chave, versao_base); // Busca o nó na versão base
        if (!alvo) { // Se o nó não for encontrado na versão base
            versions.push_back(root); // A nova versão é idêntica à versão base
            vers++;
            return;
        }

        Node tempAlvo;
        aplicaMods(alvo, &tempAlvo, versao_base); // Materializa 'alvo' na versão base

        if (tempAlvo.esq == nullptr) {
            // Caso 1: alvo não tem filho esquerdo (substitui alvo pelo seu filho direito)
            transplantar(alvo, tempAlvo.dir, vers);
        } else if (tempAlvo.dir == nullptr) {
            // Caso 2: alvo não tem filho direito (substitui alvo pelo seu filho esquerdo)
            transplantar(alvo, tempAlvo.esq, vers);
        } else {
            // Caso 3: alvo tem dois filhos
            Node* succ = minimo(tempAlvo.dir, versao_base); // Encontra o sucessor na subárvore direita do alvo
            Node tempSucc;
            aplicaMods(succ, &tempSucc, versao_base); // Materializa o sucessor

            if (tempSucc.pai != alvo) { // Se o sucessor não é o filho direito direto do alvo
                // Primeiro, transplantar o sucessor por seu próprio filho direito
                transplantar(succ, tempSucc.dir, vers);
                // O filho direito do alvo se torna o filho direito do sucessor na nova versão
                modificar(succ, dir, NONE, tempAlvo.dir, vers);
                // O pai do filho direito original do alvo precisa apontar para 'succ'
                modificar(tempAlvo.dir, pai, NONE, succ, vers);
            }

            // Agora, transplantar o alvo pelo sucessor
            transplantar(alvo, succ, vers);
            // O filho esquerdo do alvo se torna o filho esquerdo do sucessor na nova versão
            modificar(succ, esq, NONE, tempAlvo.esq, vers);
            // O pai do filho esquerdo original do alvo precisa apontar para 'succ'
            modificar(tempAlvo.esq, pai, NONE, succ, vers);
        }

        versions.push_back(root); // Adiciona a nova versão da árvore após a remoção
        vers++; // Incrementa para a próxima versão
    }

    Node *busca(int k, int versao_query) {
        if (versao_query < 0 || versao_query >= (int)versions.size()) {
            // Se a versão solicitada é inválida, usa a versão mais recente.
            versao_query = versions.size() - 1;
        }
        return buscaVersao(versions[versao_query], k, versao_query);
    }

    Node* buscaVersao(Node* no, int k, int versao_query) {
        if (no == nullptr) return nullptr; // Se o nó for nulo, não encontrou

        Node temp;
        aplicaMods(no, &temp, versao_query); // Materializa o nó atual para a versão desejada

        if (k == temp.chave) {
            return no; // Se a chave for igual, encontrou o nó (retorna o nó real, não a cópia temporária)
        } else if (k < temp.chave) {
            return buscaVersao(temp.esq, k, versao_query); // Desce na subárvore esquerda
        } else {
            return buscaVersao(temp.dir, k, versao_query); // Desce na subárvore direita
        }
    }

    int get_key(Node* node, int version_query){
        Node temp;
        aplicaMods(node, &temp, version_query);
        return temp.chave; // Retorna a chave materializada
    }

    void DFS_REC(Node* node, int version_query, int profundidade, vector<pair<int,int>>& dfs_vector) {
        if(node == nullptr) return;

        Node temp;
        aplicaMods(node, &temp, version_query); // Materializa o nó para acessar seus filhos e chave corretamente

        DFS_REC(temp.esq, version_query, profundidade + 1, dfs_vector);
        dfs_vector.push_back({temp.chave, profundidade}); // Usa a chave materializada
        DFS_REC(temp.dir, version_query, profundidade + 1, dfs_vector);
    }

    vector<pair<int,int>> DFS(int version_query){
        // Garante que a versão de consulta é válida, usando a mais recente se for inválida.
        if (version_query < 0 || version_query >= (int)versions.size())
            version_query = versions.size() - 1;

        Node* root_to_dfs = versions[version_query];
        vector<pair<int,int>> dfs_vector;

        DFS_REC(root_to_dfs, version_query, 0, dfs_vector);

        return dfs_vector;
    }
};