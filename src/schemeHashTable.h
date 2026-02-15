#ifndef SCHEME_HASHTABLE_H
#define SCHEME_HASHTABLE_H

#include <string>
#include <vector>
#include <list>
#include <fstream>

using namespace std;

struct Scheme {
  string name;
  string pattern;
  vector<string> parsedPattern;

  Scheme() : name(""), pattern("") {}

  // Constructor to auto-parse pattern
  Scheme(string n, string p) : name(n), pattern(p) {
    parsePattern();
  }

  void parsePattern() {
    parsedPattern.clear();
    for (size_t i = 0; i < pattern.length();) {
      unsigned char c = (unsigned char)pattern[i];
      int len = 1;

      if ((c & 0xE0) == 0xC0) len = 2;
      else if ((c & 0xF0) == 0xE0) len = 3;
      else if ((c & 0xF8) == 0xF0) len = 4;

      if (i + len > pattern.length()) len = pattern.length() - i;

      parsedPattern.push_back(pattern.substr(i, len));
      i += len;
    }
  }
};

class SchemeHashTable {
  private:
    static const int TABLE_SIZE = 101;
    list<Scheme> table[TABLE_SIZE];

    int hashFunction(const string& key) const {
      int hash = 0;
      for (char c : key) {
        hash = (hash * 31 + c) % TABLE_SIZE;
      }
      if (hash < 0) hash += TABLE_SIZE;
      return hash;
    }

  public:
    // Combined Logic: Handles New & Edit
    void insert(const string& name, const string& pattern) {
      int index = hashFunction(name);

      for (auto& scheme : table[index]) {
        if (scheme.name == name) {
          scheme.pattern = pattern;
          scheme.parsePattern();
          return; 
        }
      }

      // If not found, add new
      table[index].emplace_back(name, pattern);
    }

    // Remove a scheme. Returns true if removed, false if not found.
    bool remove(const string& name) {
      int index = hashFunction(name);
      auto& bucket = table[index];
      for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->name == name) {
          bucket.erase(it);
          return true;
        }
      }
      return false;
    }

    string getPattern(const string& name) const {
      int index = hashFunction(name);
      for (const auto& scheme : table[index]) {
        if (scheme.name == name) {
          return scheme.pattern;
        }
      }
      return "";
    }

    vector<Scheme> getSchemesByAddedLength(int extraLengthNeeded) const {
      vector<Scheme> matchingSchemes;

      // Target size of the pattern (e.g. if we need +3 letters, pattern must be 6 chars long)
      int targetPatternSize = extraLengthNeeded + 3; 

      for (int i = 0; i < TABLE_SIZE; i++) {
        for (const auto& scheme : table[i]) {
          if ((int)scheme.parsedPattern.size() == targetPatternSize) {
            matchingSchemes.push_back(scheme);
          }
        }
      }
      return matchingSchemes;
    }

    string getNameByPattern(const string& patternToFind) const {
      for (int i = 0; i < TABLE_SIZE; i++) {
        for (const auto& scheme : table[i]) {

          string schemeSignature = "";
          for(const string& p : scheme.parsedPattern) {
            schemeSignature += p;
          }

          if (schemeSignature == patternToFind) {
            return scheme.name;
          }
        }
      }
      return "";
    }

    vector<Scheme> getAllSchemes() const {
      vector<Scheme> allSchemes;
      for (int i = 0; i < TABLE_SIZE; i++) {
        for (const auto& scheme : table[i]) {
          allSchemes.push_back(scheme);
        }
      }
      return allSchemes;
    }

    // Returns true if successful, false otherwise
    bool saveToFile(const string& filename) const {
      ofstream outFile(filename);
      if (!outFile) {
        return false;
      }
      for (int i = 0; i < TABLE_SIZE; i++) {
        for (const auto& scheme : table[i]) {
          outFile << scheme.name << " " << scheme.pattern << endl;
        }
      }
      outFile.close();
      return true;
    }

    // Returns true if successful, false otherwise
    bool loadFromFile(const string& filename) {
      ifstream inFile(filename);
      if (!inFile) return false;

      // Clear table before loading
      for(int i=0; i<TABLE_SIZE; i++) table[i].clear();

      string name, pattern;
      while (inFile >> name >> pattern) {
        insert(name, pattern);
      }
      inFile.close();
      return true;
    }
};

#endif
