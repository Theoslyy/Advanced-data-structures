#include <bits/stdc++.h>
#include "parser.hpp"
#include "BST.cpp"

using namespace std;

/*Comunicação entre as funções de Parser e a árvore BST, chamada das funções de escrever artigo de saída.*/
int main(){

    //Testes
    cout<<"hello Theo e Galvao"<<endl;

    vector<operation> v = parser_text();

    for(operation it : v) cout << get<0>(it) << " " << get<1>(it) << " " << get<2>(it) << endl;

    return 0;
    
}