#ifndef TREEPRINTER_H
#define TREEPRINTER_H

#include <iostream>
#include <string>
#include "AVLTree.h" // Your AVL Header

using namespace std;

// Helper struct to manage the "history" of the indentation
struct Trunk {
    Trunk *prev;
    string str;

    Trunk(Trunk *prev, string str) {
        this->prev = prev;
        this->str = str;
    }
};

class TreePrinter {
private:
    // Helper to print the indentation branches
    static void showTrunks(Trunk *p) {
        if (p == nullptr) {
            return;
        }
        showTrunks(p->prev);
        cout << p->str;
    }

    // Recursive function to print the tree
    // root: current node
    // prev: pointer to the previous trunk (indentation history)
    // isLeft: true if this node is a left child
    template <typename T>
    static void printTree(AVLNode<T>* root, Trunk *prev, bool isLeft) {
        if (root == nullptr) {
            return;
        }

        string prev_str = "    ";
        Trunk *trunk = new Trunk(prev, prev_str);

        // 1. Process Right Child (Top)
        // We pass 'true' for indentation because the right child needs a vertical bar 
        // to connect to the left child below it.
        printTree(root->right, trunk, true);

        // 2. Print Current Node
        if (!prev) {
            trunk->str = "---"; // Root
        } else if (isLeft) {
            trunk->str = ".---"; // Left child (bottom branch)
            prev_str = "   |";
        } else {
            trunk->str = "`---"; // Right child (top branch)
            prev->str = prev_str;
        }

        showTrunks(trunk);
        cout << root->key << endl;

        // 3. Process Left Child (Bottom)
        if (prev) {
            prev->str = prev_str;
        }
        trunk->str = "   |";
        
        // We pass 'false' because the left child is the last one in this visual block
        printTree(root->left, trunk, false);
        
        // Clean up memory (optional but good practice)
        delete trunk;
    }

public:
    template <typename T>
    static void print(AVLTree<T>& tree) {
        if (tree.getRoot() == nullptr) {
            cout << "(Empty Tree)" << endl;
            return;
        }
        cout << "\n";
        printTree(tree.getRoot(), nullptr, false);
        cout << "\n";
    }
};

#endif
