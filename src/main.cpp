#include <QApplication>
#include <string>
#include <thread>
#include <chrono>

// Include your core logic
#include "AVLTree.h"
#include "schemeHashTable.h"

// Include the unified GUI header
#include "mainwindow.h"
#include "menu.h"

void runCLI(AVLTree<std::string>* tree, SchemeHashTable* schemes) {
    // Wait briefly for GUI initialization before clearing the screen
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Choose language and enter main menu loop
    chooseLanguage();
    clearScreen();
    
    printAsciiHeader(Tr("SYSTEM INITIALIZATION", "INITIALISATION DU SYSTÈME", "تهيئة النظام"));
    cout << Tr("\nInitialization complete.", "\nInitialisation terminée.", "\nاكتملت التهيئة.") << endl;
    pauseScreen();

    while (true) {
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
            morphologyMenu(*tree, *schemes); 
            break;
          case 1: 
            manageRoots(*tree); 
            break;
          case 2: 
            manageSchemes(*schemes); 
            break;
          case 3: 
            chooseLanguage();
            break;
          case 4: 
            printAsciiHeader(Tr("SAVING DATA & EXITING...", "SAUVEGARDE ET FERMETURE...", "جاري حفظ البيانات والخروج..."));
            cout << Tr("Saving roots to racines.txt...", "Sauvegarde des racines dans racines.txt...", "جاري حفظ الجذور...") << endl;
            tree->saveToFile("racines.txt");
            cout << Tr("Saving schemes to schemes.txt...", "Sauvegarde des schèmes dans schemes.txt...", "جاري حفظ الأوزان...") << endl;
            schemes->saveToFile("schemes.txt");
            cout << Tr("\nExiting GUI & CLI... Goodbye!", "\nFermeture... Au revoir!", "\nجاري الخروج... وداعاً!") << endl;
            exit(0); // Exit the entire process
        }
    }
}

int main(int argc, char *argv[]) {
  // Initialize the Qt Application
  QApplication app(argc, argv);

  // Instantiate your core data structures
  AVLTree<std::string> tree;
  SchemeHashTable schemes;

  // Load default data (if the files exist in the build directory)
  tree.loadFromFile("racines.txt");
  schemes.loadFromFile("schemes.txt");

  // Create the main window, passing pointers to your data
  MainWindow window(&tree, &schemes);
  window.show();

  // Run the Terminal Interface in a separate background thread
  std::thread cliThread(runCLI, &tree, &schemes);
  cliThread.detach();

  // Start the Qt event loop
  return app.exec();
}
