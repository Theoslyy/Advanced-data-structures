#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#define NULLKEY -100000

using namespace std;
enum field; struct Mod; struct Node; struct BST;

// Enum for what field a mod is modifying. 
enum field{
    left, right, key, root, none
};

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
    Node* chainUpdate(BST* tree, Node* node, Mod modification){
        Node* newNode = new Node(node->left, node->right, node->nodeKey, node->nodeVersion, node->isRoot); 
        applyMods(node, newNode);
        switch(modification.mod_field){
                case field::key: newNode->nodeKey = modification.mod_key; break;
                case field::left: newNode->left = modification.mod_pointer; break;
                case field::right: newNode->right = modification.mod_pointer; break;
                case field::root: newNode->isRoot = modification.mod_key; break;
                case field::none: break;
                default: break;
            }
            if (modification.mod_field == field::root && modification.mod_key == 1){
            tree->root = newNode;
            }
        newNode->nodeVersion = modification.version;
        //to - do: (done)
        /*
        When we create this new node, there will be a node stil pointing to the old node. 
        This is the parent. Given that we are not keeping the parent pointer,
        we need to do a search for this parent, find it and update his left or right pointer. 
        so,*/
        //even if the parent's mods get filled and we end up creating a new node,
        //we don't need to be worried about updating the current New Node given that we have no parent pointer. 
        Node* tempParent = search(tree, node->nodeKey);
        if(newNode->nodeKey < tempParent->nodeKey){
            Mod newmodification(modification.version, field::left, NULLKEY, newNode);
            insertMod(tree, tempParent, newmodification); 
        }
        else{
            Mod newmodification(modification.version, field::right, NULLKEY, newNode);
            insertMod(tree, tempParent, newmodification);
        }
        return node; 

    }

    Node* insertMod(BST* tree, Node* node, Mod newModification){
        if (node == nullptr) return nullptr;  
        if (node->nodeVersion == newModification.version){
            switch(newModification.mod_field){
                case field::key: node->nodeKey = newModification.mod_key; break;
                case field::left: node->left = newModification.mod_pointer; break;
                case field::right: node->right = newModification.mod_pointer; break;
                case field::root: node->isRoot = newModification.mod_key; break;
                case field::none: break;
                default: break;
            }
            if (newModification.mod_field == field::root && newModification.mod_key == 1){
            tree->root = node;
            }
            return node; 
        }

        if (node->modifications[0].mod_field == field::none) node->modifications[0] = newModification; 
        else if (node->modifications[1].mod_field == field::none) node->modifications[1] = newModification;
        else{
            return chainUpdate(tree, node, newModification);
        }
        if (newModification.mod_field == field::root && newModification.mod_key == 1){
            tree->root = node;
        }
        return node; 
    }
    //we will be inserting on version tree->tree_version + 1
    void insertNode(BST* tree, int k){
        Node* newNode = new Node(nullptr, nullptr, k, tree->tree_version+1, false);
        if (tree->tree_version == -1){
            tree->root = newNode; 
            newNode->isRoot = true;
            tree->nodes.push_back(newNode); 
            tree->versions.push_back(newNode);
            tree->tree_version++; 
            return; 
        }
        Node* root = tree->versions[tree->tree_version];
        if (root == nullptr){
            tree->root = newNode; 
            newNode->isRoot = true;
            tree->nodes.push_back(newNode); 
            tree->versions.push_back(newNode);
            tree->tree_version++; 
            return;
        }
        Node* tempNode = new Node(root->left, root->right, root->nodeKey, root->nodeVersion, true);
        Node* tempParent = root;
        applyMods(root, tempNode); 
        vector<Node*> path;
        //path.push_back(tempParent); if we wanted, we could save the path downwards in a vector
        //this way, we'd keep all the 'original' parents. 
        while (tempNode != nullptr){
            if(newNode->nodeKey < tempNode->nodeKey){
                if(tempNode->left != nullptr){
                Node* tempLeft = new Node(tempNode->left->left, tempNode->left->right, 
                    tempNode->left->nodeKey, tempNode->left->nodeVersion, false);
                applyMods(tempNode->left, tempLeft);
                tempParent = tempNode->left; //this way, tempParent is the actual node in the BST, not a copy.
                tempNode = tempLeft; 
                }
                else tempNode == nullptr; 
            }
            else{
                if(tempNode->right != nullptr){
                Node* tempRight = new Node(tempNode->right->left, tempNode->right->right, 
                    tempNode->right->nodeKey, tempNode->right->nodeVersion, false);
                applyMods(tempNode->right, tempRight);
                tempParent = tempNode->right; //this way, tempParent is the actual node in the BST, not a copy.
                tempNode = tempRight; 
                }
                else tempNode == nullptr;
            }
        }
        if(newNode->nodeKey < tempParent->nodeKey){ //nodes don't change keys, we don't need to apply anything here.
            Mod newmodification(tree->tree_version+1, field::left, NULLKEY, newNode);
            tempParent = insertMod(tree, tempParent, newmodification); //this might return a new node?
        }
        else{
            Mod newmodification(tree->tree_version+1, field::right, NULLKEY, newNode);
            tempParent = insertMod(tree, tempParent, newmodification); //this might return a new node?
        }
        tree->nodes.push_back(newNode);
        tree->versions.push_back(tree->root);
        tree->tree_version++;
        delete tempNode;
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

};