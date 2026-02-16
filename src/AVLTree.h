#ifndef AVLTREE_H  
#define AVLTREE_H 

#include <algorithm>
#include <fstream>  
#include <vector>
#include <sstream>
#include <string>
#include <memory>

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
    unique_ptr<AVLNode<T>> left; 
    unique_ptr<AVLNode<T>> right; 
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
    unique_ptr<AVLNode<T>> root;
    string filename;

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
      return height(node->left.get()) - height(node->right.get());
    }

    unique_ptr<AVLNode<T>> rightRotate(unique_ptr<AVLNode<T>> y)
    {
      unique_ptr<AVLNode<T>> x = std::move(y->left);
      unique_ptr<AVLNode<T>> T2 = std::move(x->right);

      y->left = std::move(T2);
      x->right = std::move(y);

      x->right->height
        = max(height(x->right->left.get()), height(x->right->right.get())) + 1;
      x->height
        = max(height(x->left.get()), height(x->right.get())) + 1;

      return x;
    }

    unique_ptr<AVLNode<T>> leftRotate(unique_ptr<AVLNode<T>> x)
    {
      unique_ptr<AVLNode<T>> y = std::move(x->right);
      unique_ptr<AVLNode<T>> T2 = std::move(y->left);

      x->right = std::move(T2);
      y->left = std::move(x);

      y->left->height
        = max(height(y->left->left.get()), height(y->left->right.get())) + 1;
      y->height
        = max(height(y->left.get()), height(y->right.get())) + 1;

      return y;
    }

    unique_ptr<AVLNode<T>> insert(unique_ptr<AVLNode<T>> node, const T& key)
    {
      if (node == nullptr)
        return make_unique<AVLNode<T>>(key);

      if (key < node->key)
        node->left = insert(std::move(node->left), key);
      else if (key > node->key)
        node->right = insert(std::move(node->right), key);
      else
        return node;

      node->height = 1
        + max(height(node->left.get()),
            height(node->right.get()));

      int balance = balanceFactor(node.get());

      // Left heavy
      if (balance > 1) {
        if (balanceFactor(node->left.get()) >= 0) {
          return rightRotate(std::move(node));
        } else {
          node->left = leftRotate(std::move(node->left));
          return rightRotate(std::move(node));
        }
      }

      // Right heavy
      if (balance < -1) {
        if (balanceFactor(node->right.get()) <= 0) {
          return leftRotate(std::move(node));
        } else {
          node->right = rightRotate(std::move(node->right));
          return leftRotate(std::move(node));
        }
      }

      return node;
    }

    AVLNode<T>* minValueNode(AVLNode<T>* node) const
    {
      AVLNode<T>* current = node;
      while (current && current->left != nullptr)
        current = current->left.get();
      return current;
    }

    unique_ptr<AVLNode<T>> deleteNode(unique_ptr<AVLNode<T>> root, const T& key)
    {
      if (root == nullptr)
        return root;

      if (key < root->key)
        root->left = deleteNode(std::move(root->left), key);
      else if (key > root->key)
        root->right = deleteNode(std::move(root->right), key);
      else {
        if ((root->left == nullptr) || (root->right == nullptr)) {
          unique_ptr<AVLNode<T>> temp = std::move(root->left ? root->left : root->right);

          if (temp == nullptr) {
            return nullptr;
          } else {
            root = std::move(temp);
          }
        }
        else {
          AVLNode<T>* temp = minValueNode(root->right.get());
          root->key = temp->key;
          root->derivedWords = temp->derivedWords; 
          root->right = deleteNode(std::move(root->right), temp->key);
        }
      }

      if (root == nullptr)
        return root;

      root->height = 1
        + max(height(root->left.get()),
            height(root->right.get()));

      int balance = balanceFactor(root.get());

      if (balance > 1 && balanceFactor(root->left.get()) >= 0)
        return rightRotate(std::move(root));

      if (balance > 1 && balanceFactor(root->left.get()) < 0) {
        root->left = leftRotate(std::move(root->left));
        return rightRotate(std::move(root));
      }

      if (balance < -1 && balanceFactor(root->right.get()) <= 0)
        return leftRotate(std::move(root));

      if (balance < -1
          && balanceFactor(root->right.get()) > 0) {
        root->right = rightRotate(std::move(root->right));
        return leftRotate(std::move(root));
      }

      return root;
    }

    AVLNode<T>* search(AVLNode<T>* node, const T& key) const {
      if (node == nullptr || node->key == key) return node;
      if (key < node->key) return search(node->left.get(), key);
      return search(node->right.get(), key);
    }

    void saveToFileHelper(AVLNode<T>* node, ofstream& file) const {
      if (node != nullptr) {
        saveToFileHelper(node->left.get(), file);

        file << node->key;

        for(const auto& dw : node->derivedWords) {
          file << " " << dw.word << " " << dw.frequency;
        }
        file << endl;

        saveToFileHelper(node->right.get(), file);
      }
    }

    void exportCSVHelper(AVLNode<T>* node, ofstream& file) const {
      if (node) {
        exportCSVHelper(node->left.get(), file);
        for(const auto& dw : node->derivedWords) {
          // Wrap strings in double quotes to safely handle any rogue commas
          file << "\"" << node->key << "\",\"" << dw.word << "\"," << dw.frequency << "\n"; 
        }
        exportCSVHelper(node->right.get(), file);
      }
    }

  public:
    AVLNode<T>* getRoot() const {
      return root.get();
    }

    AVLTree() : root(nullptr) {}

    ~AVLTree() {
      root.reset();
    }

    // Returns true if successful, false otherwise
    bool saveToFile(const string& fname = "") {
      if (fname != "") filename = fname;
      if (filename.empty()) return false; 

      ofstream file(filename);
      if (!file.is_open()) {
        return false;
      }
      saveToFileHelper(root.get(), file);
      file.close();
      return true;
    }

    // Returns true if successful, false otherwise
    bool exportToCSV(const string& csvFilename) const {
      ofstream file(csvFilename);
      if (file.is_open()) {
        file << "Root,DerivedWord,Frequency\n"; 
        exportCSVHelper(root.get(), file);
        return true;
      }
      return false;
    }

    // Returns true if successfully loaded, false if file missing or error
    bool loadFromFile(const string& fname) {
      filename = fname;
      ifstream file(filename);

      if (!file.is_open()) {
        return false; 
      }

      root.reset(); 

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

        AVLNode<T>* node = search(root.get(), key);
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
      collectAll(node->left.get(), result);
      result.push_back(node->key); 
      collectAll(node->right.get(), result);
    }

    vector<T> getAllElements() const {
      vector<T> res;
      collectAll(root.get(), res);
      return res;
    }

    void insert(const T& key) {
      root = insert(std::move(root), key);
    }

    void remove(const T& key) {
      root = deleteNode(std::move(root), key);
    }

    bool search(const T& key) const {
      return (search(this->root.get(), key) != nullptr );
    }

    void addDerivedWord(const T& key, const string& word) {
      AVLNode<T>* node = search(root.get(), key);
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

    // Replaced showFamily with getFamily for GUI compatibility
    vector<DerivedWord> getFamily(const T& key) const {
      AVLNode<T>* node = search(root.get(), key);
      if (node != nullptr) {
        return node->derivedWords;
      }
      return vector<DerivedWord>(); // Return empty vector if root not found
    }
};
#endif
