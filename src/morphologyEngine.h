#ifndef MORPHOLOGY_ENGINE_H
#define MORPHOLOGY_ENGINE_H

#include <iostream>
#include <string>
#include <vector>
#include "AVLTree.h"
#include "schemeHashTable.h" 

using namespace std;

class MorphologyEngine {
  private:
    // Helper: Splits a UTF-8 string into individual characters
    static vector<string> splitUTF8(const string& str) {
      vector<string> chars;
      for (size_t i = 0; i < str.length();) {
        int len = 1;
        // UTF-8 byte handling
        if ((str[i] & 0xE0) == 0xC0) len = 2;       // 2-byte char (e.g., Arabic)
        else if ((str[i] & 0xF0) == 0xE0) len = 3;  // 3-byte char
        else if ((str[i] & 0xF8) == 0xF0) len = 4;  // 4-byte char

        chars.push_back(str.substr(i, len));
        i += len;
      }
      return chars;
    }

  public:
    // GENERATE (Forward): Root + Pattern -> Word
    static string generate(string root, string pattern) {
      vector<string> rootChars = splitUTF8(root);
      if (rootChars.size() != 3) return "Error: Root must be 3 chars";

      string R1 = rootChars[0];
      string R2 = rootChars[1];
      string R3 = rootChars[2];

      string result = "";
      vector<string> patChars = splitUTF8(pattern);

      for (const string& ch : patChars) {
        if (ch == "1") result += R1;
        else if (ch == "2") result += R2;
        else if (ch == "3") result += R3;
        else result += ch;
      }
      return result;
    }

    // VALIDATE: Check if Word comes from Root using any known Scheme
    // Tries every scheme in the table to see if 'root' + 'scheme' == 'word'
    static bool validate(string word, string root, SchemeHashTable& schemes, string& foundSchemeName) {
      // Get all schemes to test
      vector<Scheme> allSchemes = schemes.getAllSchemes();

      for (const auto& scheme : allSchemes) {
        // Try to generate the word using this specific scheme
        string prediction = generate(root, scheme.pattern);

        if (prediction == word) {
          foundSchemeName = scheme.name;
          return true;
        }
      }
      return false;
    }

    // REVERSE ENGINEER helpers
    static string extractRootIfMatches(string word, string pattern) {
      vector<string> wChars = splitUTF8(word);
      vector<string> pChars = splitUTF8(pattern);

      // Length Check
      if (wChars.size() != pChars.size()) return "";

      string r1 = "", r2 = "", r3 = "";

      // Iterate and Compare
      for (size_t i = 0; i < pChars.size(); i++) {
        if (pChars[i] == "1") r1 = wChars[i];
        else if (pChars[i] == "2") r2 = wChars[i];
        else if (pChars[i] == "3") r3 = wChars[i];
        else {
          // Fixed letter must match exactly
          if (pChars[i] != wChars[i]) return ""; 
        }
      }

      // Return combined root if all placeholders were found
      if (r1 != "" && r2 != "" && r3 != "") return r1 + r2 + r3;
      return "";
    }

    // OPTIMIZED FINDER (Root Detection)
    static void findRoot(string word, AVLTree<string>& rootsTree, SchemeHashTable& schemesTable) {
      vector<Scheme> schemes = schemesTable.getAllSchemes();

      cout << "Analyzing '" << word << "' against " << schemes.size() << " schemes..." << endl;

      bool matchFound = false;

      for (const auto& scheme : schemes) {
        string candidateRoot = extractRootIfMatches(word, scheme.pattern);

        if (candidateRoot != "") {
          if (rootsTree.search(candidateRoot)) {
            cout << "\n------------------------------------------------" << endl;
            cout << "✔ MATCH FOUND!" << endl;
            cout << "Word:   " << word << endl;
            cout << "Root:   " << candidateRoot  << endl;
            cout << "Scheme: " << scheme.name << " (" << scheme.pattern << ")" << endl;
            cout << "------------------------------------------------\n" << endl;
            matchFound = true;
            return; 
          }
        }
      }

      if (!matchFound) {
        cout << "\n------------------------------------------------" << endl;
        cout << "✘ No valid root found in the database." << endl;
        cout << "------------------------------------------------\n" << endl;
      }
    }

    // PATTERN AUTO-GENERATOR
    // Converts standard Arabic names (using ف, ع, ل) into patterns (1, 2, 3)
    static string derivePatternFromName(string name) {
      vector<string> chars = splitUTF8(name);
      string pattern = "";

      for (const string& c : chars) {
        if (c == "ف") {
          pattern += "1";
        } else if (c == "ع") {
          pattern += "2";
        } else if (c == "ل") {
          pattern += "3";
        } else {
          pattern += c;
        }
      }
      return pattern;
    }
};

#endif
