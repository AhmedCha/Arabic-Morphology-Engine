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

class corpusAnalyzer {
private:
  // Basic helper to strip standard punctuation from the start/end of a word
  static string cleanWord(const string& word) {
    string cleaned = "";

    // List of common Arabic diacritics (Tashkeel) and Tatweel to filter out
    const vector<string> diacritics = {
      // Diacritics
      "\u064B", "\u064C", "\u064D", "\u064E", 
      "\u064F", "\u0650", "\u0652", "\u0640", 
      // BiDi Marks
      "\u202B", "\u202C", "\u200E", "\u200F", "\u202A", "\u202D", "\u202E",
      // Arabic Punctuation
      "\u060C", "\u061B", "\u061F", "\u00AB", "\u00BB",
      // Western Numbers (Just in case they bypassed the 1-byte wall)
      "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
      // Eastern Arabic Numerals
      "\u0660", "\u0661", "\u0662", "\u0663", "\u0664", 
      "\u0665", "\u0666", "\u0667", "\u0668", "\u0669",
      // Persian/Urdu Numerals
      "\u06F0", "\u06F1", "\u06F2", "\u06F3", "\u06F4", 
      "\u06F5", "\u06F6", "\u06F7", "\u06F8", "\u06F9"
    };

    vector<string> chars = MorphologyEngine::splitUTF8(word);

    for (const string& c : chars) {
      // Filter standard ASCII punctuation
      if (c.length() == 1) {
            char asciiChar = c[0];
            if (isdigit(asciiChar) || ispunct(asciiChar) || asciiChar == ' ') {
                continue;
            }
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
  static void analyzeFile(const string& filename, AVLTree<string>& tree, SchemeHashTable& schemes, bool strictMode) {
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

      vector<Scheme> candidates = schemes.getSchemesByAddedLength(diff);

      for (const auto& scheme : candidates) {
        string candidateRoot = MorphologyEngine::extractRootIfMatches(word, scheme.pattern);

        if (candidateRoot != "") {
          bool rootExists = tree.search(candidateRoot);

          // Check strictMode before inserting
          if (strictMode) {
            // Strict Mode: Only add if the root is already known in the database
            if (rootExists) {
              tree.addDerivedWord(candidateRoot, word);
              derivedWordsLogged++;
              break; 
            }
          } else {
            // Discovery Mode: Add root if missing, then add the word
            if (!rootExists) {
              tree.insert(candidateRoot);
              newRootsFound++;
            }
            tree.addDerivedWord(candidateRoot, word);
            derivedWordsLogged++;
            break; 
          }
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
