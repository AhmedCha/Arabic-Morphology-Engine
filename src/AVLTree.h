// Code from https://www.geeksforgeeks.org/cpp/cpp-program-to-implement-avl-tree/

#ifndef AVLTREE_H  
#define AVLTREE_H 

#include <algorithm>
#include <iostream>
#include <fstream>  

using namespace std;

template <typename T> class AVLNode {
  public:
    T key; 
    AVLNode* left; 
    AVLNode* right; 
    int height; 

    // Constructor to initialize a node with a given key
    AVLNode(T k)
      : key(k)
        , left(nullptr)
        , right(nullptr)
        , height(1)
  {
  }
};

// Template class representing the AVL tree
template <typename T> class AVLTree {
  private:
    // Pointer to the root of the tree
    AVLNode<T>* root;
    string filename;

    // function to get the height of a node
    int height(AVLNode<T>* node)
    {
      if (node == nullptr)
        return 0;
      return node->height;
    }

    // function to get the balance factor of a node
    int balanceFactor(AVLNode<T>* node)
    {
      if (node == nullptr)
        return 0;
      return height(node->left) - height(node->right);
    }

    // function to perform a right rotation on a subtree
    AVLNode<T>* rightRotate(AVLNode<T>* y)
    {
      AVLNode<T>* x = y->left;
      AVLNode<T>* T2 = x->right;

      // Perform rotation
      x->right = y;
      y->left = T2;

      // Update heights
      y->height
        = max(height(y->left), height(y->right)) + 1;
      x->height
        = max(height(x->left), height(x->right)) + 1;

      // Return new root
      return x;
    }

    // function to perform a left rotation on a subtree
    AVLNode<T>* leftRotate(AVLNode<T>* x)
    {
      AVLNode<T>* y = x->right;
      AVLNode<T>* T2 = y->left;

      y->left = x;
      x->right = T2;

      // Update heights
      x->height
        = max(height(x->left), height(x->right)) + 1;
      y->height
        = max(height(y->left), height(y->right)) + 1;

      // Return new root
      return y;
    }

    // function to insert a new key into the subtree rooted
    // with node
    AVLNode<T>* insert(AVLNode<T>* node, T key)
    {
      // Perform the normal BST insertion
      if (node == nullptr)
        return new AVLNode<T>(key);

      if (key < node->key)
        node->left = insert(node->left, key);
      else if (key > node->key)
        node->right = insert(node->right, key);
      else
        return node;

      // Update height of this ancestor node
      node->height = 1
        + max(height(node->left),
            height(node->right));

      // Get the balance factor of this ancestor node
      int balance = balanceFactor(node);

      // If this node becomes unbalanced, then there are 4
      // cases

      // Left heavy
      if (balance > 1) {
        if (balanceFactor(node->left) >= 0) {
          return rightRotate(node);          // LL
        } else {
          node->left = leftRotate(node->left); // LR
          return rightRotate(node);
        }
      }

      // Right heavy
      if (balance < -1) {
        if (balanceFactor(node->right) <= 0) {
          return leftRotate(node);           // RR
        } else {
          node->right = rightRotate(node->right); // RL
          return leftRotate(node);
        }
      }

      return node;
    }

    // function to find the node with the minimum key value
    AVLNode<T>* minValueNode(AVLNode<T>* node)
    {
      AVLNode<T>* current = node;
      while (current->left != nullptr)
        current = current->left;
      return current;
    }

    // function to delete a key from the subtree rooted with
    // root
    AVLNode<T>* deleteNode(AVLNode<T>* root, T key)
    {
      // Perform standard BST delete
      if (root == nullptr)
        return root;

      if (key < root->key)
        root->left = deleteNode(root->left, key);
      else if (key > root->key)
        root->right = deleteNode(root->right, key);
      else {
        // Node with only one child or no child
        if ((root->left == nullptr) || (root->right == nullptr)) {
          AVLNode<T>* temp = root->left ? root->left : root->right;

          if (temp == nullptr) {
            delete root;
            return nullptr;
          } else {
            AVLNode<T>* old = root;
            root = temp;
            delete old;
          }
        }

        else {

          AVLNode<T>* temp
            = minValueNode(root->right);
          root->key = temp->key;
          root->right
            = deleteNode(root->right, temp->key);
        }
      }

      if (root == nullptr)
        return root;

      // Update height of the current node
      root->height = 1
        + max(height(root->left),
            height(root->right));

      // Get the balance factor of this node
      int balance = balanceFactor(root);

      // If this node becomes unbalanced, then there are 4
      // cases

      // Left Left Case
      if (balance > 1 && balanceFactor(root->left) >= 0)
        return rightRotate(root);

      // Left Right Case
      if (balance > 1 && balanceFactor(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
      }

      // Right Right Case
      if (balance < -1 && balanceFactor(root->right) <= 0)
        return leftRotate(root);

      // Right Left Case
      if (balance < -1
          && balanceFactor(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
      }

      return root;
    }

    // function to perform inorder traversal of the tree
    void inorder(AVLNode<T>* root)
    {
      if (root != nullptr) {
        inorder(root->left);
        cout << root->key << " ";
        inorder(root->right);
      }
    }

    // function to search for a key in the subtree rooted
    // with root
    bool search(AVLNode<T>* root, T key)
    {
      if (root == nullptr)
        return false;
      if (root->key == key)
        return true;
      if (key < root->key)
        return search(root->left, key);
      return search(root->right, key);
    }

    void saveToFileHelper(AVLNode<T>* node, ofstream& file) {
      if (node != nullptr) {
        saveToFileHelper(node->left, file);
        file << node->key << endl;
        saveToFileHelper(node->right, file);
      }
    }

  public:
    AVLNode<T>* getRoot() {
      return root;
    }
    // Constructor to initialize the AVL tree
    AVLTree()
      : root(nullptr)
    {
    }

    void loadFromFile(string fname) {
      filename = fname;
      ifstream file(filename);

      if (!file.is_open()) {
        cout << "Warning: Scheme file '" << filename << "' not found. A new one will be created." << endl;
        return;
      }

      T key;
      while (file >> key) { 
        root = insert(root, key);
      }
      file.close();
    }

    void saveToFile() {
      if (filename.empty()) return; 

      ofstream file(filename);
      if (!file.is_open()) {
        cerr << "Error: Could not open file '" << filename << "' for writing." << endl;
        return;
      }
      saveToFileHelper(root, file);
      file.close();
    }

    // Function to insert a key into the AVL tree
    void insert(T key) {
      root = insert(root, key);
      saveToFile(); 
    }

    // Function to search for a key in the AVL tree
    void remove(T key) {
      root = deleteNode(root, key);
      saveToFile(); 
    }

    // Function to print the inorder traversal of the AVL
    // tree
    void printInorder()
    {
      inorder(root);
      cout << endl;
    }
    bool search(T key) {
      return search(this->root, key);
    }
};
#endif
