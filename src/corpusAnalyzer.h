#ifndef CORPUS_ANALYZER_H
#define CORPUS_ANALYZER_H

#include <fstream>
#include <string>
#include <vector>
#include <string_view>
#include <cstdlib>
#include <list>

#include "AVLTree.h"
#include "schemeHashTable.h"
#include "morphologyEngine.h"
#include "StringUtils.h"

using namespace std;

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

class WordFrequencyMap {
  private:
    static const int TABLE_SIZE = 1009;
    list<AnalyzedWord> table[TABLE_SIZE];

    int hash(const string& key) const {
      int h = 0;
      for (unsigned char c : key) {
        h = (h * 31 + c) % TABLE_SIZE;
      }
      return h;
    }

  public:
    void addWord(const string& derivedWord, const string& root, const string& schemeName) {
      int index = hash(derivedWord);

      // If word exists, increment frequency
      for (auto& aw : table[index]) {
        if (aw.derivedWord == derivedWord) {
          aw.frequency++;
          return;
        }
      }

      // Otherwise, add new word
      AnalyzedWord newWord = {root, derivedWord, schemeName, 1};
      table[index].push_back(newWord);
    }

    void exportToVector(vector<AnalyzedWord>& outVector) const {
      for (int i = 0; i < TABLE_SIZE; i++) {
        for (const auto& aw : table[i]) {
          outVector.push_back(aw);
        }
      }
    }
};

class corpusAnalyzer {
  private:
    static string cleanWord(const string& word) {
      string cleaned = "";

      static const vector<string_view> diacritics = {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "\u00AB", "\u00BB", "\u060C", "\u061B", "\u061F", 
        "\u0640", "\u064B", "\u064C", "\u064D", "\u064E", 
        "\u064F", "\u0650", "\u0652", "\u0660", "\u0661", 
        "\u0662", "\u0663", "\u0664", "\u0665", "\u0666", 
        "\u0667", "\u0668", "\u0669", "\u06F0", "\u06F1", 
        "\u06F2", "\u06F3", "\u06F4", "\u06F5", "\u06F6", 
        "\u06F7", "\u06F8", "\u06F9", "\u200E", "\u200F", 
        "\u202A", "\u202B", "\u202C", "\u202D", "\u202E"
      };

      vector<string_view> chars = StringUtils::splitUTF8(word);

      for (const auto& c : chars) {
        if (c.length() == 1) {
          char asciiChar = c[0];
          bool isNumber = (asciiChar >= '0' && asciiChar <= '9');
          bool isPunctuation = (asciiChar >= '!' && asciiChar <= '/') || 
            (asciiChar >= ':' && asciiChar <= '@') || 
            (asciiChar >= '[' && asciiChar <= '`') || 
            (asciiChar >= '{' && asciiChar <= '~');

          if (isNumber || isPunctuation || asciiChar == ' ') {
            continue;
          }
        }

        bool isDiacritic = false;
        int left = 0;
        int right = diacritics.size() - 1;

        while (left <= right) {
          int mid = left + (right - left) / 2;

          if (diacritics[mid] == c) {
            isDiacritic = true;
            break;
          }
          else if (diacritics[mid] < c) {
            left = mid + 1;
          }
          else {
            right = mid - 1;
          }
        }

        if (!isDiacritic) {
          cleaned += string(c);
        }
      }

      if (!cleaned.empty()) {
        // This array is STRICTLY ALPHABETICALLY SORTED by UTF-8 bytes!
        // If you add new words to this list, you MUST keep them in alphabetical order
        // otherwise the binary search will fail to find them.
        static const vector<string> stopWords = {
          "أجل", "أنا", "أنت", "أنتم", "أو", "أولئك", "أي", "أين",
          "إذن", "إلا", "إلى", "إليك", "إن", "إنما",
          "اب", "ابن", "ابو", "التي", "الذي", "الذين", "اللاتي", "اللواتي", 
          "الى", "اليك", "اليكم", "ام", "اما", "انا", "انت", "انتم", "انما", "انه", "او", "اي", "اين",
          "بل", "به", "بها", "بهم", "بين",
          "تلك",
          "ثم",
          "حتى", "حين",
          "ذلك",
          "رب",
          "سوف", "سوى",
          "صار",
          "عدا", "على", "عليه", "عليها", "عليهم", "عن", "عند", "عندما",
          "غير",
          "في", "فيه", "فيها", "فيهم",
          "قد", "قط",
          "كأن", "كان", "كانت", "كذلك", "كل", "كلا", "كلما", "كم", "كما", "كيف",
          "لا", "لعل", "لقد", "لك", "لكم", "لكن", "لم", "لما", "لن", "لو", "لولا", "لي", "ليت", "ليس",
          "ما", "ماذا", "متى", "مذ", "مع", "مما", "من", "منذ", "منه", "منها", "منهم", "مهما",
          "نحن", "نحو", "نعم",
          "هؤلاء", "هاتان", "هذا", "هذان", "هذه", "هل", "هم", "هما", "هن", "هنا", "هناك", "هو", "هي",
          "يا"
        };

        int left = 0;
        int right = stopWords.size() - 1;
        bool isStopWord = false;

        while (left <= right) {
          int mid = left + (right - left) / 2;
          if (stopWords[mid] == cleaned) {
            isStopWord = true;
            break;
          } else if (stopWords[mid] < cleaned) {
            left = mid + 1;
          } else {
            right = mid - 1;
          }
        }

        // If it's a common word, return an empty string to instantly discard it
        if (isStopWord) {
          return ""; 
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
      vector<string_view> chars = StringUtils::splitUTF8(word);

      if (chars.size() >= 5) {
        string first = string(chars[0]);
        string second = string(chars[1]);
        string third = string(chars[2]);

        if (chars.size() >= 6) {
          if ((first == "و" || first == "ف" || first == "ب" || first == "ك") && 
              second == "ا" && third == "ل") {
            string stripped = "";
            for (size_t i = 3; i < chars.size(); ++i) stripped += chars[i];
            return stripped;
          }
        }

        if ((first == "ا" && second == "ل") || (first == "ل" && second == "ل")) {
          string stripped = "";
          for (size_t i = 2; i < chars.size(); ++i) stripped += chars[i];
          return stripped;
        }
      }

      return word; 
    }

  public:
    static AnalysisReport analyzeFile(const string& filename, AVLTree<string>& tree, const SchemeHashTable& schemes, bool strictMode) {
      AnalysisReport report;
      string fileToProcess = filename;
      bool isTempFile = false;

      if (isPDF(filename)) {
        fileToProcess = "temp_corpus_extracted.txt";

        string safeFilename = "";
        for (char c : filename) {
          if (c != '\"' && c != ';' && c != '&' && c != '|' && c != '`' && c != '$') {
            safeFilename += c;
          }
        }

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

      WordFrequencyMap localWordFrequencies;

      string rawWord;
      while (file >> rawWord) {
        string word = StringUtils::sanitize(cleanWord(rawWord));
        word = stripPrefixes(word);
        if (word.empty()) continue;

        report.totalWordsProcessed++;

        vector<string_view> wChars = StringUtils::splitUTF8(word);
        int wordLen = wChars.size();
        int diff = wordLen - 3;

        if (diff < 0) continue;

        vector<Scheme> candidates = schemes.getSchemesByAddedLength(diff);

        for (const auto& scheme : candidates) {
          // extractRootIfMatches remains here because it's morphology specific
          string candidateRoot = MorphologyEngine::extractRootIfMatches(word, scheme.pattern);

          if (candidateRoot != "") {
            bool rootExists = tree.search(candidateRoot);
            bool wordAccepted = false;

            if (strictMode) {
              if (rootExists) {
                wordAccepted = true;
              }
            } else { 
              if (!rootExists) {
                tree.insert(candidateRoot);
                report.newRootsFound++;
              }
              wordAccepted = true;
            }

            if (wordAccepted) {
              tree.addDerivedWord(candidateRoot, word);
              report.derivedWordsLogged++;

              localWordFrequencies.addWord(word, candidateRoot, scheme.name);

              break; 
            }
          }
        }
      }

      file.close();
      if (isTempFile) {
        remove(fileToProcess.c_str());
      }

      localWordFrequencies.exportToVector(report.extractedWords);

      report.success = true;
      return report;
    }
};

#endif
