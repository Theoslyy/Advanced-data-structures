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
    Mod mods; 
    int chave;    //
    bool isRoot;
    int ver;
    // como a árvore binária de busca já mantém ponteiros para o nó
    // esquerdo e direito, não precisamos do vetor de retorno, já temos os ponteiros,
    // então, basta anotar mudanças nesses ponteiros como mods
    Node() // essa forma de inicializar as coisa é tão bonita...
    : esq(nullptr), dir(nullptr), pai(nullptr),
      chave(0), mods(), isRoot(false), ver(0) {}
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
    Node* atualizaCadeia(BST* tree, Node *no, int versao_base, Mod modificacao_nova){ // Versão base para materializar
        cout << "cadeia.. \n";
        // Criando o novo nó:
        Node *no_novo = new Node();
        Node temp_original;
        aplicaMods(no, &temp_original, modificacao_nova.ver);
        //os dois primeiros ifs são só de teste. 
        if (no->mods.campo != nenhum && no->mods.ver == modificacao_nova.ver)
        {
            //isso só acontece se um nó for modificado em cadeia duas vezes na mesma versão
            //em teoria, isso não era para estar acontecendo. 
            //caso venha a acontecer, temos que pensar no que fazer
            cout << "nao era pra poder acontecer isso, fudeu";    
            return nullptr; 
        }
        if (no->mods.campo != nenhum && no->mods.ver > modificacao_nova.ver)
        {
            cout << "não era pra acontecer isso, também. Fudeu.";
            return nullptr;
        }
        //esse if é desnecessário. Essa função só será chamada se tiver um nó com uma modificação que estourou
        //e só pode ser modificado um nó a partir de uma versão maior do que a atual. 
        no_novo->esq = no->esq;
        no_novo->dir = no->dir;
        no_novo->pai = no->pai;
        no_novo->isRoot = no->isRoot;
        no_novo->chave = no->chave; 
        no_novo->mods = Mod(); // Limpa as modificações da cópia (será preenchida agora)
        
        switch (no->mods.campo){
        case esq: no_novo->esq = no->mods.modPointer; break;
        case dir: no_novo->dir = no->mods.modPointer; break;
        case pai: no_novo->pai = no->mods.modPointer; break;
        case raiz: no_novo->isRoot = no->mods.modChave; if(no_novo->isRoot) no_novo->pai = nullptr; break;
        case chave: no_novo->chave = no->mods.modChave; break;
        default: break;
        }
        if(no_novo->dir != nullptr)
        cout << "dir: " << no_novo->dir->chave<< "\n";;
        switch (modificacao_nova.campo){
        case esq: no_novo->esq = modificacao_nova.modPointer; break;
        case dir: no_novo->dir = modificacao_nova.modPointer; break;
        case pai: no_novo->pai = modificacao_nova.modPointer; break;
        case raiz: no_novo->isRoot = modificacao_nova.modChave; if(no_novo->isRoot) no_novo->pai = nullptr; break;
        case chave: no_novo->chave = modificacao_nova.modChave; break;
        default: break;
        }
        no_novo->ver = modificacao_nova.ver; 
        cout << "\n";
        if(no->mods.modPointer != nullptr)
        cout << "modpointer :" << no->mods.modPointer->chave<< "\n";; 
        cout << "no novo todo: \n";
        cout << no_novo->chave << "\n";
        if(no_novo->esq != nullptr)
        cout << "esq: " << no_novo->esq->chave<< "\n";;
        if(no_novo->dir != nullptr)
        cout << "dir: " << no_novo->dir->chave<< "\n";
        if(no_novo->pai != nullptr)
        cout << "pai:" << no_novo->pai->chave<< "\n"; 

        // Adiciona a cópia ao vetor de todos os nós para gerenciamento de memória
        tree->nodes.push_back(no_novo);

        // Se o novo nó se torna raiz, seu pai é nulo e atualiza a raiz da árvore em construção
        if (no_novo->isRoot){
            no_novo->pai = nullptr; //desnecessário pois já fazemos isso antes, mas nunca se sabe. 
            tree->root = no_novo;
        }
        // AGORA, ajusta os ponteiros de quem apontava para o nó original para apontar para a cópia (no_novo)
        // Isso deve ser feito na *nova versão* que está sendo construída (tree->vers)
        // Se o nó original tinha um pai, precisamos atualizar o ponteiro do pai
        else if (temp_original.pai != nullptr){
            Node tempPaiNo;
            aplicaMods(temp_original.pai, &tempPaiNo, modificacao_nova.ver); // Materializa o pai do 'no' na versao_base
            // O pai de 'no' (temp_original.pai) deve apontar para 'no_novo' na nova versão
            if (tempPaiNo.esq == no) { // Se 'no' era filho esquerdo do seu pai
                modificar(tree, no_novo->pai, modificacao_nova.ver, esq, NONE, no_novo);
            } else if (tempPaiNo.dir == no) { // Se 'no' era filho direito do seu pai
                modificar(tree, no_novo->pai, modificacao_nova.ver, dir, NONE, no_novo);
            }
        } 
        // Ajusta os ponteiros de filhos do nó original para que seus pais apontem para 'no_novo'
        // (A cópia do nó original se torna o pai para seus filhos na nova versão)
        if(temp_original.esq != nullptr){
            modificar(tree, no_novo->esq, modificacao_nova.ver, pai, NONE, no_novo);
        }
        if(temp_original.dir != nullptr){
            modificar(tree, no_novo->dir, modificacao_nova.ver, pai, NONE, no_novo);
        }
        cout << "fim da cadeia.. :";
        if(no_novo->esq != nullptr)
        cout << "esq: " << no_novo->esq->chave; 
        if(no_novo->dir != nullptr)
        cout << "dir: " << no_novo->dir->chave; 
        return no_novo;
    }

    // Esta função decide se uma modificação é aplicada diretamente ou se uma cópia é necessária.
    // Retorna o nó (original ou uma nova cópia) que foi modificado e que agora pertence à versao_alvo.
    Node* modificar(BST *tree, Node *no, int versao_alvo, field campo, int valor_chave = NONE, Node* valor_no = nullptr){
        if (no == nullptr) return nullptr;
        cout << "MODIFICA!! \n";
        Mod modifica;
        modifica.ver = versao_alvo;
        modifica.campo = campo;
        modifica.modChave = valor_chave;
        modifica.modPointer = valor_no;
        // Se o nó ainda não tem modificação para esta versão
        // Isso significa que podemos aplicar a modificação diretamente ao nó sem criar uma cópia
        if (no->mods.campo == nenhum) {
            no->mods = modifica;
            if(modifica.modPointer !=nullptr)
            cout << "modificacao: " << modifica.modPointer->chave;
            cout << "ver da modificacao : " << modifica.ver; 
            if (campo == raiz && valor_chave == 1) { // Se a modificação for tornar o nó raiz
                tree->root = no;
            }
            cout << "preenchendo" << no->chave << " com: ";
            cout << valor_no->chave << " "; 
            if(no->esq != nullptr)
            cout << "esq: " << no->esq->chave; 
            if(no->dir != nullptr)
            cout << "dir: " << no->dir->chave; 
            cout << "testing stuff:";
            cout << no->chave << "\n";
            cout << no->mods.ver << "\n";
            if(no->mods.modPointer !=nullptr)
            cout << no->mods.modPointer->chave << "\n";
            return no; // Retorna o nó original, que foi modificado
        } else { // O nó já tem uma modificação para esta 'versao_alvo', então precisamos de uma nova cópia na cadeia
            // A versão base para aplicaMods dentro de atualizaCadeia deve ser a versão da modificação existente no nó.
            cout << "lotou... " << no->chave << "com : ";
            if (valor_no != nullptr)
            cout << valor_no->chave << " ";  
            if(no->esq != nullptr)
            cout << "esq: " << no->esq->chave; 
            if(no->dir != nullptr)
            cout << "dir: " << no->dir->chave;
            return atualizaCadeia(tree, no, no->mods.ver, modifica);
        }
    }
    
    void aplicaMods(Node *atual, Node *temp, int version_query){
        // em resumo, um atual = temp.
        // Esta função "materializa" o estado de um nó para a 'version_query'
        *temp = *atual; // Copia o estado base do nó (campos e ponteiros base)

        // Se houver uma modificação válida para a 'version_query', aplica-a
        // A modificação é válida se for para a versão exata ou uma mais antiga a que estamos consultando
        if ((atual->mods.campo != nenhum) && (atual->mods.ver <= version_query)) {
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
        cout << "BUSCA POR " << k << "!! \n";
        // A raiz da nova versão (tree->root) deve ser inicializada a partir da raiz da versão base.
        // Qualquer modificação subsequente irá atualizar tree->root para a nova cópia se a raiz mudar.
        tree->root = tree->versions[version_base];
        Node *no_novo = new Node();
        no_novo->chave = k;
        no_novo->ver = version_base+1;
        tree->nodes.push_back(no_novo); // Adiciona o nó novo à lista de todos os nós

        if (tree->root == nullptr) { // Árvore base está vazia
            // O nó novo se torna a raiz da nova versão
            no_novo->isRoot = true; 
            tree->root = no_novo; 
            tree->versions.push_back(no_novo);
            tree->vers++; 
            return; 
        }
        cout << "ver da raiz: " << tree->root->ver << " - " << tree->root->mods.ver; 
        Node *no_atual_search = tree->root;
        Node *no_anterior_search = nullptr;

        Node no_temp_atual;

        while (no_atual_search != nullptr) {
            aplicaMods(no_atual_search, &no_temp_atual, version_base);

            no_anterior_search = no_atual_search;
            // Nao precisamos materializar no_anterior_search aqui, ele é apenas uma referencia ao no_atual_search antes do loop.
            // aplicaMods(no_anterior_search, &no_temp_anterior, version_base); // Removido: Redundante
            cout << no_atual_search->chave << " - "; 
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
        if (no_novo->chave < no_temp_anterior_materialized.chave) 
        {
            pai_modificado = modificar(tree, no_anterior_search, version_base+1, esq, NONE, no_novo);
        } 
        else 
        {
            pai_modificado = modificar(tree, no_anterior_search, version_base+1, dir, NONE, no_novo);
        }

        // Define o pai do novo nó. O pai do no_novo é o nó retornado por modificar acima.
        no_novo->pai = pai_modificado; 

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

    Node* transplantar(BST *tree, Node* u, Node* v, int versao_base) { // int versao)
        // 'u' é o nó a ser substituído, 'v' é o nó que o substitui
        // 'versao_base' é a versão da qual estamos lendo para determinar os pais/filhos

        Node tempU;
        aplicaMods(u, &tempU, versao_base); // Estado de 'u' na versão base

        // se u for raiz, então v também será raiz
        if (tempU.isRoot) {
            if (v != nullptr) 
            { // Se 'v' existe, ele se torna a nova raiz
                Node *no_novo = new Node();
                Node temp_original;
                aplicaMods(v, &temp_original, versao_base);
                *no_novo = temp_original;
                no_novo->mods = Mod();
                no_novo->ver = versao_base+1;
                no_novo->isRoot = true; 
                no_novo->pai = nullptr; 
                // Adiciona a cópia ao vetor de todos os nós para gerenciamento de memória
                tree->nodes.push_back(no_novo);
                return no_novo; 
            } 
            else 
            {   // Se 'v' é nulo e 'u' era a raiz, a nova raiz da versão atual é nula
                // Não há um nó para modificar para raiz=0, então apenas define tree->root
                tree->root = nullptr;
                return nullptr; 
            }
        }
        else // 'u' não era a raiz
        {
            Node tempPai;
            if(v != nullptr)
            {
                Node *no_novo = new Node();
                Node temp_original;
                aplicaMods(v, &temp_original, versao_base);
                *no_novo = temp_original;
                no_novo->mods = Mod();
                no_novo->ver = versao_base+1;
                no_novo->isRoot = false; 
                no_novo->pai = tempU.pai; 
                tree->nodes.push_back(no_novo);
                
                if(tempU.pai != nullptr)
                    aplicaMods(tempU.pai, &tempPai, versao_base); // Estado do pai de 'u' na versão base
                
                if (tempPai.esq == u) // se u for filho esquerdo de pai, então v também será filho esquerdo
                    modificar(tree, tempU.pai, versao_base+1, esq, NONE, no_novo);
                else if (tempPai.dir == u) // se u for filho direito de pai, então v também será filho direito
                    modificar(tree, tempU.pai,  versao_base+1, dir, NONE, no_novo);
                return no_novo;
            }
            else //caso em que v é nulo
            {
                if(tempU.pai != nullptr)
                aplicaMods(tempU.pai, &tempPai, versao_base); // Estado do pai de 'u' na versão base
                if (tempPai.esq == u) // se u for filho esquerdo de pai, então v também será filho esquerdo
                modificar(tree, tempU.pai, versao_base+1, esq, NONE, nullptr);
                else if (tempPai.dir == u) // se u for filho direito de pai, então v também será filho direito
                modificar(tree, tempU.pai,  versao_base+1, dir, NONE, nullptr);
                return nullptr; 
            }
        }
    }

    void remover(BST *tree, int chave, int versao_base) { // int versao)
        // 'versao_base' é a versão da qual estamos partindo para criar a nova versão
        cout << "REMOVE!! \n";
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
        cout << "ver da raiz: " << tree->root->ver << " - " << tree->root->mods.ver; 
        Node tempAlvo;
        cout << "mods alvo ver: " << alvo->mods.ver;
        cout << "aaah" << "ver base: " << versao_base << "\n ";
        aplicaMods(alvo, &tempAlvo, versao_base); // aplica as modificações do nó alvo para a versão desejada
        cout << "mods tempAlvo ver: " << tempAlvo.mods.ver;
        Node* tempNovo; 
        if (tempAlvo.esq == nullptr && tempAlvo.dir != nullptr) {
            tempNovo = transplantar(tree, alvo, tempAlvo.dir, versao_base); // se não tiver filho esquerdo, transplanta o filho direito
        } 
        else if (tempAlvo.dir == nullptr && tempAlvo.esq != nullptr) {
            tempNovo = transplantar(tree, alvo, tempAlvo.esq, versao_base); // se não tiver filho direito, transplanta o filho esquerdo
        }
        else if(tempAlvo.esq == nullptr && tempAlvo.dir == nullptr) {
            cout << "aaah";
            if (tempAlvo.isRoot){ //remove e a árvore fica vazia
                tree->versions.push_back(nullptr);
                tree->vers++;
                return; 
            }
            else{
                if(tempAlvo.pai->chave > tempAlvo.chave) modificar(tree, tempAlvo.pai, versao_base+1, esq, NONE, nullptr);
                else modificar(tree, tempAlvo.pai, versao_base+1, dir, NONE, nullptr);
                tree->versions.push_back(tree->root);
                tree->vers++;
                cout << "fim remoção pwww";
                return;
            }
        }
        else {
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
        cout << "mods do no buscando: " << temp.mods.ver;
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