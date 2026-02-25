#ifndef AVLTREE_H  
#define AVLTREE_H 

#include <algorithm>
#include <fstream>  
#include <vector>
#include <sstream>
#include <string>

#include "StringUtils.h"

using namespace std;

// ─── Real-time Visualizer Event Logger ──────────────────────────────────────
// Define AVL_VISUALIZER at compile time to enable: g++ -DAVL_VISUALIZER ...
#ifdef AVL_VISUALIZER
  #include <mutex>
  #include <chrono>

  static const char* AVL_EVENTS_FILE = "/tmp/avl_events.jsonl";
  static mutex       avl_log_mutex;

  // Helper: convert any key type to string for JSON
  template<typename U>
  static string to_string_avl(const U& val) {
    ostringstream oss;
    oss << val;
    return oss.str();
  }

  // Escape a string for JSON
  static string avl_json_escape(const string& s) {
    string out;
    for (unsigned char c : s) {
      if (c == '"')       out += "\\\"";
      else if (c == '\\') out += "\\\\";
      else                out += (char)c;
    }
    return out;
  }

  // Forward-declare so we can use it in AVLTree methods
  #define AVL_LOG(json_str) do { \
    lock_guard<mutex> _lg(avl_log_mutex); \
    ofstream _f(AVL_EVENTS_FILE, ios::app); \
    if (_f) { _f << (json_str) << "\n"; } \
  } while(0)

  // Serialize a node subtree to compact JSON string
  template<typename NodeT>
  static string avl_node_json(NodeT* node) {
    if (!node) return "null";
    ostringstream oss;
    oss << "{\"key\":\"" << avl_json_escape(to_string_avl(node->key)) << "\","
        << "\"height\":" << node->height << ","
        << "\"left\":"  << avl_node_json(node->left)  << ","
        << "\"right\":" << avl_node_json(node->right) << "}";
    return oss.str();
  }

#else
  #define AVL_LOG(json_str) /* no-op */
#endif

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
#ifdef AVL_VISUALIZER
      if (search(key)) {
        ostringstream _oss;
        _oss << "{\"type\":\"already_exists\",\"key\":\"" 
             << avl_json_escape(to_string_avl(key)) << "\"}";
        AVL_LOG(_oss.str());
        return;
      }
#endif
      root = insert(root, key);
      #ifdef AVL_VISUALIZER
      {
        ostringstream _oss;
        _oss << "{\"type\":\"snapshot\",\"op\":\"insert\",\"key\":\"" 
             << avl_json_escape(to_string_avl(key)) << "\","
             << "\"tree\":" << avl_node_json(root) << "}";
        AVL_LOG(_oss.str());
      }
      #endif
    }

    void remove(const T& key) {
      root = deleteNode(root, key);
      #ifdef AVL_VISUALIZER
      {
        ostringstream _oss;
        _oss << "{\"type\":\"snapshot\",\"op\":\"delete\",\"key\":\"" 
             << avl_json_escape(to_string_avl(key)) << "\","
             << "\"tree\":" << avl_node_json(root) << "}";
        AVL_LOG(_oss.str());
      }
      #endif
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
