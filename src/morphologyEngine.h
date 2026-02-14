#ifndef MORPHOLOGY_ENGINE_H
#define MORPHOLOGY_ENGINE_H

#include <iostream>
#include <string>
#include <vector>

#include "AVLTree.h"
#include "schemeHashTable.h" 
#include "language.h" 

using namespace std;

class MorphologyEngine {
  public:
    // Splits a UTF-8 string into individual characters
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

    // HELPER: Normalize Alif variations to bare Alif
    static string normalizeAlif(const string& c) {
      if (c == "\u0623" || c == "\u0625" || c == "\u0622") return "\u0627"; 
      return c;
    }

    // HELPER: Expand Shadda into two identical letters
    static vector<string> expandShadda(const vector<string>& chars) {
      vector<string> expanded;
      for (const string& c : chars) {
        if (c == "\u0651") { 
          if (!expanded.empty()) expanded.push_back(expanded.back()); 
        } else {
          expanded.push_back(c);
        }
      }
      return expanded;
    }

    // GENERATE (Forward): Root + Pattern -> Word
    static string generate(string root, const vector<string>& patChars) {
      vector<string> rootChars = splitUTF8(root);
      if (rootChars.size() != 3) return "Error";

      vector<string> resChars;
      for (const string& ch : patChars) {
        if (ch == "1") resChars.push_back(rootChars[0]);
        else if (ch == "2") resChars.push_back(rootChars[1]);
        else if (ch == "3") resChars.push_back(rootChars[2]);
        else resChars.push_back(ch);
      }

      string result = "";
      for (size_t i = 0; i < resChars.size(); i++) {
        if (i > 0 && resChars[i] == resChars[i-1]) {
          result += "\u0651"; // Contract into Shadda
        } else {
          result += resChars[i];
        }
      }
      return result;
    }

    // Generate Family & Update Tree
    static void generateFamily(string root, AVLTree<string>& tree, SchemeHashTable& schemes) {
      if (!tree.search(root)) {
        cout << Tr("✘ Root '", "✘ Racine '", "✘ الجذر '") << root << Tr("' not found in Tree.", "' introuvable dans l'arbre.", "' غير موجود في الشجرة.") << endl;
        return;
      }

      cout << "\n  +================================================+" << endl;
      cout << "  |  >> " << Tr("GENERATING FAMILY FOR: ", "GÉNÉRATION DE LA FAMILLE : ", "توليد العائلة للجذر: ") << root << endl;
      cout << "  +================================================+\n" << endl;
      
      vector<Scheme> allSchemes = schemes.getAllSchemes();

      for (const auto& scheme : allSchemes) {
        string word = generate(root, scheme.parsedPattern);
        cout << "    -> " << scheme.name << ": " << word << endl;
        tree.addDerivedWord(root, word);
      }
      
      cout << "\n  " << Tr("✔ Family generated and saved to tree node.", "✔ Famille générée et sauvegardée.", "✔ تم توليد العائلة وحفظها بنجاح.") << endl;
      cout << "  +================================================+\n" << endl;
    }

    // VALIDATE: Check if Word comes from Root using any known Scheme
    static bool validate(string word, string root, SchemeHashTable& schemes, string& foundSchemeName, AVLTree<string>& tree) {
      vector<string> wChars = expandShadda(splitUTF8(word));
      vector<string> rChars = splitUTF8(root);

      if (rChars.size() != 3) return false;

      string deducedPattern = "";
      int rIndex = 0;

      for (const string& wChar : wChars) {
        if (rIndex < 3 && normalizeAlif(wChar) == normalizeAlif(rChars[rIndex])) {
          deducedPattern += to_string(rIndex + 1);
          rIndex++;
        } else {
          deducedPattern += wChar;
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
    static string extractRootIfMatches(string word, string pattern) {
      vector<string> wChars = expandShadda(splitUTF8(word));
      vector<string> pChars = splitUTF8(pattern);

      if (wChars.size() != pChars.size()) return "";

      string r1 = "", r2 = "", r3 = "";

      for (size_t i = 0; i < pChars.size(); i++) {
        if (pChars[i] == "1") r1 = wChars[i];
        else if (pChars[i] == "2") r2 = wChars[i];
        else if (pChars[i] == "3") r3 = wChars[i];
        else {
          if (normalizeAlif(pChars[i]) != normalizeAlif(wChars[i])) return ""; 
        }
      }

      if (r1 != "" && r2 != "" && r3 != "") return r1 + r2 + r3;
      return "";
    }

    // OPTIMIZED FINDER (Root Detection)
    static void findRoot(string word, AVLTree<string>& rootsTree, SchemeHashTable& schemes) {
      vector<string> wChars = splitUTF8(word);
      int wordLen = wChars.size();
      int rootLen = 3; 
      int requiredAddedLength = wordLen - rootLen;
      
      if (requiredAddedLength < 0) {
        cout << Tr("Word is too short to contain a triliteral root.", "Le mot est trop court pour contenir une racine trilitère.", "الكلمة قصيرة جداً لاحتواء جذر ثلاثي.") << endl;
        return;
      }

      vector<Scheme> candidates = schemes.getSchemesByAddedLength(requiredAddedLength);
      bool matchFound = false;

      cout << Tr("Analyzing word: ", "Analyse du mot : ", "جاري تحليل الكلمة: ") << word << "..." << endl;

      for (const auto& scheme : candidates) {
        string candidateRoot = extractRootIfMatches(word, scheme.pattern);

        if (candidateRoot != "") {
          if (rootsTree.search(candidateRoot)) {
            cout << "\n  +================================================+" << endl;
            cout << "  |  >> " << Tr("MATCH FOUND!", "CORRESPONDANCE TROUVÉE !", "تم العثور على تطابق!") << endl;
            cout << "  +================================================+" << endl;
            cout << "    " << Tr("Word:   ", "Mot :   ", "الكلمة: ") << word << endl;
            cout << "    " << Tr("Root:   ", "Racine: ", "الجذر : ") << candidateRoot  << endl;
            cout << "    " << Tr("Scheme: ", "Schème: ", "الوزن : ") << scheme.name << " (" << scheme.pattern << ")" << endl;
            cout << "  +================================================+\n" << endl;

            matchFound = true;
            return; 
          }
        }
      }

      if (!matchFound) {
        cout << "\n  +================================================+" << endl;
        cout << "  |  " << Tr("No valid root found in the database.", "Aucune racine valide trouvée dans la base.", "لم يتم العثور على جذر صالح في قاعدة البيانات.") << endl;
        if (candidates.empty()) {
          cout << "  |  " << Tr("(No schemes found with the correct length)", "(Aucun schème trouvé avec la bonne longueur)", "(لم يتم العثور على أوزان بالطول المطلوب)") << endl;
        }
        cout << "  +================================================+\n" << endl;
      }
    }

    // PATTERN AUTO-GENERATOR
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
