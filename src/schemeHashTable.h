#ifndef SCHEME_HASHTABLE_H
#define SCHEME_HASHTABLE_H

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <fstream>  

using namespace std;

struct Scheme {
  string name;    
  string pattern; 

  Scheme(string n, string p) : name(n), pattern(p) {}
};

class SchemeHashTable {
  private:
    vector<list<Scheme>> table;
    int numBuckets;
    string filename; 

    unsigned long hashFunction(const string& key) {
      unsigned long hash = 5381;
      for (char c : key) {
        hash = ((hash << 5) + hash) + c; 
      }
      return hash % numBuckets;
    }

    // Private helper to save data back to the file
    void saveToFile() {
      ofstream outFile(filename);
      if (!outFile.is_open()) {
        cerr << "Error: Could not open file '" << filename << "' for writing." << endl;
        return;
      }

      for (int i = 0; i < numBuckets; i++) {
        for (const auto& scheme : table[i]) {
          outFile << scheme.name << " " << scheme.pattern << endl;
        }
      }
      outFile.close();
    }

  public:
    SchemeHashTable(int buckets = 101) : numBuckets(buckets) {
      table.resize(numBuckets);
    }

    // Load schemes from a file initially
    void loadFromFile(string fname) {
      filename = fname;
      ifstream inFile(filename);

      // If file doesn't exist, we will create it later when saving
      if (!inFile.is_open()) {
        cout << "Warning: Scheme file '" << filename << "' not found. A new one will be created." << endl;
        return;
      }

      string name, pattern;
      while (inFile >> name >> pattern) {
        // We use a simplified insert here to avoid re-saving during loading
        int index = hashFunction(name);
        table[index].emplace_back(name, pattern);
      }
      inFile.close();
    }

    // Insert or Update a scheme
    void insert(string name, string pattern) {
      int index = hashFunction(name);

      // Check if updating existing scheme
      for (auto& scheme : table[index]) {
        if (scheme.name == name) {
          scheme.pattern = pattern;
          saveToFile(); 
          cout << "Scheme '" << name << "' updated." << endl;
          return;
        }
      }

      // Insert new scheme
      table[index].emplace_back(name, pattern);
      saveToFile(); 
      cout << "Scheme '" << name << "' added." << endl;
    }

    // Remove a scheme
    void remove(string name) {
      int index = hashFunction(name);
      auto& bucket = table[index];

      for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->name == name) {
          bucket.erase(it);
          saveToFile(); 
          cout << "Scheme '" << name << "' removed." << endl;
          return;
        }
      }
      cout << "Error: Scheme '" << name << "' not found." << endl;
    }

    string getPattern(string name) {
      int index = hashFunction(name);
      for (auto& scheme : table[index]) {
        if (scheme.name == name) {
          return scheme.pattern;
        }
      }
      return "";
    }

    void display() {
      cout << "\n--- Current Schemes (" << filename << ") ---" << endl;
      for (int i = 0; i < numBuckets; i++) {
        for (auto& scheme : table[i]) {
          cout << "Name: " << scheme.name << " \t Pattern: " << scheme.pattern << endl;
        }
      }
      cout << "-----------------------------------" << endl;
    }
};

#endif
