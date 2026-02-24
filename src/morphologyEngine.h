#ifndef MORPHOLOGY_ENGINE_H
#define MORPHOLOGY_ENGINE_H

#include <string>
#include <vector>
#include <string_view>
#include <algorithm>

#include "AVLTree.h"
#include "schemeHashTable.h" 
#include "StringUtils.h"

using namespace std;

// Data structure to hold the results of a reverse root search
struct RootSearchResult {
  bool found;
  string root;
  string schemeName;
  string schemePattern;
  bool noSchemesMatchedLength;

  RootSearchResult() : found(false), root(""), schemeName(""), schemePattern(""), noSchemesMatchedLength(false) {}
};

class MorphologyEngine {
  public:
    // HELPER: Expand Shadda into two identical letters (Kept here because it's specific to morphology)
    static vector<string_view> expandShadda(const vector<string_view>& chars) {
      vector<string_view> expanded;
      for (const auto& c : chars) {
        if (c == "\u0651") { 
          if (!expanded.empty()) expanded.push_back(expanded.back()); 
        } else {
          expanded.push_back(c);
        }
      }
      return expanded;
    }

    // GENERATE (Forward): Root + Pattern -> Word
    static string generate(const string& rawRoot, const vector<string>& patChars) {
      string root = StringUtils::sanitize(rawRoot);
      vector<string_view> rootChars = StringUtils::splitUTF8(root);
      if (rootChars.size() != 3) return "Error";

      string result = "";
      string_view lastChar = "";

      for (const string& ch : patChars) {
        string_view current;
        if (ch == "1") current = rootChars[0];
        else if (ch == "2") current = rootChars[1];
        else if (ch == "3") current = rootChars[2];
        else current = ch;

        if (!result.empty() && current == lastChar) {
          result += "\u0651"; // Contract into Shadda
        } else {
          result += current;
        }
        lastChar = current;
      }
      return result;
    }

    // APPLY SCHEME: String Pattern + Root -> Word (Used heavily by the Custom Generator UI)
    static string applyScheme(const string& rawRoot, const string& rawPattern) {
      string root = StringUtils::sanitize(rawRoot);
      vector<string_view> rootChars = StringUtils::splitUTF8(root);

      // Safety check: Ensure exactly 3 root letters
      if (rootChars.size() != 3) return ""; 

      string result = "";
      string_view lastChar = "";
      vector<string_view> patChars = StringUtils::splitUTF8(rawPattern);

      for (string_view ch : patChars) {
        string_view current;
        if (ch == "1") current = rootChars[0];
        else if (ch == "2") current = rootChars[1];
        else if (ch == "3") current = rootChars[2];
        else current = ch;

        if (!result.empty() && current == lastChar) {
          result += "\u0651"; // Contract into Shadda
        } else {
          result += string(current);
        }
        lastChar = current;
      }
      return result;
    }

    // Generate Family & Update Tree
    // Returns number of words generated. Returns -1 if root not found.
    static int generateFamily(const string& rawRoot, AVLTree<string>& tree, const SchemeHashTable& schemes) {
      string root = StringUtils::sanitize(rawRoot);
      if (!tree.search(root)) {
        return -1; 
      }

      vector<Scheme> allSchemes = schemes.getAllSchemes();
      int count = 0;

      for (const auto& scheme : allSchemes) {
        string word = generate(root, scheme.parsedPattern);
        tree.addDerivedWord(root, word);
        count++;
      }
      return count;
    }

    // VALIDATE: Check if Word comes from Root using any known Scheme
    static bool validate(const string& rawWord, const string& rawRoot, const SchemeHashTable& schemes, string& foundSchemeName, AVLTree<string>& tree) {
      string word = StringUtils::sanitize(rawWord);
      string root = StringUtils::sanitize(rawRoot);

      vector<string_view> wChars = expandShadda(StringUtils::splitUTF8(word));
      vector<string_view> rChars = StringUtils::splitUTF8(root);

      if (rChars.size() != 3) return false;

      string deducedPattern = "";
      int rIndex = 0;

      for (const auto& wChar : wChars) {
        if (rIndex < 3 && StringUtils::normalizeAlif(wChar) == StringUtils::normalizeAlif(rChars[rIndex])) {
          deducedPattern += to_string(rIndex + 1);
          rIndex++;
        } else {
          deducedPattern += string(wChar);
        }
      }

      if (rIndex != 3) return false;

      string name = schemes.getNameByPattern(deducedPattern); 
      if (name != "") {
        foundSchemeName = name;
        tree.addDerivedWord(root, word);
        return true;
      }
      return false;
    }

    // REVERSE ENGINEER helpers
    static string extractRootIfMatches(const string& word, const string& pattern) {
      vector<string_view> wChars = expandShadda(StringUtils::splitUTF8(word));
      vector<string_view> pChars = StringUtils::splitUTF8(pattern);

      if (wChars.size() != pChars.size()) return "";

      string_view r1 = "", r2 = "", r3 = "";

      for (size_t i = 0; i < pChars.size(); i++) {
        if (pChars[i] == "1") r1 = wChars[i];
        else if (pChars[i] == "2") r2 = wChars[i];
        else if (pChars[i] == "3") r3 = wChars[i];
        else {
          if (StringUtils::normalizeAlif(pChars[i]) != StringUtils::normalizeAlif(wChars[i])) return ""; 
        }
      }

      if (!r1.empty() && !r2.empty() && !r3.empty()) {
        return string(r1) + string(r2) + string(r3);
      }
      return "";
    }

    // OPTIMIZED FINDER (Root Detection)
    // Returns a RootSearchResult struct for GUI consumption
    static RootSearchResult findRoot(const string& rawWord, const AVLTree<string>& rootsTree, const SchemeHashTable& schemes) {
      RootSearchResult result;
      string word = StringUtils::sanitize(rawWord); 
      vector<string_view> wChars = StringUtils::splitUTF8(word);
      int wordLen = wChars.size();
      int rootLen = 3; 
      int requiredAddedLength = wordLen - rootLen;

      if (requiredAddedLength < 0) {
        return result; // word too short, found remains false
      }

      vector<Scheme> candidates = schemes.getSchemesByAddedLength(requiredAddedLength);

      if (candidates.empty()) {
        result.noSchemesMatchedLength = true;
        return result;
      }

      for (const auto& scheme : candidates) {
        string candidateRoot = extractRootIfMatches(word, scheme.pattern);

        if (candidateRoot != "") {
          if (rootsTree.search(candidateRoot)) {
            result.found = true;
            result.root = candidateRoot;
            result.schemeName = scheme.name;
            result.schemePattern = scheme.pattern;
            return result; 
          }
        }
      }

      return result; // Not found in tree
    }

    // PATTERN AUTO-GENERATOR
    static string derivePatternFromName(const string& rawName) {
      string name = StringUtils::sanitize(rawName);
      vector<string_view> chars = StringUtils::splitUTF8(name);
      string pattern = "";

      bool startsWithAl = (chars.size() >= 2 && StringUtils::normalizeAlif(chars[0]) == "ا" && chars[1] == "ل");

      for (size_t i = 0; i < chars.size(); i++) {
        string_view c = chars[i];

        // If this is the 'ل' from the 'ال' prefix, do NOT replace it with '3'
        if (startsWithAl && i == 1) {
          pattern += string(c);
        } 
        else if (c == "ف") {
          pattern += "1";
        } 
        else if (c == "ع") {
          pattern += "2";
        } 
        else if (c == "ل") {
          pattern += "3";
        } 
        else {
          pattern += string(c);
        }
      }
      return pattern;
    }
};

#endif
