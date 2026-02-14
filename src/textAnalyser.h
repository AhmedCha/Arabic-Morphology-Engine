#ifndef CORPUS_ANALYZER_H
#define CORPUS_ANALYZER_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "AVLTree.h"
#include "schemeHashTable.h"
#include "morphologyEngine.h"

using namespace std;

class CorpusAnalyzer {
private:
  // Basic helper to strip standard punctuation from the start/end of a word
  static string cleanWord(const string& word) {
    string cleaned = "";

    // List of common Arabic diacritics (Tashkeel) and Tatweel to filter out
    const vector<string> diacritics = {
      "\u064B", 
      "\u064C", 
      "\u064D", 
      "\u064E", 
      "\u064F", 
      "\u0650", 
      "\u0651", 
      "\u0652", 
      "\u0640", 
      "\u202B", 
      "\u202C", 
      "\u200E", 
      "\u200F", 
      "\u202A", 
      "\u202D", 
      "\u202E"  
    };

    vector<string> chars = MorphologyEngine::splitUTF8(word);

    for (const string& c : chars) {
      // Filter standard ASCII punctuation
      if (c.length() == 1 && ispunct(c[0])) {
        continue; 
      }

      // Filter Arabic diacritics
      bool isDiacritic = false;
      for (const string& d : diacritics) {
        if (c == d) {
          isDiacritic = true;
          break;
        }
      }

      // If it's neither punctuation nor a diacritic, keep it
      if (!isDiacritic) {
        cleaned += c;
      }
    }

    return cleaned;
  }

  static bool isPDF(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) return false;

    char buffer[5];
    file.read(buffer, 5);
    file.close();

    // Every valid PDF starts with "%PDF-"
    return (string(buffer, 5) == "%PDF-");
  }

public:
  static void analyzeFile(const string& filename, AVLTree<string>& tree, SchemeHashTable& schemes) {
    string fileToProcess = filename;
    bool isTempFile = false;

    // DETECT FILE TYPE
    if (isPDF(filename)) {
      cout << "ℹ PDF detected. Converting to plain text..." << endl;
      fileToProcess = "temp_corpus_extracted.txt";

      // CONVERT PDF TO TXT USING SYSTEM COMMAND
      // Linux: usually pre-installed or 'sudo apt install poppler-utils'
      // Windows: Download Xpdf tools and add to PATH.
      string command = "pdftotext -enc UTF-8 \"" + filename + "\" \"" + fileToProcess + "\"";

      int result = system(command.c_str());
      if (result != 0) {
        cerr << "✘ Error: Failed to convert PDF. Ensure 'pdftotext' is installed." << endl;
        return;
      }
      isTempFile = true;
    } else {
      cout << "ℹ Plain text file detected." << endl;
    }

    // READ THE TEXT FILE
    ifstream file(fileToProcess);
    if (!file.is_open()) {
      cerr << "✘ Error: Could not open corpus file: " << fileToProcess << endl;
      return;
    }

    string rawWord;
    int totalWordsProcessed = 0;
    int newRootsFound = 0;
    int derivedWordsLogged = 0;

    cout << "Analyzing corpus: " << filename << "..." << endl;

    while (file >> rawWord) {
      string word = cleanWord(rawWord);
      if (word.empty()) continue;

      totalWordsProcessed++;

      vector<string> wChars = MorphologyEngine::splitUTF8(word);
      int wordLen = wChars.size();
      int diff = wordLen - 3;

      if (diff < 0) continue;

      // Fetch only schemes that match the required length addition
      vector<Scheme> candidates = schemes.getSchemesByAddedLength(diff);

      for (const auto& scheme : candidates) {
        // Try to extract the root based on the scheme
        string candidateRoot = MorphologyEngine::extractRootIfMatches(word, scheme.pattern);

        if (candidateRoot != "") {

          if (!tree.search(candidateRoot)) {
            tree.insert(candidateRoot);
            newRootsFound++;
          }

          tree.addDerivedWord(candidateRoot, word);
          derivedWordsLogged++;

          break; 
        }
      }
    }

    file.close();
    if (isTempFile) {
      remove(fileToProcess.c_str());
    }

    // Print Summary Report
    cout << "\n========================================" << endl;
    cout << "       CORPUS ANALYSIS COMPLETE" << endl;
    cout << "========================================" << endl;
    cout << "Words Processed:     " << totalWordsProcessed << endl;
    cout << "Derived Words Found: " << derivedWordsLogged << endl;
    cout << "New Roots Extracted: " << newRootsFound << endl;
    cout << "========================================\n" << endl;
  }
};

#endif
