#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define NONE -10000

using namespace std;

enum field {
    esq, dir, pai, chave, raiz, chave
};


struct Mod
{
    int ver;   // versao a se modificada, como a persistência é parcial, sempre modificamos na última versão
    field campo; // campo a ser modificado 
    int modChave; 
    Node* modPtr;
    Node* modPointer; 
    
    //Mod() : ver(0), campo(0), valor(0) {}
};

struct Node
{
    Node *esq;    // ponteiro filho esquerdo
    Node *dir;    // ponteiro filho direito
    Node *pai;    // ponteiro pai!
    vector<Mod> mods; // <- vetor de mods terá tamanho um
    int chave;    // 
    bool isRoot;
    // como a árvore binária de busca já mantém ponteiros para o nó 
    // esquerdo e direito, não precisamos do vetor de retorno, já temos os ponteiros, 
    // então, basta anotar mudanças nesses ponteiros como mods     
    Node() // essa forma de inicializar as coisa é tão bonita...
    : esq(nullptr), dir(nullptr), pai(nullptr),
      chave(0), isRoot(false) {}
};

struct BST
{
    Node *root; // raiz da versão vers - 1. vers é a 'versão a ser criada' na próx mod. 
    vector<Node*> nodes;
    vector<Node*> versions; // -> não precisa ser uma tupla de (int, node), o índice do vetor já dá em que versão estamos
    int vers; // desnecessário, é só ver o tamanho do vetor versions, mas ajuda na organização. 
    
    BST()
    :   root(nullptr), vers(0) {}

    void atualizaCadeia(BST* tree, Node *no, int versao, Mod modificacao_nova){
        // Criando o novo nó:
        Node *no_novo = new Node(); 
        no_novo->esq = no->esq; 
        no_novo->dir = no->dir;
        no_novo->pai = no->pai;
        no_novo->isRoot = no->isRoot;

        switch (no->mods[0].campo)
        // Não existe 
        {
        case esq:
            no_novo->esq = no->mods[0].modPointer; 
            break;

        case dir:
            no_novo->dir = no->mods[0].modPointer; 
            break;

        case pai:
            no_novo->pai = no->mods[0].modPointer; 
            break;

        case raiz:
            no_novo->isRoot  = no->mods[0].modChave;
            break;

        case chave:
            no_novo->chave = no->mods[0].modChave; 
            break;

        default:
            break;
        }
        // mods que vieram do o nó anterior ^
        // mods que acabou de ser aplicada:
        switch (modificacao_nova.campo)
        {
        case esq:
            no_novo->esq = modificacao_nova.modPointer; 
            break;

        case dir:
            no_novo->dir = modificacao_nova.modPointer; 
            break;

        case pai:
            no_novo->pai = modificacao_nova.modPointer; 
            break;

        case raiz:
            no_novo->isRoot  = modificacao_nova.modChave;
            break;

        case chave:
            no_novo->chave = modificacao_nova.modChave; 
            break;

        default:
            break;
        }
        // como esse mod foi o que acabou de ser passado e estorou o vetor de mods do nó
        // ele não é anotado como nova modificação e, na verdade, é o estado atual do nó 
        //mods aplicados, agora modifica os ponteiros de retorno
        if (no_novo->isRoot){
            no_novo->pai = nullptr;  
            tree->root = no_novo;
        }
        if (no->pai != nullptr){
            if (no->pai->esq == no) modificar(tree, no->pai, versao, esq, NONE, no_novo);
            else modificar(tree, no->pai, versao, dir, NONE, no_novo); 
        }
        if(no->esq != nullptr) modificar(tree, no->esq, versao, pai, NONE, no_novo);
        if(no->dir != nullptr) modificar(tree, no->dir, versao, pai, NONE, no_novo);
        tree->nodes.push_back(no_novo);
    }

    void modificar(BST *tree, Node *no, int versao, field campo, int valor_chave = NONE, Node* valor_no = nullptr){
        Mod modifica; 
        if (no->mods.empty()){ 
            if(campo != chave && campo != raiz){ //Ou seja, o campo é um no!
                modifica.ver = versao; modifica.campo = campo; modifica.modPointer = valor_no; 
                no->mods[0] = modifica; //<- ou seja, se ainda há espaço para novas modificações, eu só taco elas no vetor de modificações

            }
            else if(campo == chave) {
                modifica.ver = versao; modifica.campo = campo; modifica.modChave = valor_chave; 
                no->mods[0] = modifica;
            }
            else{ // caso em que é uma raiz!
                modifica.ver = versao; modifica.campo = campo; 
                modifica.modChave = valor_chave; //já que bool -> 0 ou 1, usamos valor chave mod do bool raiz 
                no->mods[0] = modifica;
                if (valor_chave == 1)
                tree->root = no; 
            }
            
        }
        else{
            if(campo == esq || campo == dir || campo == pai){ 
                //podemos mudar o pai de alguém para nullptr (raiz)
                //No entanto, assim que um nó vira raiz, há duas modificações sobre ele:
                //1. virar raiz; 2. apontar para null
                //Assim... Temos que tratar como só uma modificação.
                modifica.ver = versao; 
                modifica.campo = campo; 
                modifica.modPointer = valor_no; 

            }
            else{
                modifica.ver = versao; 
                modifica.campo = campo; 
                modifica.modChave = valor_chave; 
            }

            atualizaCadeia(tree, no, versao, modifica); //<- se o campo de mods estiver cheio, temos que fazer o processo de atualização em cadeia:
            /*
            1. Criar nova cópia deste nó
            2. Aplicar todos os mods para obter novos campos originais neste novo nó 
            3. mods vazio
            4. Quem aponta para o nó na última versão passa a apontar para o novo nó na nova versão
            */
        }
        //se alguma atualização mudar ponteiros de retorno, atualizar os ponteiros de retorno do nó que atingimos, chama a função de novo!
        //Atualizações em cascata mantém-se na mesma versão.
        tree->versions.push_back(tree->root); //nova versão vai para o vetor de versões. 
        return; 
    }
    void aplicaMods(Node *atual, Node *temp, int version){

        // em resumo, um atual = temp. 

        temp->chave = atual->chave;
        temp->esq = atual->esq;
        temp->dir = atual->dir;
        temp->pai = atual->pai;
        temp->isRoot = atual->isRoot; 

        if((atual->mods.empty()) || (atual->mods[0].ver > version)) return;
        
        switch (atual->mods[0].campo){

            case esq:
                temp->esq = atual->mods[0].modPointer; 
                break;

            case dir:
                temp->dir = atual->mods[0].modPointer; 
                break;

            case pai:
                temp->pai = atual->mods[0].modPointer;  
                break;

            case raiz:
                temp->isRoot = atual->mods[0].modChave; 
                if(temp->isRoot) temp->pai = nullptr; 
                break;

            case chave:
                temp->chave = atual->mods[0].modChave;                
                break;
        }
    }


    void inserir(BST *tree, int k, int version){ 
        // inserção é sempre na versão tree->vers, ou seja, na última versão da árvore
        //assumimos que a inserção vai acontecer na versão arvore->vers. Só incrementamos a versão dps
        //de fazer uma modificação, não antes. Ou seja, começamos da 'versão 0'
        Node *no_novo = new Node();        
        tree->nodes.push_back(no_novo);    
        no_novo->chave = k;
        Node *no_atual = tree->versions[version]; //começamos da raiz. 
        if(no_atual == nullptr){
            no_novo->isRoot == true;
            tree->root = no_novo;
            tree->versions.push_back(tree->root);
            tree->vers++; 
            return; 
            //caso em que o nó é raiz. 
        }
        Node *no_temp = nullptr; 
        while (no_atual != nullptr){
            aplicaMods(no_atual, no_temp, version);
            //no_ant = no_atual;
            if (no_novo->chave < no_temp->chave)
                no_atual = no_atual->esq;
            else
                no_atual = no_atual->dir;
        }
        no_novo->pai = no_temp;
        if(no_novo->chave < no_temp->chave){ 
            //no novo é filho esquerdo de no_ant:
            no_novo->pai = no_temp; // no_ant aponta para no_novo, logo, o array de retorno de no_novo tem que ter no_ant
            // como o nó novo foi criado agora, não chamamos a função modifica, esses são os valores iniciais dele, e não 'modificações'
            // como no_novo tambem aponta para no_ant, temos que atualizar o array de retorno de no_ant.
            modificar(tree, no_temp, tree->vers + 1, esq, NONE, no_novo); //-> modifica o no_ant na versão vers + 1 no campo esq com o valor no_novo.
            }
        else{
            //processo equivalente para caso no_novo seja filho direito
            no_novo->pai = no_temp;
            modificar(tree, no_temp, tree->vers + 1, dir, NONE, no_novo); //-> modifica o no_ant na versão vers + 1 no campo dir com o valor no_novo.
            }
        tree->vers++;
        }
    Node *minimo(Node *no)
    {
        while (no->esq != nullptr) // percorre a árvore até o nó mais a esquerda (que na BST é o de menor valor)
        {
            no = no->esq;
        }
        return no;
    }

    Node *sucessor(Node *no)
    {
        if (no->dir != nullptr)
        {
            return minimo(no->dir); // se tiver filho a direita, o sucessor é o mínimo da subárvore direita
        }
        Node *y = no->pai;
        while (y != nullptr && no == y->dir) // se não tiver, então o sucessor é o primeiro ancestral que for pai do nó atual no qual atual é filho esquerdo
        {
            no = y;
            y = no->pai;
        }
        return y;
    }

    void transplantar(BST *tree, Node* u, Node* v, int versao) 
    {
        Node tempU;
        aplicaMods(u, &tempU, versao);

        // se u for raiz, então v também será raiz
        if (tempU.isRoot) modificar(tree, v, versao + 1, raiz, 1, nullptr); // v não tem pai, pois é raiz
        else
        {
            Node tempPai;
            aplicaMods(tempU.pai, &tempPai, versao);

            if (tempPai.esq == u)
            {
                modificar(tree, tempU.pai, versao + 1, esq, NONE, v); // se u for filho esquerdo de pai, então v também será filho esquerdo
            }
            else
            {
                modificar(tree, tempU.pai, versao + 1, dir, NONE, v); // se u for filho direito de pai, então v também será filho direito
            }

            if (v != nullptr)
            {
                modificar(tree, v, versao + 1, pai, NONE, tempU.pai); // v passa a ser filho de pai
            }
        }
    }

    void remover(BST *tree, int chave, int versao) {
        Node* raiz = tree->versions[versao];
        if(!raiz) return;

        Node* alvo = busca(tree, chave, versao);
        if(!alvo) return; // se não encontrar o nó, não faz nada

        Node tempAlvo;
        aplicaMods(alvo, &tempAlvo, versao); // aplica as modificações do nó alvo para a versão desejada

        if(tempAlvo.esq == nullptr) {
            transplantar(tree, alvo, tempAlvo.dir, versao); // se não tiver filho esquerdo, transplanta o filho direito
        }
        else if(tempAlvo.dir == nullptr) {
            transplantar(tree, alvo, tempAlvo.esq, versao); // se não tiver filho direito, transplanta o filho esquerdo
        }
        else {
            Node* sucessor = tree->sucessor(alvo); // se tiver os dois filhos, encontra o sucessor
            Node tempSucessor;
            aplicaMods(sucessor, &tempSucessor, versao); // aplica as modificações do sucessor para a versão desejada

            if (tempSucessor.pai != alvo) {
                transplantar(tree, sucessor, tempSucessor.dir, versao); // transplanta o sucessor por seu próprio filho direito
                modificar(tree, sucessor, versao, dir, NONE, tempAlvo.dir); // atualiza o filho direito do sucessor
                modificar(tree, sucessor, versao, pai, NONE, tempAlvo.pai); // atualiza o pai do sucessor
            }

            transplantar(tree, alvo, sucessor, versao); // transplanta o nó alvo pelo sucessor
            modificar(tree, sucessor, versao, esq, NONE, tempAlvo.esq); // atualiza o filho esquerdo do sucessor
            modificar(tree, tempAlvo.esq, versao, pai, NONE, sucessor); // atualiza a chave do sucessor
        }

        tree->versions.push_back(tree->root); // adiciona a nova versão da árvore após a remoção
        tree->vers++;
    }

    Node *busca(BST* tree, int k, int versao) //, int version) -> essa função é necessária?
    {                            // buscar em uma versao especifica requer aplicar as mods enquanto se desce na árvore para saber os filhos para o qual o nó aponta
       return buscaVersao(tree->versions[versao], k, versao);
    }

    Node* buscaVersao(Node* no, int k, int versao) {
        if (no == nullptr) return nullptr; // se o nó for nulo, não encontrou

        Node temp;
        aplicaMods(no, &temp, versao); // aplica as modificações do nó atual para a versão desejada
        if(k == temp.chave) {
            return no; // se a chave for igual, encontrou o nó
        }
        else if(k < temp.chave) {
            return buscaVersao(temp.esq, k, versao); // se a chave for menor, desce na subárvore esquerda
        } else {
            return buscaVersao(temp.dir, k, versao); // se a chave for maior, desce na subárvore direita
        }
    }

    int get_key(Node* node, int version){
        /*Função que lê corretamente a chave de um nó dada a sua versão
        Implementei de forma que nó possa ter várias modificações dentro
        dele, porém sabemos que só pode haver no máximo 1*/

        for(int index = 0; index < node->mods.size(); index++) {
        
            if(node->mods[index].ver > version) 
                return index > 0? node->mods[index-1].modChave : node->chave;

        } 


        return node->mods[node->mods.size() - 1].campo;


    }
    
    void DFS_REC(Node* node, int version, int profundidade, vector<pair<int,int>>& dfs_vector) {
        /*Função recursiva auxiliar que será usada para utilizar a DFS sem necessidade de dar 
            profundidade=0 como um dos parametros na função */

        if(node == nullptr) return;

        DFS_REC(node->esq, version, profundidade + 1, dfs_vector);
        dfs_vector.push_back(pair<int,int>(get_key(node, version), profundidade));
        DFS_REC(node->dir, version, profundidade + 1, dfs_vector);

    }
    
    vector<pair<int,int>> DFS(BST tree, int version){
        /*Função que retorna uma array com o par (Chave, profundidade de um nó) */

        Node* root = tree.versions[version];
        vector<pair<int,int>> dfs_vector;

        DFS_REC(root, version, 0, dfs_vector);

        return dfs_vector;
    }
    void imprimir(Node *no)
    {
        if (no != nullptr)
        {
            imprimir(no->esq);
            cout << no->chave << " ";
            imprimir(no->dir);
        }
    }    
};
