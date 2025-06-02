#include <bits/stdc++.h>
#include "parser.hpp"
#include "BST.cpp"

using namespace std;


/*Comunicação entre as funções de Parser e a árvore BST, chamada das funções de escrever artigo de saída.*/
int main(){

    //Testes

    string data_path;

    cin>>data_path;

    cout<<"hello Theo e Galvao"<<endl;

    vector<operation> v = parser_text(data_path);

    for(operation it : v) cout << get<0>(it) << " " << get<1>(it) << " " << get<2>(it) << endl;

    return 0;
    
}