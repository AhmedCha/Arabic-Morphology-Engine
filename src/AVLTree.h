#ifndef AVLTREE_H  
#define AVLTREE_H 

#include <algorithm>
#include <fstream>  
#include <vector>
#include <sstream>
#include <string>
#include <functional> 

#include "StringUtils.h"

using namespace std;

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
  string scheme;
  int frequency;
  DerivedWord(const string& w, const string& s = "", int f = 1) : word(w), scheme(s), frequency(f) {}
  DerivedWord(const string& w, int f) : word(w), scheme(""), frequency(f) {}
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
    mutable int operationCount = 0;
    mutable int rotationCount = 0;
    string logFilePath = "tree_log.txt";

    void logAction(const string& action, const T& key) {
      ofstream logFile(logFilePath, ios::app);
      if (logFile.is_open()) {
        logFile << "Action: " << action << " | Key: " << key 
          << " | Total Ops: " << operationCount 
          << " | Total Rotations: " << rotationCount << "\n";
        logFile.close();
      }
    }

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

    // Pass by reference so parent's pointer is updated instantly!
    void rightRotate(AVLNode<T>*& y)
    {
      addOp(); AVLNode<T>* x = y->left;
      addOp(); AVLNode<T>* T2 = x->right;

      addOp(); y->left = T2;
      addOp(); x->right = y;

      y->height = max(height(y->left), height(y->right)) + 1;
      x->height = max(height(x->left), height(x->right)) + 1;

      y = x; // Instantly update the parent's pointer to the new root
      addRot(); // Safe to animate now!
    }

    // Pass by reference so parent's pointer is updated instantly!
    void leftRotate(AVLNode<T>*& x)
    {
      addOp(); AVLNode<T>* y = x->right;
      addOp(); AVLNode<T>* T2 = y->left;

      addOp(); x->right = T2;
      addOp(); y->left = x;

      x->height = max(height(x->left), height(x->right)) + 1;
      y->height = max(height(y->left), height(y->right)) + 1;

      x = y; // Instantly update the parent's pointer to the new root
      addRot(); // Safe to animate now!
    }

    void insert(AVLNode<T>*& node, const T& key)
    {            
      addOp(); 
      if (node == root) {
        logAction("Insert", key);
      }
      
      if (node == nullptr) {
        addOp();
        node = new AVLNode<T>(key); // Updates parent's pointer instantly
        if (animateCb) animateCb(); // Safe to animate appearance
        return; 
      }

      addOp();
      if (key < node->key) {
        insert(node->left, key);
      } else {
        addOp();
        if (key > node->key) {
          insert(node->right, key);
        } else {
          return; // Duplicate
        }
      }

      node->height = 1 + max(height(node->left), height(node->right));

      int balance = balanceFactor(node);

      if (balance > 1) {
        addOp();
        if (balanceFactor(node->left) >= 0) {
          rightRotate(node);
        } else {
          leftRotate(node->left);
          rightRotate(node);
        }
      } else if (balance < -1) {
        addOp();
        if (balanceFactor(node->right) <= 0) {
          leftRotate(node);
        } else {
          rightRotate(node->right);
          leftRotate(node);
        }
      }
    }

    AVLNode<T>* minValueNode(AVLNode<T>* node) const
    {
      AVLNode<T>* current = node;
      while (current && current->left != nullptr) {
        addOp(); 
        current = current->left;
      }
      return current;
    }

    void deleteNode(AVLNode<T>*& rootNode, const T& key)
    {
      addOp();
      if (rootNode == root) { 
        logAction("Delete", key);
      }
      if (rootNode == nullptr) return;

      addOp();
      if (key < rootNode->key) {
        deleteNode(rootNode->left, key);
      } else {
        addOp();
        if (key > rootNode->key) {
          deleteNode(rootNode->right, key);
        } else {
          addOp();
          if ((rootNode->left == nullptr) || (rootNode->right == nullptr)) {
            AVLNode<T>* temp = rootNode->left ? rootNode->left : rootNode->right;

            if (temp == nullptr) {
              addOp();
              AVLNode<T>* nodeToDelete = rootNode;
              rootNode = nullptr; // Clear parent's pointer FIRST
              if (animateCb) animateCb(); // Safe to animate
              delete nodeToDelete; // Free memory safely
              return;
            } else {
              addOp();
              AVLNode<T>* nodeToDelete = rootNode;
              rootNode = temp; // Update parent's pointer FIRST
              if (animateCb) animateCb(); // Safe to animate
              delete nodeToDelete; // Free memory safely
            }
          }
          else {
            AVLNode<T>* temp = minValueNode(rootNode->right);
            addOp(); rootNode->key = temp->key;
            addOp(); rootNode->derivedWords = temp->derivedWords; 
            if (animateCb) animateCb(); 
            
            deleteNode(rootNode->right, temp->key);
          }
        }
      }

      if (rootNode == nullptr) return;

      rootNode->height = 1 + max(height(rootNode->left), height(rootNode->right));

      int balance = balanceFactor(rootNode);

      addOp();
      if (balance > 1 && balanceFactor(rootNode->left) >= 0) {
        rightRotate(rootNode);
        return;
      }
      addOp();
      if (balance > 1 && balanceFactor(rootNode->left) < 0) {
        leftRotate(rootNode->left);
        rightRotate(rootNode);
        return;
      }
      addOp();
      if (balance < -1 && balanceFactor(rootNode->right) <= 0) {
        leftRotate(rootNode);
        return;
      }
      addOp();
      if (balance < -1 && balanceFactor(rootNode->right) > 0) {
        rightRotate(rootNode->right);
        leftRotate(rootNode);
        return;
      }
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
    std::function<void()> updateStatsCb = nullptr;
    std::function<void()> animateCb = nullptr;

    void addOp() const {
        operationCount++;
        if (updateStatsCb) updateStatsCb(); 
    }

    void addRot() const {
        rotationCount++;
        if (updateStatsCb) updateStatsCb();
        if (animateCb) animateCb(); 
    }

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
      insert(root, key);
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
      deleteNode(root, key);
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

    
    AVLNode<T>* search(AVLNode<T>* node, const T& key) const {
      addOp(); 
      if (node == nullptr || node->key == key) return node;
      
      addOp(); 
      if (key < node->key) return search(node->left, key);
      return search(node->right, key);
    }

    bool search(const T& key) const {
      return (search(this->root, key) != nullptr );
    }

    void addDerivedWord(const T& key, const string& word, const string& scheme = "") {
      AVLNode<T>* node = search(root, key);
      if (node != nullptr) {
        for (auto& dw : node->derivedWords) {
          addOp();
          if (dw.word == word) {
            dw.frequency++;
            if (dw.scheme.empty() && !scheme.empty()) dw.scheme = scheme;
            return;
          }
        }
        addOp();
        node->derivedWords.push_back(DerivedWord(word, scheme, 1));
      }
    }

    

    int getOperationCount() const { return operationCount; }
    int getRotationCount() const { return rotationCount; }

    vector<DerivedWord> getFamily(const T& key) const {
      AVLNode<T>* node = search(root, key);
      if (node != nullptr) {
        return node->derivedWords;
      }
      return vector<DerivedWord>(); 
    }
};
#endif
