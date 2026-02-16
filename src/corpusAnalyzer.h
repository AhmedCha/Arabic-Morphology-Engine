#ifndef CORPUS_ANALYZER_H
#define CORPUS_ANALYZER_H

#include <fstream>
#include <string>
#include <vector>
#include <string_view>
#include <cstdlib>
#include <unordered_map> // ADDED: Required for tracking word frequencies

#include "AVLTree.h"
#include "schemeHashTable.h"
#include "morphologyEngine.h"

using namespace std;

// THIS IS THE STRUCT THE COMPILER IS LOOKING FOR:
struct AnalyzedWord {
  std::string root;
  std::string derivedWord;
  std::string schemeName;
  int frequency;
};

struct AnalysisReport {
  bool success = false;
  std::string errorMessage;
  int totalWordsProcessed = 0;
  int derivedWordsLogged = 0;
  int newRootsFound = 0;

  std::vector<AnalyzedWord> extractedWords; 
};

class corpusAnalyzer {
  private:
    // Helper to strip standard punctuation from the start/end of a word
    static string cleanWord(const string& word) {
      string cleaned = "";

      static const vector<string_view> diacritics = {
        "\u064B", "\u064C", "\u064D", "\u064E", 
        "\u064F", "\u0650", "\u0652", "\u0640", 
        "\u202B", "\u202C", "\u200E", "\u200F", "\u202A", "\u202D", "\u202E",
        "\u060C", "\u061B", "\u061F", "\u00AB", "\u00BB",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "\u0660", "\u0661", "\u0662", "\u0663", "\u0664", 
        "\u0665", "\u0666", "\u0667", "\u0668", "\u0669",
        "\u06F0", "\u06F1", "\u06F2", "\u06F3", "\u06F4", 
        "\u06F5", "\u06F6", "\u06F7", "\u06F8", "\u06F9"
      };

      vector<string_view> chars = MorphologyEngine::splitUTF8(word);

      for (const auto& c : chars) {
        if (c.length() == 1) {
          char asciiChar = c[0];
          if (isdigit(asciiChar) || ispunct(asciiChar) || asciiChar == ' ') {
            continue;
          }
        }

        bool isDiacritic = false;
        for (const auto& d : diacritics) {
          if (c == d) {
            isDiacritic = true;
            break;
          }
        }

        if (!isDiacritic) {
          cleaned += string(c);
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

      return (string(buffer, 5) == "%PDF-");
    }

    static string stripPrefixes(const string& word) {
      vector<string_view> chars = MorphologyEngine::splitUTF8(word);

      // We only strip if the remaining word will have at least 3 letters 
      // (since valid Arabic roots/schemes are at least 3 letters long).
      if (chars.size() >= 5) {
        string first = string(chars[0]);
        string second = string(chars[1]);
        string third = string(chars[2]);

        // 1. Check for 3-letter prefixes: وال (wa-al), فال (fa-al), بال (bi-al), كال (ka-al)
        if (chars.size() >= 6) {
          if ((first == "و" || first == "ف" || first == "ب" || first == "ك") && 
              second == "ا" && third == "ل") {
            string stripped = "";
            for (size_t i = 3; i < chars.size(); ++i) stripped += chars[i];
            return stripped;
          }
        }

        // 2. Check for 2-letter prefixes: ال (al), لل (lil)
        if ((first == "ا" && second == "ل") || (first == "ل" && second == "ل")) {
          string stripped = "";
          for (size_t i = 2; i < chars.size(); ++i) stripped += chars[i];
          return stripped;
        }
      }

      return word; // Return as-is if no prefix matched
    }

  public:
    static AnalysisReport analyzeFile(const string& filename, AVLTree<string>& tree, const SchemeHashTable& schemes, bool strictMode) {
      AnalysisReport report;
      string fileToProcess = filename;
      bool isTempFile = false;

      if (isPDF(filename)) {
        fileToProcess = "temp_corpus_extracted.txt";
        string safeFilename = filename;
        safeFilename.erase(remove(safeFilename.begin(), safeFilename.end(), '\"'), safeFilename.end());
        safeFilename.erase(remove(safeFilename.begin(), safeFilename.end(), ';'), safeFilename.end());
        
        string command = "pdftotext -enc UTF-8 \"" + safeFilename + "\" \"" + fileToProcess + "\"";

        int result = system(command.c_str());
        if (result != 0) {
          report.errorMessage = "Failed to convert PDF. Ensure 'pdftotext' is installed.";
          return report;
        }
        isTempFile = true;
      }

      ifstream file(fileToProcess);
      if (!file.is_open()) {
        report.errorMessage = "Could not open corpus file: " + fileToProcess;
        return report;
      }

      // --- TRACKING MAP ---
      // Key: derivedWord | Value: AnalyzedWord
      unordered_map<string, AnalyzedWord> localWordFrequencies;

      string rawWord;
      while (file >> rawWord) {
        string word = MorphologyEngine::sanitize(cleanWord(rawWord));
        word = stripPrefixes(word);
        if (word.empty()) continue;

        report.totalWordsProcessed++;

        vector<string_view> wChars = MorphologyEngine::splitUTF8(word);
        int wordLen = wChars.size();
        int diff = wordLen - 3;

        if (diff < 0) continue;

        vector<Scheme> candidates = schemes.getSchemesByAddedLength(diff);

        for (const auto& scheme : candidates) {
          string candidateRoot = MorphologyEngine::extractRootIfMatches(word, scheme.pattern);

          if (candidateRoot != "") {
            bool rootExists = tree.search(candidateRoot);
            bool wordAccepted = false;

            // Check if we accept this root based on the mode
            if (strictMode) {
              if (rootExists) {
                wordAccepted = true;
              }
            } else { // Brute-force mode
              if (!rootExists) {
                tree.insert(candidateRoot);
                report.newRootsFound++;
              }
              wordAccepted = true;
            }

            // If the word passed our checks, log it!
            if (wordAccepted) {
              tree.addDerivedWord(candidateRoot, word);
              report.derivedWordsLogged++;

              // --- POPULATE THE TRACKING MAP ---
              if (localWordFrequencies.find(word) != localWordFrequencies.end()) {
                // We've seen this exact word before, just bump the frequency count
                localWordFrequencies[word].frequency++;
              } else {
                // First time seeing this word, create a new record
                AnalyzedWord newWordData;
                newWordData.root = candidateRoot;
                newWordData.derivedWord = word;
                newWordData.schemeName = scheme.name; 
                newWordData.frequency = 1;

                localWordFrequencies[word] = newWordData;
              }

              break; // We found the matching scheme, stop checking other candidates for this word
            }
          }
        }
      }

      file.close();
      if (isTempFile) {
        remove(fileToProcess.c_str());
      }

      // --- TRANSFER MAP DATA TO REPORT LIST ---
      // The UI table needs a vector, so we push all map values into the extractedWords vector
      for (const auto& pair : localWordFrequencies) {
        report.extractedWords.push_back(pair.second);
      }

      report.success = true;
      return report;
    }
};

#endif
