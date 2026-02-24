#ifndef SCHEME_HASHTABLE_H
#define SCHEME_HASHTABLE_H

#include <string>
#include <vector>
#include <list>
#include <fstream>

#include "StringUtils.h"
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
    vector<string_view> chars = StringUtils::splitUTF8(pattern);
    for (const auto& c : chars) {
      parsedPattern.push_back(string(c));
    }
  }
};

class SchemeHashTable {
  private:
    static const int TABLE_SIZE = 101;
    static const int MAX_LENGTH = 30;

    list<Scheme*> nameTable[TABLE_SIZE];    
    list<Scheme*> patternTable[TABLE_SIZE]; 
    list<Scheme*> lengthTable[MAX_LENGTH];

    int hashFunction(const string& key) const {
      int hash = 0;
      for (unsigned char c : key) {
        hash = (hash * 31 + c) % TABLE_SIZE;
      }
      if (hash < 0) hash += TABLE_SIZE;
      return hash;
    }

  public:
    // Destructor to manually clean up memory and prevent leaks
    ~SchemeHashTable() {
      for (int i = 0; i < TABLE_SIZE; i++) {
        for (Scheme* ptr : nameTable[i]) {
          delete ptr; // Free the dynamically allocated memory
        }
      }
    }

    void insert(const string& name, const string& pattern) {
      int nameIndex = hashFunction(name);

      for (Scheme* schemePtr : nameTable[nameIndex]) {
        if (schemePtr->name == name) {
          return; // Already exists, ignore
        }
      }

      Scheme* newSchemePtr = new Scheme(name, pattern);

      int patternIndex = hashFunction(pattern);
      int lengthIndex = newSchemePtr->parsedPattern.size();

      nameTable[nameIndex].push_back(newSchemePtr);
      patternTable[patternIndex].push_back(newSchemePtr);

      if (lengthIndex < MAX_LENGTH) {
        lengthTable[lengthIndex].push_back(newSchemePtr);
      }
    }

    bool remove(const string& name) {
      int nameIndex = hashFunction(name);
      auto& bucket = nameTable[nameIndex];

      for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if ((*it)->name == name) {
          Scheme* targetScheme = *it;
          string targetPattern = targetScheme->pattern;
          int patternIndex = hashFunction(targetPattern);
          int lengthIndex = targetScheme->parsedPattern.size();

          // Remove from patternTable
          auto& patBucket = patternTable[patternIndex];
          for (auto patIt = patBucket.begin(); patIt != patBucket.end(); ++patIt) {
            if ((*patIt)->name == name) {
              patBucket.erase(patIt);
              break;
            }
          }

          // Remove from lengthTable
          if (lengthIndex < MAX_LENGTH) {
            auto& lenBucket = lengthTable[lengthIndex];
            for (auto lenIt = lenBucket.begin(); lenIt != lenBucket.end(); ++lenIt) {
              if ((*lenIt)->name == name) {
                lenBucket.erase(lenIt);
                break;
              }
            }
          }

          // Remove from primary nameTable
          bucket.erase(it);

          delete targetScheme; 

          return true;
        }
      }
      return false;
    }

    string getPattern(const string& name) const {
      int index = hashFunction(name);
      for (Scheme* schemePtr : nameTable[index]) {
        if (schemePtr->name == name) {
          return schemePtr->pattern;
        }
      }
      return "";
    }

    vector<Scheme> getSchemesByAddedLength(int extraLengthNeeded) const {
      vector<Scheme> result;
      int targetLength = extraLengthNeeded + 3; // base root is 3 chars

      if (targetLength >= 0 && targetLength < MAX_LENGTH) {
        for (Scheme* schemePtr : lengthTable[targetLength]) {
          result.push_back(*schemePtr);
        }
      }
      return result;
    }

    string getNameByPattern(const string& patternToFind) const {
      int index = hashFunction(patternToFind);
      for (Scheme* schemePtr : patternTable[index]) {
        string schemeSignature = "";
        for(const string& p : schemePtr->parsedPattern) {
          schemeSignature += p;
        }
        if (schemeSignature == patternToFind) return schemePtr->name;
      }
      return "";
    }

    vector<Scheme> getAllSchemes() const {
      vector<Scheme> allSchemes;
      for (int i = 0; i < TABLE_SIZE; i++) {
        for (Scheme* schemePtr : nameTable[i]) {
          allSchemes.push_back(*schemePtr);
        }
      }
      return allSchemes;
    }

    bool saveToFile(const string& filename) const {
      ofstream outFile(filename);
      if (!outFile) {
        return false;
      }
      for (int i = 0; i < TABLE_SIZE; i++) {
        for (Scheme* schemePtr : nameTable[i]) {
          outFile << schemePtr->name << " " << schemePtr->pattern << endl;
        }
      }
      outFile.close();
      return true;
    }

    bool loadFromFile(const string& filename) {
      ifstream inFile(filename);
      if (!inFile) return false;

      for(int i = 0; i < TABLE_SIZE; i++) {
        for(Scheme* ptr : nameTable[i]) {
          delete ptr;
        }
        nameTable[i].clear();
        patternTable[i].clear();
      }
      for(int i = 0; i < MAX_LENGTH; i++) {
        lengthTable[i].clear();
      }

      string name, pattern;
      while (inFile >> name >> pattern) {
        name = StringUtils::sanitize(name);
        pattern = StringUtils::sanitize(pattern);

        if (!name.empty() && !pattern.empty()) {
          insert(name, pattern);
        }
      }
      inFile.close();
      return true;
    }
};

#endif
