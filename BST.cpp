#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

using namespace std;

struct Mod
{
    int ver;   // versao a se modificada, como a persistência é parcial, sempre modificamos na última versão
    string campo; // campo a ser modificado
    int modChave;
    Node* modPointer; 
    //Mod() : ver(0), campo(0), valor(0) {}
};

struct Node
{
    Node *esq;    // ponteiro filho esquerdo
    Node *dir;    // ponteiro filho direito
    Node *pai;    // ponteiro pai!
    int chave;    // <- talvez isso não seja guardado em uma só variável mas sim esteja em um vetor
    // + um vetor com as modificações de cada versão
    //Node* retorno[50]; //<- vetor com os ponteiros de retorno de tamanho p. Usando um array só porque é menos custoso ? pode ser um vector
    vector<Mod> mods; 
    // como a árvore binária de busca já mantém ponteiros para o nó 
    // esquerdo e direito, não precisamos do vetor de retorno, já temos os ponteiros, 
    // então, basta anotar mudanças nesses ponteiros como mods     
    Node() // essa forma de inicializar as coisa é tão bonita...
    : esq(nullptr), dir(nullptr), pai(nullptr),
      chave(0) {}
};

struct BST
{
    // aqui, na verdade, temos que ter um vetor com uma tupla (raiz, versao),
    // pois toda versão da árvore tem que ter especificada sua raiz. 
    Node *raiz;
    int vers = 0;

    void atualizaCadeia(Node *no, int versao, Mod modificacao_nova){
        Node *novo_no; 
        novo_no->esq = no->esq; 
        novo_no->dir = no->dir;
        novo_no->pai = no->pai;
        //por favor transformar isso num switch case . 
        if(no->mods[0].campo == "esq")
            novo_no->esq = no->mods[0].modPointer; 
        else if(no->mods[0].campo == "dir")
            novo_no->dir = no->mods[0].modPointer; 
        else if(no->mods[0].campo == "pai")
            novo_no->pai = no->mods[0].modPointer; 
        else
            novo_no->chave = no->mods[0].modChave; 
        // mods que vieram do o nó anterior ^
        // mods que acabou de ser aplicada:
        if(modificacao_nova.campo == "esq")
            novo_no->esq = modificacao_nova.modPointer; 
        else if(modificacao_nova.campo == "dir")
            novo_no->dir = modificacao_nova.modPointer; 
        else if(modificacao_nova.campo == "pai")
            novo_no->pai = modificacao_nova.modPointer; 
        else
            novo_no->chave = modificacao_nova.modChave; 
        // como esse mod foi o que acabou de ser passado e estorou o vetor de mods do nó
        // ele não é anotado como nova modificação e, na verdade, é o estado atual do nó 
        //mods aplicados, agora modifica os ponteiros de retorno
        if (no->pai->esq == no)
            modificar(no->pai, versao, "esq", novo_no);
        else
            modificar(no->pai, versao, "dir", novo_no); 
        modificar(no->esq, versao, "pai", novo_no);
        modificar(no->dir, versao, "pai", novo_no);
    }
    template <typename T> 
    void modificar(Node *no, int versao, string campo, T valor){
        Mod modifica; 
        if (no->mods.empty()){
            if(is_same<decltype(valor), Node*>){
                modifica.ver = versao; modifica.campo = campo; modifica.modPointer = valor; 
                no->mods[0] = modifica; //<- ou seja, se ainda há espaço para novas modificações, eu só taco elas no vetor de modificações
            }
            else{
                modifica.ver = versao; modifica.campo = campo; modifica.modChave = valor; 
                no->mods[0] = modifica;
            }
            
        }
        else{
             if(decltype(valor) == Node*){
                modifica.ver = versao; modifica.campo = campo; modifica.modPointer = valor; 
            }
            else{
                modifica.ver = versao; modifica.campo = campo; modifica.modChave = valor; 
            }
            atualizaCadeia(no, versao, modifica) //<- se o campo de mods estiver cheio, temos que fazer o processo de atualização em cadeia:
            /*
            1. Criar nova cópia deste nó
            2. Aplicar todos os mods para obter novos campos originais neste novo nó 
            3. mods vazio
            4. Quem aponta para o nó na última versão passa a apontar para o novo nó na nova versão
            */
        }
        //se alguma atualização mudar ponteiros de retorno, atualizar os ponteiros de retorno do nó que atingimos, chama a função de novo!
        //Atualizações em cascata mantém-se na mesma versão.
        return; 
    }

    Node *busca(Node *no, int k) //, int version) -> essa função é necessária? 
    { // buscar em uma versao especifica requer aplicar as mods enquanto se desce na árvore para saber os filhos para o qual o nó aponta
        if ((no->esq == nullptr && no->dir == nullptr) || k == no->chave)
        { // ser folha significa não ter filhos! Ou seja, não tem filho direito nem esquerdo.
            return no;
        }
        if (k < no->chave)
            return busca(no->esq, k);
        else
            return busca(no->dir, k);
    }
    void inserir(BST *arvore, Node *no_novo){ // estaremos na versão x e criaremos a versão x+1 -> modificar pra receber um int
        //assumimos que a inserção vai acontecer na versão arvore->vers. Só incrementamos a versão dps
        //de fazer uma modificação, não antes. Ou seja, começamos da 'versão 0'
        Node *no_ant;                  // no anterior
        Node *no_atual = arvore->raiz; // prox no
        while (no_atual != nullptr){
            no_ant = no_atual;
            if (no_novo->chave < no_atual->chave)
            {
                no_atual = no_atual->esq;
            }
            else
                no_atual = no_atual->dir;
            }
        no_novo->pai = no_ant;
        if (no_ant == nullptr){ 
            //arvore vazia, no novo é raiz.
            arvore->raiz = no_novo;
            }
        else if (no_novo->chave < no_ant->chave){ 
            //no novo é filho esquerdo de no_ant:
            no_novo->pai = no_ant; // no_ant aponta para no_novo, logo, o array de retorno de no_novo tem que ter no_ant
            // como o nó novo foi criado agora, não chamamos a função modifica, esses são os valores iniciais dele, e não 'modificações'
            // como no_novo tambem aponta para no_ant, temos que atualizar o array de retorno de no_ant.
            modificar(no_ant, arvore->vers + 1, "esq", no_novo); //-> modifica o no_ant na versão vers + 1 no campo esq com o valor no_novo.
            }
        else{
            //processo equivalente para caso no_novo seja filho direito
            no_novo->pai = no_ant;
            modificar(no_ant, arvore->vers + 1, "dir", no_novo); //-> modifica o no_ant na versão vers + 1 no campo dir com o valor no_novo.
            }
            arvore->vers++;
        }
    /*
        TO-DOS persistência: implementar as operações considerando as versões.
    */
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

    void transplantar(Node *&raiz, Node *u, Node *v)
    {
        if (u->pai == nullptr) // se u é raiz, então v substituirá
        {
            raiz = v;
        }
        else if (u == u->pai->esq) // se u é filho esquerdo, então u substituirá o filho esquerdo de seu pai
        {
            u->pai->esq = v;
        }
        else
        {
            u->pai->dir = v; // se u é filho direito, então u substituirá o filho direito de seu pai
        }
        if (v != nullptr)
        {
            v->pai = u->pai; // se v não for nulo, então pai de v será pai de u
        }
    }

    void remover(Node *&raiz, Node *no)
    {
        if (no->esq == nullptr)
        {
            transplantar(raiz, no, no->dir); // se não tiver filho esquerdo, então transplantamos o nó atual com o filho direito
        }
        else if (no->dir == nullptr)
        {
            transplantar(raiz, no, no->esq); // se não tiver filho direito, então analogamente transplantamos o nó atual com o filho esquerdo
        }
        else
        {
            Node *y = sucessor(no); // se tiver os dois filhos, então o sucessor será o mínimo da subárvore direita
            transplantar(raiz, y, y->dir);
            y->esq = no->esq;
            no->esq->pai = y;
            y->dir = no->dir;
            no->dir->pai = y;
            transplantar(raiz, no, y);
        }
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