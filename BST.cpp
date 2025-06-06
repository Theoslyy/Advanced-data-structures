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
    Mod mods; // <- vetor de mods terá tamanho um, logo, usamos só uma variável
    int chave;    //
    bool isRoot;
    // como a árvore binária de busca já mantém ponteiros para o nó
    // esquerdo e direito, não precisamos do vetor de retorno, já temos os ponteiros,
    // então, basta anotar mudanças nesses ponteiros como mods
    Node() // essa forma de inicializar as coisa é tão bonita...
    : esq(nullptr), dir(nullptr), pai(nullptr),
      chave(0), mods(), isRoot(false) {}
};

struct BST {
    Node *root; // raiz da versão vers - 1. vers é a 'versão a ser criada' na próx mod.
    vector<Node*> nodes; // Todos os nós criados, para gerenciamento de memória (opcional)
    vector<Node*> versions; // -> não precisa ser uma tupla de (int, node), o índice do vetor já dá em que versão estamos
    int vers; // desnecessário, é só ver o tamanho do vetor versions, mas ajuda na organização.

    BST()
    :   root(nullptr), vers(0) {
        versions.push_back(nullptr);  // Versão 0: árvore vazia (inicial)
    }

    // Função auxiliar para criar uma cópia de um nó e aplicar uma modificação.
    // Esta função será chamada por 'modificar' APENAS quando uma cópia é necessária.
    // Retorna a nova cópia do nó que foi modificada.
    void atualizaCadeia(BST* tree, Node *no, int versao_base, Mod modificacao_nova){ // Versão base para materializar
        // Criando o novo nó:
        Node *no_novo = new Node();
        Node temp_original; // Para materializar o estado do nó original

        // Copia o estado "materializado" do nó original para o novo nó.
        // Se o nó original já tinha uma modificação para essa mesma versão (versao_base),
        // isso significa que 'no->mods.ver' já é 'versao_base'.
        // Devemos materializar o estado do 'no' para a 'versao_base' antes de copiá-lo.
        // Caso contrário, copia o estado base (fields esq, dir, pai, chave) do nó original.
        if (no->mods.campo != nenhum && no->mods.ver == versao_base) {
            aplicaMods(no, &temp_original, versao_base); // Use versao_base para materializar o estado do nó
        } else {
            temp_original = *no; // Copia o estado atual do nó
        }
        *no_novo = temp_original;
        no_novo->mods = Mod(); // Limpa as modificações da cópia (será preenchida agora)

        // Aplica a nova modificação ao 'no_novo' (que é a cópia)
        no_novo->mods = modificacao_nova;

        // Adiciona a cópia ao vetor de todos os nós para gerenciamento de memória
        tree->nodes.push_back(no_novo);

        // Se o novo nó se torna raiz, seu pai é nulo e atualiza a raiz da árvore em construção
        if (no_novo->isRoot){
            no_novo->pai = nullptr;
            tree->root = no_novo;
        }

        // AGORA, ajusta os ponteiros de quem apontava para o nó original para apontar para a cópia (no_novo)
        // Isso deve ser feito na *nova versão* que está sendo construída (tree->vers)

        // Se o nó original tinha um pai, precisamos atualizar o ponteiro do pai
        if (temp_original.pai != nullptr){
            Node tempPaiNo;
            aplicaMods(temp_original.pai, &tempPaiNo, versao_base); // Materializa o pai do 'no' na versao_base

            // O pai de 'no' (temp_original.pai) deve apontar para 'no_novo' na nova versão
            if (tempPaiNo.esq == no) { // Se 'no' era filho esquerdo do seu pai
                modificar(tree, temp_original.pai, tree->vers, esq, NONE, no_novo);
            } else if (tempPaiNo.dir == no) { // Se 'no' era filho direito do seu pai
                modificar(tree, temp_original.pai, tree->vers, dir, NONE, no_novo);
            }
        } else if (no->isRoot) { // Se o nó original era a raiz da versao_alvo (ou versão anterior)
            tree->root = no_novo; // A nova raiz da versão atual sendo construída é a cópia
        }

        // Ajusta os ponteiros de filhos do nó original para que seus pais apontem para 'no_novo'
        // (A cópia do nó original se torna o pai para seus filhos na nova versão)
        if(temp_original.esq != nullptr){
            modificar(tree, temp_original.esq, tree->vers, pai, NONE, no_novo);
        }
        if(temp_original.dir != nullptr){
            modificar(tree, temp_original.dir, tree->vers, pai, NONE, no_novo);
        }
    }

    // Esta função decide se uma modificação é aplicada diretamente ou se uma cópia é necessária.
    // Retorna o nó (original ou uma nova cópia) que foi modificado e que agora pertence à versao_alvo.
    Node* modificar(BST *tree, Node *no, int versao_alvo, field campo, int valor_chave = NONE, Node* valor_no = nullptr){
        if (no == nullptr) return nullptr;

        Mod modifica;
        modifica.ver = versao_alvo;
        modifica.campo = campo;
        modifica.modChave = valor_chave;
        modifica.modPointer = valor_no;

        // Se o nó ainda não tem modificação para esta versão ou a modificação existente é de uma versão anterior
        // Isso significa que podemos aplicar a modificação diretamente ao nó sem criar uma cópia
        if (no->mods.campo == nenhum || no->mods.ver < versao_alvo) {
            no->mods = modifica;
            if (campo == raiz && valor_chave == 1) { // Se a modificação for tornar o nó raiz
                tree->root = no;
                no->pai = nullptr; // Raiz não tem pai
            }
            return no; // Retorna o nó original, que foi modificado
        } else { // O nó já tem uma modificação para esta 'versao_alvo', então precisamos de uma nova cópia na cadeia
            // A versão base para aplicaMods dentro de atualizaCadeia deve ser a versão da modificação existente no nó.
            atualizaCadeia(tree, no, no->mods.ver, modifica);
            return tree->root; // Retorna a nova raiz, pois a atualização da cadeia pode ter mudado a raiz da nova versão
        }
    }

    void aplicaMods(Node *atual, Node *temp, int version_query){
        // em resumo, um atual = temp.
        // Esta função "materializa" o estado de um nó para a 'version_query'
        *temp = *atual; // Copia o estado base do nó (campos e ponteiros base)

        // Se houver uma modificação válida para a 'version_query', aplica-a
        // A modificação é válida se for para a versão exata que estamos consultando
        if ((atual->mods.campo != nenhum) && (atual->mods.ver == version_query)) {
             switch (atual->mods.campo){
                case esq: temp->esq = atual->mods.modPointer; break;
                case dir: temp->dir = atual->mods.modPointer; break;
                case pai: temp->pai = atual->mods.modPointer; break;
                case raiz: temp->isRoot = atual->mods.modChave; if(temp->isRoot) temp->pai = nullptr; break;
                case chave: temp->chave = atual->mods.modChave; break;
                default: break;
            }
        }
    }

    void inserir(BST *tree, int k, int version_base) {
        // 'version_base' é a versão da qual estamos partindo para criar a nova versão

        // A raiz da nova versão (tree->root) deve ser inicializada a partir da raiz da versão base.
        // Qualquer modificação subsequente irá atualizar tree->root para a nova cópia se a raiz mudar.
        tree->root = tree->versions[version_base];

        Node *no_novo = new Node();
        no_novo->chave = k;
        tree->nodes.push_back(no_novo); // Adiciona o nó novo à lista de todos os nós

        if (tree->root == nullptr) { // Árvore base está vazia
            // O nó novo se torna a raiz da nova versão
            tree->root = modificar(tree, no_novo, tree->vers, raiz, 1, nullptr);
        } else {
            Node *no_atual_search = tree->root;
            Node *no_anterior_search = nullptr;

            Node no_temp_atual;
            // Node no_temp_anterior; // Removido: não é usado aqui

            while (no_atual_search != nullptr) {
                aplicaMods(no_atual_search, &no_temp_atual, version_base);

                no_anterior_search = no_atual_search;
                // Nao precisamos materializar no_anterior_search aqui, ele é apenas uma referencia ao no_atual_search antes do loop.
                // aplicaMods(no_anterior_search, &no_temp_anterior, version_base); // Removido: Redundante

                if (no_novo->chave < no_temp_atual.chave)
                    no_atual_search = no_temp_atual.esq;
                else
                    no_atual_search = no_temp_atual.dir;
            }

            // no_anterior_search é o nó pai onde o novo nó será anexado
            // Precisamos materializar no_anterior_search aqui para obter seu estado na version_base
            Node no_temp_anterior_materialized; // Nova cópia para materialização
            aplicaMods(no_anterior_search, &no_temp_anterior_materialized, version_base);

            // Precisamos garantir que o no_anterior_search é modificado para a nova versão,
            // e obter a cópia (se houver) para anexar o novo nó.
            // A modificação no pai não é 'pai', é 'esq' ou 'dir'. O pai do novo nó será o 'pai_modificado'.
            Node* pai_modificado;

            // Anexa o novo nó como filho do pai_modificado
            if (no_novo->chave < no_temp_anterior_materialized.chave) {
                pai_modificado = modificar(tree, no_anterior_search, tree->vers, esq, NONE, no_novo);
            } else {
                pai_modificado = modificar(tree, no_anterior_search, tree->vers, dir, NONE, no_novo);
            }

            // Define o pai do novo nó. O pai do no_novo é o nó retornado por modificar acima.
            modificar(tree, no_novo, tree->vers, pai, NONE, pai_modificado);
        }

        // As novas versões são adicionadas no final da operação.
        tree->versions.push_back(tree->root);
        tree->vers++;
    }

    Node *minimo(Node *no, int ver){
        if (no == nullptr) return nullptr;
        Node no_temp;
        aplicaMods(no, &no_temp, ver);
        Node* current_node = no; // Manter um ponteiro para o nó real, não a cópia temporária
        while (no_temp.esq != nullptr){ // percorre a árvore até o nó mais a esquerda (que na BST é o de menor valor)
            current_node = no_temp.esq;
            aplicaMods(current_node, &no_temp, ver);
        }
        return current_node; // Retorna o nó real
    }

    Node* sucessor(Node* no, int ver) {
        if (no == nullptr) return nullptr;

        Node temp;
        aplicaMods(no, &temp, ver);

        // Caso 1: tem filho à direita → sucessor é o mínimo da subárvore direita
        if (temp.dir != nullptr) {
            return minimo(temp.dir, ver);
        }

        // Caso 2: sobe pela árvore até encontrar um ancestral do qual 'no' é filho à esquerda
        Node* pai_real = temp.pai; // O ponteiro real para o pai
        Node* filho_real = no; // O ponteiro real para o filho

        while (pai_real != nullptr) { // se não tiver, então o sucessor é o primeiro ancestral que for pai do nó atual no qual atual é filho esquerdo
            Node tempPai;
            aplicaMods(pai_real, &tempPai, ver);

            if (tempPai.esq == filho_real) {
                return pai_real;
            }

            filho_real = pai_real;
            pai_real = tempPai.pai;
        }
        return nullptr;
    }

    void transplantar(BST *tree, Node* u, Node* v, int versao_base) { // int versao)
        // 'u' é o nó a ser substituído, 'v' é o nó que o substitui
        // 'versao_base' é a versão da qual estamos lendo para determinar os pais/filhos

        Node tempU;
        aplicaMods(u, &tempU, versao_base); // Estado de 'u' na versão base

        // se u for raiz, então v também será raiz
        if (tempU.isRoot) {
            if (v != nullptr) { // Se 'v' existe, ele se torna a nova raiz
                modificar(tree, v, tree->vers, raiz, 1, nullptr); // v não tem pai, pois é raiz
            } else { // Se 'v' é nulo e 'u' era a raiz, a nova raiz da versão atual é nula
                // Não há um nó para modificar para raiz=0, então apenas define tree->root
                tree->root = nullptr;
            }
        }
        else // 'u' não era a raiz
        {
            Node tempPai;
            aplicaMods(tempU.pai, &tempPai, versao_base); // Estado do pai de 'u' na versão base

            Node* pai_u_modificado = modificar(tree, tempU.pai, tree->vers, pai, NONE, tempU.pai); // Garante que o pai é copiado se necessário
            // O campo 'pai' aqui é apenas para forçar a cópia se necessário.
            // A modificação real será para 'esq' ou 'dir' do pai.

            if (tempPai.esq == u) // se u for filho esquerdo de pai, então v também será filho esquerdo
                modificar(tree, pai_u_modificado, tree->vers, esq, NONE, v);
            else if (tempPai.dir == u) // se u for filho direito de pai, então v também será filho direito
                modificar(tree, pai_u_modificado, tree->vers, dir, NONE, v);

            if (v != nullptr) // v passa a ser filho de pai
            { // Se 'v' existe, atualiza o pai de 'v'
                modificar(tree, v, tree->vers, pai, NONE, pai_u_modificado);
            }
        }
    }

    void remover(BST *tree, int chave, int versao_base) { // int versao)
        // 'versao_base' é a versão da qual estamos partindo para criar a nova versão

        // A raiz da nova versão (tree->root) deve ser inicializada a partir da raiz da versão base.
        tree->root = tree->versions[versao_base];

        if (tree->root == nullptr) { // Se a árvore base está vazia
            tree->versions.push_back(nullptr); // Nova versão também vazia
            tree->vers++;
            return;
        }

        Node* alvo = buscaVersao(tree->root, chave, versao_base); // Busca na versão base
        if (!alvo) { // se não encontrar o nó, não faz nada
            tree->versions.push_back(tree->versions[versao_base]); // Nova versão é igual à base
            tree->vers++;
            return;
        }

        Node tempAlvo;
        aplicaMods(alvo, &tempAlvo, versao_base); // aplica as modificações do nó alvo para a versão desejada

        if (tempAlvo.esq == nullptr) {
            transplantar(tree, alvo, tempAlvo.dir, versao_base); // se não tiver filho esquerdo, transplanta o filho direito
        } else if (tempAlvo.dir == nullptr) {
            transplantar(tree, alvo, tempAlvo.esq, versao_base); // se não tiver filho direito, transplanta o filho esquerdo
        } else {
            Node* succ = minimo(tempAlvo.dir, versao_base); // se tiver os dois filhos, encontra o sucessor
            Node tempSucc;
            aplicaMods(succ, &tempSucc, versao_base); // aplica as modificações do sucessor para a versão desejada

            if (tempSucc.pai != alvo) {
                transplantar(tree, succ, tempSucc.dir, versao_base); // transplanta o sucessor por seu próprio filho direito
                // O sucessor precisa ter seu pai atualizado para seu novo filho direito se ele não era filho direto do alvo.
                // Mas aqui, o foco é o sucessor em si.
                // A linha abaixo é apenas para garantir que 'succ' é copiado se necessário para a nova versão.
                if (tempSucc.dir != nullptr) {
                    modificar(tree, tempSucc.dir, tree->vers, pai, NONE, succ); // Pai do filho direito do sucessor
                }
            }

            // Transplantar o alvo pelo sucessor (antes de ajustar os filhos do sucessor)
            transplantar(tree, alvo, succ, versao_base);

            // Agora, o sucessor (ou sua cópia) já é o novo nó na posição do alvo.
            // Precisamos ajustar os filhos esquerdo e direito do sucessor na nova versão.
            // O sucessor precisa ter os filhos do alvo.
            Node* succ_modificado_esq = modificar(tree, succ, tree->vers, esq, NONE, tempAlvo.esq); // atualiza o filho esquerdo do sucessor
            Node* succ_modificado_dir = modificar(tree, succ_modificado_esq, tree->vers, dir, NONE, tempAlvo.dir); // atualiza o filho direito do sucessor (após o esquerdo)

            // Atualiza os pais dos filhos do alvo para apontarem para o sucessor
            if (tempAlvo.esq != nullptr)
                modificar(tree, tempAlvo.esq, tree->vers, pai, NONE, succ_modificado_dir);

            // O pai do filho direito do alvo já foi tratado pelo transplantar,
            // e agora precisa ser atualizado para apontar para o sucessor modificado.
            if (tempAlvo.dir != nullptr && tempAlvo.dir != succ) { // Se o filho direito do alvo não é o próprio sucessor
                modificar(tree, tempAlvo.dir, tree->vers, pai, NONE, succ_modificado_dir);
            }
        }

        tree->versions.push_back(tree->root); // adiciona a nova versão da árvore após a remoção
        tree->vers++;
    }

    Node *busca(BST* tree, int k, int versao){ //, int version) -> essa função é necessária?
        // buscar em uma versao especifica requer aplicar as mods enquanto se desce na árvore para saber os filhos para o qual o nó aponta
        if (versao < 0 || versao >= (int)tree->versions.size()) {
            return nullptr; // Versão inválida
        }
        return buscaVersao(tree->versions[versao], k, versao);
    }

    Node* buscaVersao(Node* no, int k, int versao){
        if (no == nullptr) return nullptr; // se o nó for nulo, não encontrou
        Node temp;
        aplicaMods(no, &temp, versao); // aplica as modificações do nó atual para a versão desejada
        if(k == temp.chave) return no; // se a chave for igual, encontrou o nó (retorna o nó original, não a cópia temporária)
        else if(k < temp.chave) return buscaVersao(temp.esq, k, versao); // se a chave for menor, desce na subárvore esquerda
        else return buscaVersao(temp.dir, k, versao); // se a chave for maior, desce na subárvore direita
    }

    int get_key(Node* node, int version_query){
        /*Função que lê corretamente a chave de um nó dada a sua versão
        Implementei de forma que nó possa ter várias modificações dentro
        dele, porém sabemos que só pode haver no máximo 1*/
        /*
        caso fosse um vetor:
        for(int index = 0; index < 1; index++) {

            if(node->mods[index].ver > version)
                return index > 0? node->mods[index-1].modChave : node->chave;

        }
        */
        Node temp; // Materializa o nó para obter a chave correta
        aplicaMods(node, &temp, version_query);
        return temp.chave; // Retorna a chave materializada
    }

    void DFS_REC(Node* node, int version_query, int profundidade, vector<pair<int,int>>& dfs_vector){
        /*Função recursiva auxiliar que será usada para utilizar a DFS sem necessidade de dar
            profundidade=0 como um dos parametros na função */
        if(node == nullptr) return;

        Node temp; // Materializa o nó para acessar seus filhos e chave corretamente
        aplicaMods(node, &temp, version_query);
        DFS_REC(temp.esq, version_query, profundidade + 1, dfs_vector);
        dfs_vector.push_back({temp.chave, profundidade}); // Usa a chave materializada
        DFS_REC(temp.dir, version_query, profundidade + 1, dfs_vector);
    }

    vector<pair<int,int>> DFS(int version_query){ // BST tree, int version) -> Removido BST tree, já é membro
        /*Função que retorna um array com o par (Chave, profundidade de um nó) */
        if (version_query < 0 || version_query >= versions.size())
            version_query = versions.size() - 1; // Garante que a versão de consulta é válida

        Node* root_to_dfs = versions[version_query]; // Node* root = tree.versions[version];
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