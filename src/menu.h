#ifndef MENU_H
#define MENU_H

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cstdlib>
#include <algorithm>

#include "AVLTree.h"
#include "schemeHashTable.h"
#include "morphologyEngine.h"
#include "treePrinter.h"
#include "corpusAnalyzer.h"
#include "language.h"

#ifdef _WIN32
  #include <conio.h>
  #define CLEAR_SCREEN "cls"
#else
  #include <termios.h>
  #include <unistd.h>
  #define CLEAR_SCREEN "clear"
  inline int _getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
  }
#endif

using namespace std;

// ASCII ART & TERMINAL HELPERS
inline void clearScreen() { system(CLEAR_SCREEN); }

inline void flushInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

inline void pauseScreen() {
    cout << "\n" << Tr("[Press ANY KEY to continue...]", 
                       "[Appuyez sur UNE TOUCHE pour continuer...]", 
                       "[اضغط على أي مفتاح للمتابعة...]") << endl;
    _getch(); 
}

// Unified ASCII decoration for all headers
inline void printAsciiHeader(const string& title) {
    cout << "\n  +================================================+" << endl;
    cout << "  |  >> " << title << endl;
    cout << "  +================================================+\n" << endl;
}

// PAGINATED INTERACTIVE MENU
inline int showInteractiveMenu(const string& title, const vector<string>& options) {
    if (options.empty()) return -1;
    
    int selected = 0;
    int numOptions = options.size();
    int maxVisible = 12; 
    
    while (true) {
        clearScreen();
        printAsciiHeader(title);
        
        int startIdx = max(0, selected - maxVisible / 2);
        int endIdx = min(numOptions, startIdx + maxVisible);
        if (endIdx - startIdx < maxVisible && numOptions > maxVisible) {
            startIdx = numOptions - maxVisible;
        }

        if (startIdx > 0) cout << Tr("      ▲ (Scroll Up)", "      ▲ (Défiler vers le haut)", "      ▲ (التمرير لأعلى)") << endl;

        for (int i = startIdx; i < endIdx; i++) {
            if (i == selected) cout << "  👉 \033[1;36m" << options[i] << "\033[0m" << endl; 
            else cout << "     " << options[i] << endl;
        }

        if (endIdx < numOptions) cout << Tr("      ▼ (Scroll Down)", "      ▼ (Défiler vers le bas)", "      ▼ (التمرير لأسفل)") << endl;
        
        cout << "\n  --------------------------------------------------" << endl;
        cout << Tr("  (Use UP/DOWN arrows, ENTER to select)", 
                   "  (Utilisez les flèches HAUT/BAS, ENTRÉE pour valider)", 
                   "  (استخدم الأسهم لأعلى/لأسفل، واضغط ENTER للاختيار)") << endl;

        int ch = _getch();
        if (ch == 224 || ch == 27) { 
            #ifndef _WIN32
            if (ch == 27) {
                int next1 = _getch();
                if (next1 == 91) { 
                    int next2 = _getch();
                    if (next2 == 65) ch = 72; else if (next2 == 66) ch = 80; 
                }
            }
            #else
            ch = _getch(); 
            #endif

            if (ch == 72) selected = (selected - 1 + numOptions) % numOptions; // UP
            else if (ch == 80) selected = (selected + 1) % numOptions;         // DOWN
        } else if (ch == '\n' || ch == '\r') {
            return selected;
        }
    }
}

// Allows the user to select the language once at boot
inline void chooseLanguage() {
    vector<string> opts = {"English", "Français", "العربية"};
    int c = showInteractiveMenu("SELECT LANGUAGE / CHOISIR LA LANGUE / اختر اللغة", opts);
    if (c == 1) currentLang = FR;
    else if (c == 2) currentLang = AR;
    else currentLang = EN;
}


// SUB-MENU: MANAGE ROOTS
inline void manageRoots(AVLTree<string>& tree) {
  vector<string> options = {
      Tr("Add New Root", "Ajouter une racine", "إضافة جذر جديد"),
      Tr("Edit Existing Root", "Modifier une racine", "تعديل جذر موجود"),
      Tr("Delete Root", "Supprimer une racine", "حذف جذر"),
      Tr("View All Roots", "Voir toutes les racines", "عرض كل الجذور"),
      Tr("Back to Main Menu", "Retour au menu principal", "العودة للقائمة الرئيسية")
  };

  while (true) {
    int choice = showInteractiveMenu(Tr("MANAGE ROOTS", "GERER LES RACINES", "إدارة الجذور"), options);
    if (choice == 4 || choice == -1) return; 

    clearScreen();
    string newRoot;
    vector<string> allRoots = tree.getAllElements();
    string cancelStr = Tr("[Cancel]", "[Annuler]", "[إلغاء]");

    switch (choice) {
      case 0: // ADD
        printAsciiHeader(Tr("ADD ROOT", "AJOUTER RACINE", "إضافة جذر"));
        cout << Tr("Enter new root: ", "Entrez la nouvelle racine: ", "أدخل الجذر الجديد: ");
        cin >> newRoot; flushInput();
        if (tree.search(newRoot)) cout << Tr("Root already exists!", "La racine existe déjà!", "الجذر موجود بالفعل!") << endl;
        else {
          tree.insert(newRoot);
          cout << Tr("Root added.", "Racine ajoutée.", "تمت إضافة الجذر بنجاح.") << endl;
        }
        break;

      case 1: // EDIT 
      {
        if (allRoots.empty()) { cout << Tr("No roots to edit!", "Aucune racine à modifier!", "لا توجد جذور لتعديلها!") << endl; break; }
        allRoots.push_back(cancelStr);
        int rIndex = showInteractiveMenu(Tr("SELECT ROOT TO EDIT", "SELECTIONNER LA RACINE A MODIFIER", "اختر الجذر لتعديله"), allRoots);
        if (rIndex == allRoots.size() - 1) break; 
        
        string targetRoot = allRoots[rIndex];
        clearScreen();
        printAsciiHeader(Tr("EDIT ROOT", "MODIFIER RACINE", "تعديل الجذر"));
        cout << Tr("Editing: ", "Modification: ", "تعديل: ") << targetRoot << "\n" << Tr("Enter NEW spelling: ", "Entrez la NOUVELLE orthographe: ", "أدخل الإملاء الجديد: ");
        cin >> newRoot; flushInput();
        
        tree.remove(targetRoot); 
        tree.insert(newRoot);   
        cout << Tr("Root updated.", "Racine mise à jour.", "تم تحديث الجذر.") << endl;
        break;
      }

      case 2: // DELETE
      {
        if (allRoots.empty()) { cout << Tr("No roots to delete!", "Aucune racine à supprimer!", "لا توجد جذور لحذفها!") << endl; break; }
        allRoots.push_back(cancelStr);
        int rIndex = showInteractiveMenu(Tr("SELECT ROOT TO DELETE", "SELECTIONNER LA RACINE A SUPPRIMER", "اختر الجذر لحذفه"), allRoots);
        if (rIndex == allRoots.size() - 1) break; 
        
        clearScreen();
        tree.remove(allRoots[rIndex]);
        cout << Tr("Root deleted.", "Racine supprimée.", "تم حذف الجذر.") << endl;
        break;
      }

      case 3: // VIEW
        printAsciiHeader(Tr("CURRENT ROOTS TREE", "ARBRE DES RACINES ACTUEL", "شجرة الجذور الحالية"));
        TreePrinter::print(tree);
        break;
    }
    pauseScreen();
  }
}

// SUB-MENU: MANAGE SCHEMES
inline void manageSchemes(SchemeHashTable& schemes) {
  vector<string> options = {
      Tr("Add New Scheme", "Ajouter un schème", "إضافة وزن جديد"),
      Tr("Edit Scheme Name", "Modifier le nom du schème", "تعديل اسم الوزن"),
      Tr("Delete Scheme", "Supprimer le schème", "حذف الوزن"),
      Tr("View All Schemes", "Voir tous les schèmes", "عرض كل الأوزان"),
      Tr("Back to Main Menu", "Retour au menu principal", "العودة للقائمة الرئيسية")
  };

  while (true) {
    int choice = showInteractiveMenu(Tr("MANAGE SCHEMES", "GERER LES SCHEMES", "إدارة الأوزان"), options);
    if (choice == 4 || choice == -1) return; 

    clearScreen();
    string newName, pattern;
    
    vector<Scheme> all = schemes.getAllSchemes();
    vector<string> schemeNames;
    for (auto& s : all) schemeNames.push_back(s.name);
    string cancelStr = Tr("[Cancel]", "[Annuler]", "[إلغاء]");

    switch (choice) {
      case 0: // ADD
        printAsciiHeader(Tr("ADD SCHEME", "AJOUTER SCHEME", "إضافة وزن"));
        cout << Tr("Enter Scheme Name: ", "Entrez le nom du schème: ", "أدخل اسم الوزن: ");
        cin >> newName; flushInput();
        if (schemes.getPattern(newName) != "") cout << Tr("Scheme exists.", "Le schème existe.", "الوزن موجود.") << endl;
        else {
          schemes.insert(newName, MorphologyEngine::derivePatternFromName(newName));
          cout << Tr("Scheme added.", "Schème ajouté.", "تمت إضافة الوزن.") << endl;
        }
        break;

      case 1: // EDIT
      {
        if (schemeNames.empty()) { cout << Tr("No schemes to edit!", "Aucun schème à modifier!", "لا توجد أوزان لتعديلها!") << endl; break; }
        schemeNames.push_back(cancelStr);
        int sIndex = showInteractiveMenu(Tr("SELECT SCHEME TO EDIT", "SELECTIONNER LE SCHEME A MODIFIER", "اختر الوزن لتعديله"), schemeNames);
        if (sIndex == schemeNames.size() - 1) break;
        
        string oldName = schemeNames[sIndex];
        clearScreen();
        printAsciiHeader(Tr("EDIT SCHEME", "MODIFIER SCHEME", "تعديل الوزن"));
        cout << Tr("Editing: ", "Modification: ", "تعديل: ") << oldName << "\n" << Tr("Enter NEW Name: ", "Entrez le NOUVEAU nom: ", "أدخل الاسم الجديد: ");
        cin >> newName; flushInput();
        
        schemes.remove(oldName);
        schemes.insert(newName, MorphologyEngine::derivePatternFromName(newName));
        cout << Tr("Scheme updated.", "Schème mis à jour.", "تم تحديث الوزن.") << endl;
        break;
      }

      case 2: // DELETE
      {
        if (schemeNames.empty()) { cout << Tr("No schemes to delete!", "Aucun schème à supprimer!", "لا توجد أوزان لحذفها!") << endl; break; }
        schemeNames.push_back(cancelStr);
        int sIndex = showInteractiveMenu(Tr("SELECT SCHEME TO DELETE", "SELECTIONNER LE SCHEME A SUPPRIMER", "اختر الوزن لحذفه"), schemeNames);
        if (sIndex == schemeNames.size() - 1) break;
        
        clearScreen();
        schemes.remove(schemeNames[sIndex]);
        cout << Tr("Scheme deleted.", "Schème supprimé.", "تم حذف الوزن.") << endl;
        break;
      }

      case 3: // VIEW
        printAsciiHeader(Tr("CURRENT SCHEMES", "SCHEMES ACTUELS", "الأوزان الحالية"));
        {
            auto all = schemes.getAllSchemes();
            if (all.empty()) {
                cout << Tr("No schemes found.", "Aucun schème trouvé.", "لا توجد أوزان.") << endl;
            } else {
                for (const auto& s : all) {
                    cout << " - " << s.name << " : " << s.pattern << endl;
                }
            }
        }
        break;
    }
    pauseScreen();
  }
}

// SUB-MENU: MORPHOLOGICAL ENGINE
inline void morphologyMenu(AVLTree<string>& tree, SchemeHashTable& schemes) {
  vector<string> options = {
    Tr("Generate Word (Single)", "Générer un mot (Simple)", "توليد كلمة (مفردة)"), 
    Tr("Validate Word", "Valider un mot", "التحقق من الكلمة"), 
    Tr("Detect Root", "Détecter la racine", "اكتشاف الجذر"), 
    Tr("Generate Family", "Générer la famille", "توليد العائلة"), 
    Tr("View Stored Family", "Voir la famille stockée", "عرض العائلة المحفوظة"), 
    Tr("Analyze Text Corpus", "Analyser le corpus de texte", "تحليل نص"), 
    Tr("Back to Main Menu", "Retour au menu principal", "العودة للقائمة الرئيسية")
  };

  while (true) {
    int choice = showInteractiveMenu(Tr("MORPHOLOGY ENGINE", "MOTEUR MORPHOLOGIQUE", "المحرك الصرفي"), options);
    if (choice == 6 || choice == -1) return; 

    clearScreen();
    string root, schemeName, word;

    switch (choice) {
      case 0: // GENERATE
        printAsciiHeader(Tr("GENERATE WORD", "GENERER UN MOT", "توليد كلمة"));
        cout << Tr("Enter Root: ", "Entrez la racine: ", "أدخل الجذر: "); cin >> root;
        cout << Tr("Enter Scheme: ", "Entrez le schème: ", "أدخل الوزن: "); cin >> schemeName; flushInput();
        {
          bool found = false;
          for(auto& s : schemes.getAllSchemes()) {
            if(s.name == schemeName) {
              cout << "\n-> " << Tr("Result: ", "Résultat: ", "النتيجة: ") << MorphologyEngine::generate(root, s.parsedPattern) << endl;
              found = true; break;
            }
          }
          if(!found) cout << "\n" << Tr("Scheme not found.", "Schème introuvable.", "الوزن غير موجود.") << endl;
        }
        break;

      case 1: // VALIDATE
        printAsciiHeader(Tr("VALIDATE WORD", "VALIDER UN MOT", "التحقق من الكلمة"));
        cout << Tr("Enter Word: ", "Entrez le mot: ", "أدخل الكلمة: "); cin >> word;
        cout << Tr("Enter Root: ", "Entrez la racine: ", "أدخل الجذر: "); cin >> root; flushInput();
        {
          string foundScheme;
          if (MorphologyEngine::validate(word, root, schemes, foundScheme, tree))
            cout << "\n✔ " << Tr("VALID (Saved to Tree)", "VALIDE (Sauvegardé dans l'arbre)", "صحيح (تم الحفظ في الشجرة)") << endl;
          else cout << "\n✘ " << Tr("INVALID", "INVALIDE", "غير صحيح") << endl;
        }
        break;

      case 2: // DETECT ROOT
        printAsciiHeader(Tr("DETECT ROOT", "DETECTER RACINE", "اكتشاف الجذر"));
        cout << Tr("Enter Word: ", "Entrez le mot: ", "أدخل الكلمة: ");
        cin >> word; flushInput();
        MorphologyEngine::findRoot(word, tree, schemes);
        break;

      case 3: // GENERATE FAMILY
        printAsciiHeader(Tr("GENERATE FAMILY", "GENERER FAMILLE", "توليد العائلة"));
        cout << Tr("Enter Root: ", "Entrez la racine: ", "أدخل الجذر: "); 
        cin >> root; flushInput();
        MorphologyEngine::generateFamily(root, tree, schemes);
        break;

      case 4: // VIEW STORED
        printAsciiHeader(Tr("VIEW STORED FAMILY", "VOIR FAMILLE STOCKEE", "عرض العائلة المحفوظة"));
        cout << Tr("Enter Root: ", "Entrez la racine: ", "أدخل الجذر: "); 
        cin >> root; flushInput();
        tree.showFamily(root);
        break;

      case 5: // CORPUS ANALYSIS
        {
          string filename;
          printAsciiHeader(Tr("CORPUS ANALYSIS", "ANALYSE DE CORPUS", "تحليل النص"));
          cout << Tr("Enter file name: ", "Nom du fichier: ", "أدخل اسم الملف: ");
          cin >> filename; flushInput();

          vector<string> modeOpts = {
              Tr("Strict Mode (Existing roots only)", "Mode Strict (Racines existantes)", "الوضع الصارم (جذور موجودة فقط)"), 
              Tr("Discovery Mode (Guess new roots)", "Mode Découverte (Deviner les racines)", "وضع الاستكشاف (تخمين جذور جديدة)")
          };
          int modeChoice = showInteractiveMenu(Tr("SELECT ANALYSIS MODE", "SELECTIONNER LE MODE D'ANALYSE", "اختر وضع التحليل"), modeOpts);
          clearScreen();

          cout << Tr("Running Analysis...", "Analyse en cours...", "جارِ التحليل...") << "\n" << endl;
          corpusAnalyzer::analyzeFile(filename, tree, schemes, modeChoice == 0);
        }
        break;
    }
    pauseScreen();
  }
}
#endif
