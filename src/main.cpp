#include <iostream>
#include <string>
#include <vector>

#include "AVLTree.h"
#include "language.h"
#include "schemeHashTable.h"
#include "menu.h"

using namespace std;

int main(int argc, char* argv[]) {

  // Ask for language immediately so even the loading screen is translated!
  chooseLanguage();
  clearScreen();

  AVLTree<string> tree;
  SchemeHashTable schemes;
  string schemeFile;
  string rootsFile;

  // Load Data
  printAsciiHeader(Tr("SYSTEM INITIALIZATION", "INITIALISATION DU SYSTÈME", "تهيئة النظام"));

  if (argc < 3) {
    schemeFile = "schemes.txt";
    rootsFile = "racines.txt";
    cout << Tr("Loading default files. To load custom files use: ", 
               "Chargement des fichiers par défaut. Pour fichiers personnalisés: ", 
               "جاري تحميل الملفات الافتراضية. لتحميل ملفات مخصصة استخدم: ") 
         << argv[0] << " <roots_file> <schemes_file>\n" << endl;
  } else {
    rootsFile = argv[1];
    schemeFile = argv[2];
  }

  schemes.loadFromFile(schemeFile);
  tree.loadFromFile(rootsFile);

  cout << Tr("\nInitialization complete.", "\nInitialisation terminée.", "\nاكتملت التهيئة.") << endl;
  pauseScreen(); // Make sure they can read the loading messages

  while (true) {
    // We define this INSIDE the loop so if they change the language, 
    // the vector instantly updates on the next menu load!
    vector<string> mainOptions = {
      Tr("Morphology Operations", "Opérations Morphologiques", "العمليات الصرفية"),
      Tr("Manage Roots", "Gérer les Racines", "إدارة الجذور"),
      Tr("Manage Schemes", "Gérer les Schèmes", "إدارة الأوزان"),
      Tr("Change Language", "Changer de Langue", "تغيير اللغة"),
      Tr("Save Data & Exit", "Sauvegarder et Quitter", "حفظ البيانات والخروج")
    };

    int mainChoice = showInteractiveMenu(Tr("ARABIC MORPHOLOGY SYSTEM", "SYSTÈME DE MORPHOLOGIE ARABE", "نظام الصرف العربي"), mainOptions);

    clearScreen(); 

    switch (mainChoice) {
      case 0: 
        morphologyMenu(tree, schemes); 
        break;
      case 1: 
        manageRoots(tree); 
        break;
      case 2: 
        manageSchemes(schemes); 
        break;
      case 3: // Change Language on the fly!
        chooseLanguage();
        break;
      case 4: // Exit
        printAsciiHeader(Tr("SAVING DATA & EXITING...", "SAUVEGARDE ET FERMETURE...", "جاري حفظ البيانات والخروج..."));
        
        cout << Tr("Saving roots to ", "Sauvegarde des racines dans ", "جاري حفظ الجذور في ") << rootsFile << "..." << endl;
        tree.saveToFile(rootsFile);
        
        cout << Tr("Saving schemes to ", "Sauvegarde des schèmes dans ", "جاري حفظ الأوزان في ") << schemeFile << "..." << endl;
        schemes.saveToFile(schemeFile);
        
        cout << Tr("\nExiting... Goodbye!", "\nFermeture... Au revoir!", "\nجاري الخروج... وداعاً!") << endl;
        return 0;
    }
  }
}
