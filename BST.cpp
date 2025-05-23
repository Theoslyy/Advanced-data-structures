#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

using namespace std;

struct Mod
{
    int ver;   // versao a se modificada
    int campo; // campo a ser modificado
    int valor; // valor a ser modificado
    // Struct para as modificações, talvez não seja necessária a implementação de um struct para isso, mas, é.
};

struct Node
{
    Node *esq;    // ponteiro filho esquerdo
    Node *dir;    // ponteiro filho direito
    Node *pai;    // ponteiro pai!
    bool isFolha; // <- esse campo não é necessário mas eu uso ele para uma coisinha da busca.
    int chave;    // <- talvez isso não seja guardado em uma só variável mas sim esteja em um vetor
    // + um vetor com as modificações de cada versão
    // Node* retorno[p]; <- vetor com os ponteiros de retorno. Usando um array só porque é menos custoso ? pode ser um vector
    // Mods modificação[2p];
    // pela definição do professor, 2p = 99, ou seja, teremos no max 99 modificações.
};

struct BST
{
    Node *raiz;

    Node *busca(Node *no, int k)
    {
        if (no->isFolha || k == no->chave)
        { // ser folha significa não ter filhos! Ou seja, paramos antes de descer num nó nulo. Poderiamos fazer um if no == nil, também.
            return no;
        }
        if (k < no->chave)
            return busca(no->esq, k);
        else
            return busca(no->dir, k);
    }
    void inserir(BST *arvore, Node *no_novo)
    {
        Node *no_ant;                  // no anterior
        Node *no_atual = arvore->raiz; // prox no
        while (no_atual != nullptr)
        {
            no_ant = no_atual;
            if (no_novo->chave < no_atual->chave)
            {
                no_atual = no_atual->esq;
            }
            else
                no_atual = no_atual->dir;
        }
        no_novo->pai = no_ant;
        if (no_ant == nullptr)
        { // arvore vazia
            arvore->raiz = no_novo;
        }
        else if (no_novo->chave < no_ant->chave)
        {
            no_ant->esq = no_novo;
        }
        else
            no_ant->dir = no_novo;
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