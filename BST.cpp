#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

using namespace std;

struct Mod{
    int ver; // versao a se modificada
    int campo; // campo a ser modificado 
    int valor; // valor a ser modificado
    // Struct para as modificações, talvez não seja necessária a implementação de um struct para isso, mas, é.
};

struct Node{
    Node* esq; // ponteiro filho esquerdo
    Node* dir; // ponteiro filho direito
    Node* pai; // ponteiro pai!
    bool isFolha; // <- esse campo não é necessário mas eu uso ele para uma coisinha da busca. 
    int chave; // <- talvez isso não seja guardado em uma só variável mas sim esteja em um vetor
    // + um vetor com as modificações de cada versão
    // Node* retorno[p]; <- vetor com os ponteiros de retorno. Usando um array só porque é menos custoso ? pode ser um vector 
    // Mods modificação[2p];
    // pela definição do professor, 2p = 99, ou seja, teremos no max 99 modificações.  
};

struct BST{
    Node* raiz; 

    Node* busca(Node* no, int k){
        if(no->isFolha || k == no->chave){ // ser folha significa não ter filhos! Ou seja, paramos antes de descer num nó nulo. Poderiamos fazer um if no == nil, também.
            return no; 
        }
        if (k < no->chave) return busca(no->esq, k);
        else return busca(no->dir, k);
    }
    void inserir(BST* arvore, Node* no_novo){
        Node* no_ant; // no anterior
        Node* no_atual = arvore->raiz;  //prox no
        while (no_atual != nullptr){
            no_ant = no_atual;
            if(no_novo->chave < no_atual->chave){
                no_atual = no_atual->esq; 
            }
            else no_atual = no_atual->dir; 
        } 
        no_novo->pai = no_ant; 
        if(no_ant == nullptr){ // arvore vazia
            arvore->raiz = no_novo; 
        }
        else if(no_novo->chave < no_ant->chave){
            no_ant->esq = no_novo;
        }
        else no_ant->dir = no_novo; 
    }
    /*
        TO-DOS árvore: sucessor, deleção (depende de sucessor), imprimir -> é um percurso em ordem. 
        TO-DOS persistência: implementar as operações considerando as versões. 
    */

};