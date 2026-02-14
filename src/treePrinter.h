#ifndef TREEPRINTER_H
#define TREEPRINTER_H

#include <iostream>
#include <string>

#include "AVLTree.h"
#include "language.h" 

using namespace std;

// Struct to manage the "history" of the indentation
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
    // isLeft: true if this node is a left child
    template <typename T>
      static void printTree(AVLNode<T>* root, Trunk *prev, bool isLeft) {
        if (root == nullptr) {
          return;
        }

        string prev_str = "    ";
        Trunk *trunk = new Trunk(prev, prev_str);

        // Process Right Child (Top)
        printTree(root->right, trunk, true);

        // Print Current Node
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

        // Process Left Child (Bottom)
        if (prev) {
          prev->str = prev_str;
        }
        trunk->str = "   |";

        // We pass 'false' because the left child is the last one in this visual block
        printTree(root->left, trunk, false);

        // Clean up memory
        delete trunk;
      }

  public:
    template <typename T>
      static void print(AVLTree<T>& tree) {
        cout << "\n  +================================================+" << endl;
        cout << "  |  >> " << Tr("TREE VISUALIZATION", "VISUALISATION DE L'ARBRE", "عرض الشجرة") << endl;
        cout << "  +================================================+\n" << endl;

        if (tree.getRoot() == nullptr) {
          cout << "    " << Tr("(Empty Tree)", "(Arbre vide)", "(شجرة فارغة)") << endl;
          cout << "\n  +================================================+\n" << endl;
          return;
        }

        printTree(tree.getRoot(), nullptr, false);

        cout << "\n  +================================================+\n" << endl;
      }
};

#endif
