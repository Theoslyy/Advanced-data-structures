#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define NULLKEY -100000

using namespace std;

// Enum for what field a mod is modifying. 
enum field{
    left, right, key, root, none
};

enum field; struct Mod; struct Node; struct BST;

struct Mod{
    int version;
    field mod_field;
    int mod_key;
    Node* mod_pointer;

    Mod() : version(-1), mod_field(field::none), mod_key(NULLKEY), mod_pointer(nullptr) {}
    Mod(int v, field f, int k, Node* ptr) : version(v), mod_field(f), mod_key(k), mod_pointer(ptr) {}
};

struct Node{
    Node* left;
    Node* right;
    int nodeKey;
    int nodeVersion; 
    bool isRoot; 
    Mod modifications[2]; 

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

struct BST{
    Node* root; 
    vector<Node*> nodes; 
    vector<Node*> versions; 
    int tree_version; // latest version

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

    //apply ALL mods from one node to the other. This is for insertions and removals. 
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
            //node_new->nodeVersion = node_old->modifications[0].version; this might be a problem so we're not doing it
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
            //node_new->nodeVersion = node_old->modifications[1].version; this might be a problem
        }
    }
    // applies mods only up to some version, for searches. 
    void applyVersion(Node* node_old, Node* node_new, int version){
        if(node_old->modifications[0].mod_field != field::none && node_old->modifications[0].version <= version){
            switch(node_old->modifications[0].mod_field){
                case field::key: node_new->nodeKey = node_old->modifications[0].mod_key; break;
                case field::left: node_new->left = node_old->modifications[0].mod_pointer; break;
                case field::right: node_new->right = node_old->modifications[0].mod_pointer; break;
                case field::root: node_new->isRoot = node_old->modifications[0].mod_key; break;
                case field::none: break;
                default: break;
            }
            //node_new->nodeVersion = node_old->modifications[0].version; this might be a problem
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
            //node_new->nodeVersion = node_old->modifications[1].version; this might be a problem
        }
    }
    //Returns the newly created node. 
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

    //we will be inserting on version tree->tree_version + 1
    void insertNode(int k) {
        Node* newNode = new Node(nullptr, nullptr, k, tree_version + 1, false);
        if (tree_version == -1) {
            root = newNode;
            newNode->isRoot = true;
            nodes.push_back(newNode);
            versions.push_back(newNode);
            tree_version++;
            return;
        }
        Node* cur = versions[tree_version];
        Node* parent = nullptr;
        Node* temp = new Node(cur->left, cur->right, cur->nodeKey, cur->nodeVersion, cur->isRoot);
        applyMods(cur, temp);

        while (cur) {
            parent = cur;
            if (k < temp->nodeKey) cur = temp->left;
            else cur = temp->right;

            if (cur) {
                temp = new Node(cur->left, cur->right, cur->nodeKey, cur->nodeVersion, cur->isRoot);
                applyMods(cur, temp);
            }
        }

        Mod m(tree_version + 1,
            (k < parent->nodeKey) ? field::left : field::right,
            NULLKEY, newNode);
        insertMod(this, parent, m);

        nodes.push_back(newNode);
        versions.push_back(root);
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
        applyMods(node, tempNode); 
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

    void DFS_REC(Node* node, int version_query, int profundidade, vector<pair<int,int>>& dfs_vector) {
        /*Função recursiva auxiliar que materializa e percorre em ordem (in-order)*/
        if (node == nullptr) return;

        // Cria uma cópia física para materializar até a versão desejada
        Node temp(node->left, node->right, node->nodeKey, node->nodeVersion, node->isRoot);
        applyVersion(node, &temp, version_query);

        // Percorre subárvore esquerda
        DFS_REC(temp.left, version_query, profundidade + 1, dfs_vector);

        // Registra o nó atual: (chave, profundidade)
        dfs_vector.push_back({ temp.nodeKey, profundidade });

        // Percorre subárvore direita
        DFS_REC(temp.right, version_query, profundidade + 1, dfs_vector);
    }

    vector<pair<int,int>> DFS(int version_query) {
        /*Função que inicia a busca em profundidade (DFS)*/
        vector<pair<int,int>> dfs_vector;
        if (version_query >= versions.size() || versions[version_query] == nullptr) return dfs_vector;
        DFS_REC(versions[version_query], version_query, 0, dfs_vector);
        return dfs_vector;
    }

    Node* minimo(Node* no, int version_query) {
        if (no == nullptr) return nullptr;
        Node* current = no;
        Node temp(current->left, current->right, current->nodeKey, current->nodeVersion, current->isRoot);
        applyVersion(current, &temp, version_query);
        while (temp.left != nullptr) {
            current = temp.left;
            temp = Node(current->left, current->right, current->nodeKey, current->nodeVersion, current->isRoot);
            applyVersion(current, &temp, version_query);
        }
        return current;
    }

    Node* sucessor(Node* raiz, int chave, int ver) {
        Node* candidato = nullptr;
        Node* atual = raiz;

        while (atual != nullptr) {
            Node temp(atual->left, atual->right, atual->nodeKey, atual->nodeVersion, atual->isRoot);
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

    Node* sucessor(Node* no, int ver) {
        if (no == nullptr) return nullptr;
        Node temp(no->left, no->right, no->nodeKey, no->nodeVersion, no->isRoot);
        applyVersion(no, &temp, ver);
        return sucessor(versions[ver], temp.nodeKey, ver);
    }

    // Função para encontrar o pai de um nó específico em uma versão
    Node* findParent(Node* target, int version) {
        if (!target || !versions[version] || target == versions[version]) return nullptr;
        
        Node* current = versions[version];
        while (current) {
            Node temp(current->left, current->right, current->nodeKey, current->nodeVersion, current->isRoot);
            applyVersion(current, &temp, version);
            
            if (temp.left == target || temp.right == target) {
                return current;
            }
            
            current = (target->nodeKey < temp.nodeKey) ? temp.left : temp.right;
        }
        return nullptr;
    }

    // Substitui o nó 'u' pelo nó 'v' na versão (versao_base + 1).
    Node* transplantar(Node* u, Node* v, int versao_base) {
        // 1. Se u for raiz na versão base, atualiza a raiz
        if (u == versions[versao_base]) {
            if (v) {
                // cria v' como nova raiz com versão atual
                Node* vlinha = new Node(v->left, v->right, v->nodeKey, tree_version, true);
                applyVersion(v, vlinha, versao_base);
                nodes.push_back(vlinha);
                root = vlinha;
                return vlinha;
            } else {
                // v é nullptr: árvore vazia
                root = nullptr;
                return nullptr;
            }
        }

        // 2. Encontra o pai de u na versão base
        Node* parent = findParent(u, versao_base);
        if (!parent) return v;  // Segurança: pai deveria existir

        // 3. Determina se u era filho esquerdo ou direito
        Node tempParent(parent->left, parent->right, parent->nodeKey, parent->nodeVersion, parent->isRoot);
        applyVersion(parent, &tempParent, versao_base);
        field lado = (tempParent.left == u) ? field::left : field::right;

        // 4. Cria nova versão de v, se necessário
        Node* vlinha = v;
        if (v && v->nodeVersion != tree_version) {
            vlinha = new Node(v->left, v->right, v->nodeKey, tree_version, false);
            applyVersion(v, vlinha, versao_base);
            nodes.push_back(vlinha);
        }

        // 5. Aplica modificação no pai (chainUpdate se necessário)
        Mod m(tree_version, lado, NULLKEY, vlinha);
        insertMod(this, parent, m);

        return vlinha;
    }

    // Remove a chave 'chave' a partir de versao_base → gera versão tree_version+1
    void remover(int chave) {
        int versao_base = tree_version;

        if (versao_base < 0 || !versions[versao_base]) {
            tree_version++;
            versions.push_back(nullptr);
            root = nullptr;
            return;
        }

        Node* atual = versions[versao_base];
        Node* alvo = nullptr;
        while (atual) {
            Node temp(atual->left, atual->right, atual->nodeKey, atual->nodeVersion, atual->isRoot);
            applyVersion(atual, &temp, versao_base);
            if (chave == temp.nodeKey) {
                alvo = atual;
                break;
            }
            atual = (chave < temp.nodeKey) ? temp.left : temp.right;
        }

        tree_version++;

        if (!alvo) {
            versions.push_back(versions[versao_base]);
            root = versions[versao_base];
            return;
        }

        Node tempAlvo(alvo->left, alvo->right, alvo->nodeKey, alvo->nodeVersion, alvo->isRoot);
        applyVersion(alvo, &tempAlvo, versao_base);

        if (!tempAlvo.left && !tempAlvo.right) {
            // nó folha → substituído por nullptr
            root = transplantar(alvo, nullptr, versao_base);
            versions.push_back(root);
            return;
        }

        if (!tempAlvo.left || !tempAlvo.right) {
            Node* filho = tempAlvo.left ? tempAlvo.left : tempAlvo.right;
            root = transplantar(alvo, filho, versao_base);
            versions.push_back(root);
            return;
        }

        Node* succ = minimo(tempAlvo.right, versao_base);
        Node tempSucc(succ->left, succ->right, succ->nodeKey, succ->nodeVersion, succ->isRoot);
        applyVersion(succ, &tempSucc, versao_base);

        Node* newSucc = new Node(nullptr, nullptr, tempSucc.nodeKey, tree_version, false);
        nodes.push_back(newSucc);

        // Copiamos filhos de tempAlvo em newSucc
        Mod mL(tree_version, field::left, NULLKEY, tempAlvo.left);
        Mod mR(tree_version, field::right, NULLKEY, tempAlvo.right);
        insertMod(this, newSucc, mL);
        insertMod(this, newSucc, mR);

        if (succ != tempAlvo.right) {
            Node* succParent = findParent(succ, versao_base);
            if (succParent) {
                Node tempP(succParent->left, succParent->right, succParent->nodeKey, succParent->nodeVersion, succParent->isRoot);
                applyVersion(succParent, &tempP, versao_base);
                field lado = (tempP.left == succ) ? field::left : field::right;
                Mod m(tree_version, lado, NULLKEY, tempSucc.right);
                insertMod(this, succParent, m);
            }
        }

        root = transplantar(alvo, newSucc, versao_base);
        versions.push_back(root);
    }

};
