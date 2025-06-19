#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define NULLKEY -100000

using namespace std;

// Enum para o campo que uma modificação (Mod) está alterando.
enum field{
    left, right, key, root, none
};

enum field; struct Mod; struct Node; struct BST;

// Estrutura para uma modificação em um nó.
struct Mod{
    int version;
    field mod_field;
    int mod_key;
    Node* mod_pointer;

    Mod() : version(-1), mod_field(field::none), mod_key(NULLKEY), mod_pointer(nullptr) {}
    Mod(int v, field f, int k, Node* ptr) : version(v), mod_field(f), mod_key(k), mod_pointer(ptr) {}
};

// Estrutura para um nó da árvore.
struct Node{
    Node* left;
    Node* right;
    int nodeKey;
    int nodeVersion; 
    bool isRoot; 
    Mod modifications[2]; // Array para modificações que evita criar um nó novo a cada mudança.

    Node() : left(nullptr), right(nullptr), nodeKey(NULLKEY), isRoot(false), modifications{} {}
    //constructor with mods:
    Node(Node* l, Node* r, int key, int version, bool root, const Mod mod0, const Mod mod1)
    : left(l), right(r), nodeKey(key), nodeVersion(version), isRoot(root) {
    modifications[0] = mod0;
    modifications[1] = mod1;
    }
    //constructor without mods:
    Node(Node* l, Node* r, int key, int version, bool root)
    : left(l), right(r), nodeKey(key), nodeVersion(version), isRoot(root), modifications{} {}
};

// Estrutura principal da Árvore de Busca Binária Persistente.
struct BST{
    Node* root; // Ponteiro para a raiz "viva" (da versão mais recente).
    vector<Node*> nodes; // Vetor para gerenciar todos os nós já criados.
    vector<Node*> versions; // Vetor onde cada índice é uma versão e o valor é a raiz daquela versão.
    int tree_version; // A versão mais recente da árvore.

    BST() : root(nullptr), nodes{}, versions{}, tree_version(-1) {}

    // functions:

    //to-dos:
    /*
        1. tratar ser root no chain update e no insertMod
        2. (acho que isso só é necessário na remoção:)
        assim que a gente insere uma modificacao e retorna o no, 
        temos que checar se o no retornado e da mesma versao q o no anterior,
        pois se nao for, sera um novo no e temos que atualizar coisas. 
    */

    /* Aplica TODAS as modificações de um nó (node_old) para outro (node_new), sem considerar a versão. */
    void applyMods(Node* node_old, Node* node_new){
        if(node_old->modifications[0].mod_field != field::none){
            switch(node_old->modifications[0].mod_field){
                case field::key: node_new->nodeKey = node_old->modifications[0].mod_key; break;
                case field::left: node_new->left = node_old->modifications[0].mod_pointer; break;
                case field::right: node_new->right = node_old->modifications[0].mod_pointer; break;
                case field::root: node_new->isRoot = node_old->modifications[0].mod_key; break;
                case field::none: break;
                default: break;
            }
        }
        if(node_old->modifications[1].mod_field != field::none){
            switch(node_old->modifications[1].mod_field){
                case field::key: node_new->nodeKey = node_old->modifications[1].mod_key; break;
                case field::left: node_new->left = node_old->modifications[1].mod_pointer; break;
                case field::right: node_new->right = node_old->modifications[1].mod_pointer; break;
                case field::root: node_new->isRoot = node_old->modifications[1].mod_key; break;
                case field::none: break;
                default: break;
            }
        }
    }
    
    /* "Materializa" um nó, aplicando modificações até uma versão específica. */
    void applyVersion(Node* node_old, Node* node_new, int version){
        // A lógica base do nó novo é a do nó antigo.
        node_new->left = node_old->left;
        node_new->right = node_old->right;
        node_new->nodeKey = node_old->nodeKey;
        node_new->isRoot = node_old->isRoot;

        // Percorre as duas possíveis modificações do nó e as aplica se a versão for compatível.
        if(node_old->modifications[0].mod_field != field::none && node_old->modifications[0].version <= version){
            switch(node_old->modifications[0].mod_field){
                case field::key: node_new->nodeKey = node_old->modifications[0].mod_key; break;
                case field::left: node_new->left = node_old->modifications[0].mod_pointer; break;
                case field::right: node_new->right = node_old->modifications[0].mod_pointer; break;
                case field::root: node_new->isRoot = node_old->modifications[0].mod_key; break;
                case field::none: break;
                default: break;
            }
        }
        if(node_old->modifications[1].mod_field != field::none && node_old->modifications[1].version <= version){
            switch(node_old->modifications[1].mod_field){
                case field::key: node_new->nodeKey = node_old->modifications[1].mod_key; break;
                case field::left: node_new->left = node_old->modifications[1].mod_pointer; break;
                case field::right: node_new->right = node_old->modifications[1].mod_pointer; break;
                case field::root: node_new->isRoot = node_old->modifications[1].mod_key; break;
                case field::none: break;
                default: break;
            }
        }
    }
    
    /* Se um nó não tem mais espaço para modificações, cria uma cópia física dele (um novo nó). */
    Node* chainUpdate(BST* tree, Node* node, Mod modification) {
        Node* newNode = new Node(node->left, node->right, node->nodeKey, node->nodeVersion, node->isRoot);
        applyMods(node, newNode);
        switch (modification.mod_field) {
            case field::key: newNode->nodeKey = modification.mod_key; break;
            case field::left: newNode->left = modification.mod_pointer; break;
            case field::right: newNode->right = modification.mod_pointer; break;
            case field::root:
                newNode->isRoot = modification.mod_key;
                if (modification.mod_key == 1) tree->root = newNode;
                break;
            default: break;
        }
        newNode->nodeVersion = modification.version;

        Node* tempParent = search(tree, node->nodeKey);
        if (tempParent) {
            Mod m(modification.version,
                (newNode->nodeKey < tempParent->nodeKey ? field::left : field::right),
                NULLKEY, newNode);
            insertMod(tree, tempParent, m);
        }

        return newNode;
    }

    /* Tenta inserir uma modificação em um nó. Se houver espaço no array `modifications`,
     * insere. Caso contrário, chama `chainUpdate` para criar um nó novo. */
    Node* insertMod(BST* tree, Node* node, Mod newModification){
        if (!node) return nullptr;

        if (node->nodeVersion == newModification.version) {
            // Mesma versão: altera em place
            if (newModification.mod_field == field::left) node->left = newModification.mod_pointer;
            if (newModification.mod_field == field::right) node->right = newModification.mod_pointer;
            if (newModification.mod_field == field::root) {
                node->isRoot = newModification.mod_key;
                if (newModification.mod_key == 1) tree->root = node;
            }
            return node;
        }

        // Se há espaço para mod no nó:
        for (int i = 0; i < 2; i++) {
            if (node->modifications[i].mod_field == field::none) {
                node->modifications[i] = newModification;
                if (newModification.mod_field == field::root && newModification.mod_key == 1)
                    tree->root = node;
                return node;
            }
        }
        // Caso contrário, faz chain update:
        return chainUpdate(tree, node, newModification);
    }

    /* Insere um novo nó na árvore, criando uma nova versão. */
    void insertNode(int k) {
        // A inserção ocorre na próxima versão.
        Node* newNode = new Node(nullptr, nullptr, k, tree_version + 1, false);
        
        // Caso especial: primeira inserção na árvore.
        if (tree_version == -1) {
            root = newNode;
            newNode->isRoot = true;
            nodes.push_back(newNode);
            versions.push_back(newNode);
            tree_version++;
            return;
        }
        
        // Encontra a posição correta para inserção na última versão.
        Node* cur = versions[tree_version];
        Node* parent = nullptr;

        while (cur) {
            parent = cur;
            Node temp;
            applyVersion(cur, &temp, tree_version);
            if (k < temp.nodeKey) cur = temp.left;
            else cur = temp.right;
        }
        
        // Cria a modificação para o nó pai.
        Node temp_parent;
        applyVersion(parent, &temp_parent, tree_version);
        field lado = (k < temp_parent.nodeKey) ? field::left : field::right;
        Mod m(tree_version + 1, lado, NULLKEY, newNode);
        
        insertMod(this, parent, m);

        // Finaliza a criação da nova versão.
        nodes.push_back(newNode);
        versions.push_back(root); // A raiz 'live' é a raiz da nova versão
        tree_version++;
    }

    //returns the parent of a node if k is a valid key
    Node* search(BST* tree, int k){
        if(tree->tree_version == -1 || tree->root == nullptr) return nullptr; 
        return searchRec(tree, tree->root, nullptr, k);
    }

    Node* searchRec(BST* tree, Node* node, Node* parent, int k){
        Node* tempNode = new Node(node->left, node->right, node->nodeKey, node->nodeVersion, true);
        Node* tempParent = parent;
        applyVersion(node, tempNode, tree_version); // Usa a versão mais recente
        if(k < tempNode->nodeKey){
            if(tempNode->left != nullptr) return searchRec(tree, tempNode->left, node, k);
            else{ delete tempNode; return nullptr; }
        }
        else if(k > tempNode -> nodeKey){
            if(tempNode->right != nullptr) return searchRec(tree, tempNode->right, node, k);
            else{ delete tempNode; return nullptr; }
            }
        delete tempNode; 
        return tempParent; 
    }

    /* Função recursiva auxiliar para o DFS que materializa e percorre em ordem (in-order). */
    void DFS_REC(Node* node, int version_query, int profundidade, vector<pair<int,int>>& dfs_vector) {
        if (node == nullptr) return;
        Node temp;
        applyVersion(node, &temp, version_query);

        DFS_REC(temp.left, version_query, profundidade + 1, dfs_vector);
        dfs_vector.push_back({ temp.nodeKey, profundidade }); // Registra o nó atual
        DFS_REC(temp.right, version_query, profundidade + 1, dfs_vector);
    }

    /* Inicia a busca em profundidade (DFS) para uma dada versão. */
    vector<pair<int,int>> DFS(int version_query) {
        vector<pair<int,int>> dfs_vector;
        if (version_query < 0 || version_query >= versions.size() || versions[version_query] == nullptr) return dfs_vector;
        DFS_REC(versions[version_query], version_query, 0, dfs_vector);
        return dfs_vector;
    }

    /* Encontra o nó com a menor chave em uma subárvore para uma dada versão. */
    Node* minimo(Node* no, int version_query) {
        if (no == nullptr) return nullptr;
        Node* current = no;
        while (true) {
            Node temp;
            applyVersion(current, &temp, version_query);
            if (temp.left == nullptr) break;
            current = temp.left;
        }
        return current;
    }

    /* Encontra o sucessor de uma DADA CHAVE em uma versão específica da árvore. */
    Node* sucessor(Node* raiz, int chave, int ver) {
        Node* candidato = nullptr;
        Node* atual = raiz;
        while (atual != nullptr) {
            Node temp;
            applyVersion(atual, &temp, ver);
            if (temp.nodeKey > chave) {
                candidato = atual;
                atual = temp.left;
            } else {
                atual = temp.right;
            }
        }
        return candidato;
    }

    /**
     * Substitui a subárvore 'u' pela 'v'. Cria cópias de todos os ancestrais
     * de 'u' para garantir que a versão anterior não seja modificada.
     */
    Node* transplantar(Node* raiz_da_versao, Node* u, Node* v, int versao_base, int nova_versao) {
        // Caso 1: O nó a ser trocado é a própria raiz. 'v' se torna a nova raiz.
        if (raiz_da_versao == u) {
            return v;
        }

        // Fase 1: Traçar o caminho da raiz até 'u', armazenando os ancestrais.
        vector<Node*> path;
        Node* atual = raiz_da_versao;
        while (atual != nullptr && atual != u) {
            path.push_back(atual);
            Node temp_atual;
            applyVersion(atual, &temp_atual, versao_base);
            atual = (u->nodeKey < temp_atual.nodeKey) ? temp_atual.left : temp_atual.right;
        }
        
        if (atual == nullptr) return raiz_da_versao; // Segurança: u não encontrado.

        // Fase 2: Reconstruir o caminho de baixo para cima com cópias.
        Node* filho_substituto = v; // A primeira substituição é 'v' no lugar do filho do nó pai de 'u'.

        // Itera do pai de 'u' (último elemento do caminho) até a raiz.
        for (int i = path.size() - 1; i >= 0; --i) {
            Node* pai_original = path[i];
            Node temp_pai;
            applyVersion(pai_original, &temp_pai, versao_base);

            // Determina qual era o filho original no caminho para saber qual ponteiro atualizar.
            Node* filho_original_no_caminho = (i + 1 < path.size()) ? path[i + 1] : u;
            
            // Cria a cópia persistente do pai.
            Node* pai_copiado = new Node(temp_pai.left, temp_pai.right, temp_pai.nodeKey, nova_versao, false);
            nodes.push_back(pai_copiado);

            // Atualiza o ponteiro correto (esquerdo ou direito) na cópia do pai.
            if (temp_pai.left == filho_original_no_caminho) {
                pai_copiado->left = filho_substituto;
            } else {
                pai_copiado->right = filho_substituto;
            }
            
            // A cópia do pai se torna o 'filho substituto' para o nível de cima (seu ancestral).
            filho_substituto = pai_copiado;
        }
        // A última cópia no topo do caminho é a nova raiz da árvore.
        return filho_substituto;
    }

    /* Função de remover com auxílode transplantar */
    void remover(int chave) {
        int versao_base = this->tree_version;
        // Se a árvore estiver vazia, cria uma nova versão vazia.
        if (versao_base < 0) {
            this->tree_version++;
            versions.push_back(nullptr);
            this->root = nullptr;
            return;
        }

        // Busca o nó a ser removido ('alvo') na versão base.
        Node* raiz_base = versions[versao_base];
        Node* alvo = raiz_base;
        while (alvo != nullptr) {
            Node temp_alvo;
            applyVersion(alvo, &temp_alvo, versao_base);
            if (chave == temp_alvo.nodeKey) break;
            alvo = (chave < temp_alvo.nodeKey) ? temp_alvo.left : temp_alvo.right;
        }

        this->tree_version++;
        int nova_versao = this->tree_version;

        // Se o nó não foi encontrado, a nova versão é uma cópia da anterior.
        if (alvo == nullptr) {
            versions.push_back(raiz_base);
            this->root = raiz_base;
            return;
        }

        Node temp_alvo;
        applyVersion(alvo, &temp_alvo, versao_base);
        Node* nova_raiz;

        // Caso 1 e 2: O alvo tem 0 ou 1 filho.
        if (temp_alvo.right == nullptr) {
            nova_raiz = transplantar(raiz_base, alvo, temp_alvo.left, versao_base, nova_versao);
        } else if (temp_alvo.left == nullptr) {
            nova_raiz = transplantar(raiz_base, alvo, temp_alvo.right, versao_base, nova_versao);
        } 
        // Caso 3: O alvo tem 2 filhos.
        else {
            // Encontra o sucessor do alvo.
            Node* sucessor_node = minimo(temp_alvo.right, versao_base);
            Node temp_sucessor;
            applyVersion(sucessor_node, &temp_sucessor, versao_base);
            
            // ETAPA A: Remove o sucessor de sua posição original DENTRO DA SUBÁRVORE DIREITA.
            // O resultado é uma subárvore direita nova e persistente, já sem o sucessor.
            Node* nova_subarvore_direita = transplantar(temp_alvo.right, sucessor_node, temp_sucessor.right, versao_base, nova_versao);
            
            // ETAPA B: Cria uma cópia do nó sucessor para ser o novo 'alvo'.
            // Seus filhos são: o filho esquerdo do alvo original e a nova subárvore direita da etapa A.
            Node* novo_alvo = new Node(temp_alvo.left, nova_subarvore_direita, temp_sucessor.nodeKey, nova_versao, false);
            nodes.push_back(novo_alvo);

            // ETAPA C: Transplanta o 'alvo' original pelo 'novo_alvo' que acabamos de montar.
            nova_raiz = transplantar(raiz_base, alvo, novo_alvo, versao_base, nova_versao);
        }
        
        // Finaliza a criação da nova versão.
        versions.push_back(nova_raiz);
        this->root = nova_raiz;
    }
};