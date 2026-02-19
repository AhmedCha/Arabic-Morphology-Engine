#ifndef AVLTREE_H  
#define AVLTREE_H 

#include <algorithm>
#include <fstream>  
#include <vector>
#include <sstream>
#include <string>

#include "StringUtils.h"

using namespace std;

struct DerivedWord {
  string word;
  int frequency;
  DerivedWord(const string& w, int f = 1) : word(w), frequency(f) {}
};

template <typename T> class AVLNode {
  public:
    T key; 
    vector<DerivedWord> derivedWords;
    AVLNode<T>* left; 
    AVLNode<T>* right; 
    int height; 

    AVLNode(const T& k)
      : key(k)
        , left(nullptr)
        , right(nullptr)
        , height(1)
  {
  }
};

template <typename T> class AVLTree {
  private:
    AVLNode<T>* root;
    string filename;

    void destroyTree(AVLNode<T>* node) {
      if (node != nullptr) {
        destroyTree(node->left);
        destroyTree(node->right);
        delete node;
      }
    }

    int height(AVLNode<T>* node) const
    {
      if (node == nullptr)
        return 0;
      return node->height;
    }

    int balanceFactor(AVLNode<T>* node) const
    {
      if (node == nullptr)
        return 0;
      return height(node->left) - height(node->right);
    }

    AVLNode<T>* rightRotate(AVLNode<T>* y)
    {
      AVLNode<T>* x = y->left;
      AVLNode<T>* T2 = x->right;

      y->left = T2;
      x->right = y;

      y->height = max(height(y->left), height(y->right)) + 1;
      x->height = max(height(x->left), height(x->right)) + 1;

      return x;
    }

    AVLNode<T>* leftRotate(AVLNode<T>* x)
    {
      AVLNode<T>* y = x->right;
      AVLNode<T>* T2 = y->left;

      x->right = T2;
      y->left = x;

      x->height = max(height(x->left), height(x->right)) + 1;
      y->height = max(height(y->left), height(y->right)) + 1;

      return y;
    }

    AVLNode<T>* insert(AVLNode<T>* node, const T& key)
    {
      if (node == nullptr)
        return new AVLNode<T>(key); // Manual allocation

      if (key < node->key)
        node->left = insert(node->left, key);
      else if (key > node->key)
        node->right = insert(node->right, key);
      else
        return node;

      node->height = 1 + max(height(node->left), height(node->right));

      int balance = balanceFactor(node);

      // Left heavy
      if (balance > 1) {
        if (balanceFactor(node->left) >= 0) {
          return rightRotate(node);
        } else {
          node->left = leftRotate(node->left);
          return rightRotate(node);
        }
      }

      // Right heavy
      if (balance < -1) {
        if (balanceFactor(node->right) <= 0) {
          return leftRotate(node);
        } else {
          node->right = rightRotate(node->right);
          return leftRotate(node);
        }
      }

      return node;
    }

    AVLNode<T>* minValueNode(AVLNode<T>* node) const
    {
      AVLNode<T>* current = node;
      while (current && current->left != nullptr)
        current = current->left;
      return current;
    }

    AVLNode<T>* deleteNode(AVLNode<T>* rootNode, const T& key)
    {
      if (rootNode == nullptr)
        return rootNode;

      if (key < rootNode->key)
        rootNode->left = deleteNode(rootNode->left, key);
      else if (key > rootNode->key)
        rootNode->right = deleteNode(rootNode->right, key);
      else {
        if ((rootNode->left == nullptr) || (rootNode->right == nullptr)) {
          AVLNode<T>* temp = rootNode->left ? rootNode->left : rootNode->right;

          if (temp == nullptr) {
            delete rootNode; // Free memory!
            return nullptr;
          } else {
            delete rootNode; // Free the old root before replacing
            rootNode = temp;
          }
        }
        else {
          AVLNode<T>* temp = minValueNode(rootNode->right);
          rootNode->key = temp->key;
          rootNode->derivedWords = temp->derivedWords; 
          rootNode->right = deleteNode(rootNode->right, temp->key);
        }
      }

      if (rootNode == nullptr)
        return rootNode;

      rootNode->height = 1 + max(height(rootNode->left), height(rootNode->right));

      int balance = balanceFactor(rootNode);

      if (balance > 1 && balanceFactor(rootNode->left) >= 0)
        return rightRotate(rootNode);

      if (balance > 1 && balanceFactor(rootNode->left) < 0) {
        rootNode->left = leftRotate(rootNode->left);
        return rightRotate(rootNode);
      }

      if (balance < -1 && balanceFactor(rootNode->right) <= 0)
        return leftRotate(rootNode);

      if (balance < -1 && balanceFactor(rootNode->right) > 0) {
        rootNode->right = rightRotate(rootNode->right);
        return leftRotate(rootNode);
      }

      return rootNode;
    }

    AVLNode<T>* search(AVLNode<T>* node, const T& key) const {
      if (node == nullptr || node->key == key) return node;
      if (key < node->key) return search(node->left, key);
      return search(node->right, key);
    }

    void saveToFileHelper(AVLNode<T>* node, ofstream& file) const {
      if (node != nullptr) {
        saveToFileHelper(node->left, file);

        file << node->key;

        for(const auto& dw : node->derivedWords) {
          file << " " << dw.word << " " << dw.frequency;
        }
        file << endl;

        saveToFileHelper(node->right, file);
      }
    }

    void exportCSVHelper(AVLNode<T>* node, ofstream& file) const {
      if (node) {
        exportCSVHelper(node->left, file);
        for(const auto& dw : node->derivedWords) {
          file << "\"" << node->key << "\",\"" << dw.word << "\"," << dw.frequency << "\n"; 
        }
        exportCSVHelper(node->right, file);
      }
    }

  public:
    AVLNode<T>* getRoot() const {
      return root;
    }

    AVLTree() : root(nullptr) {}

    ~AVLTree() {
      destroyTree(root);
      root = nullptr;
    }

    bool saveToFile(const string& fname = "") {
      if (fname != "") filename = fname;
      if (filename.empty()) return false; 

      ofstream file(filename);
      if (!file.is_open()) {
        return false;
      }
      saveToFileHelper(root, file);
      file.close();
      return true;
    }

    bool exportToCSV(const string& csvFilename) const {
      ofstream file(csvFilename);
      if (file.is_open()) {
        file << "Root,DerivedWord,Frequency\n"; 
        exportCSVHelper(root, file);
        return true;
      }
      return false;
    }

    bool loadFromFile(const string& fname) {
      filename = fname;
      ifstream file(filename);

      if (!file.is_open()) {
        return false; 
      }

      destroyTree(root);
      root = nullptr; 

      string line;
      while (getline(file, line)) {
        if(line.empty()) continue;

        stringstream ss(line);
        T key;
        ss >> key;

        key = StringUtils::sanitize(key);
        if (key.empty()) {
          continue;
        }

        insert(key);

        AVLNode<T>* node = search(root, key);
        if (node == nullptr) {
          continue; 
        }

        string word;
        int freq;
        while(ss >> word >> freq) {
          word = StringUtils::sanitize(word);
          node->derivedWords.push_back(DerivedWord(word, freq));
        }
      }
      file.close();
      return true;
    }

    void collectAll(AVLNode<T>* node, vector<T>& result) const {
      if (node == nullptr) return;
      collectAll(node->left, result);
      result.push_back(node->key); 
      collectAll(node->right, result);
    }

    vector<T> getAllElements() const {
      vector<T> res;
      collectAll(root, res);
      return res;
    }

    void insert(const T& key) {
      root = insert(root, key);
    }

    void remove(const T& key) {
      root = deleteNode(root, key);
    }

    bool search(const T& key) const {
      return (search(this->root, key) != nullptr );
    }

    void addDerivedWord(const T& key, const string& word) {
      AVLNode<T>* node = search(root, key);
      if (node != nullptr) {
        for (auto& dw : node->derivedWords) {
          if (dw.word == word) {
            dw.frequency++;
            return;
          }
        }
        node->derivedWords.push_back(DerivedWord(word, 1));
      }
    }

    vector<DerivedWord> getFamily(const T& key) const {
      AVLNode<T>* node = search(root, key);
      if (node != nullptr) {
        return node->derivedWords;
      }
      return vector<DerivedWord>(); 
    }
};
#endif
