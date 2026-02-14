#include <iostream>
#include <string>
#include "AVLTree.h"
#include "schemeHashTable.h"
#include "menu.h"

using namespace std;

int main(int argc, char* argv[]) {

  AVLTree<string> tree;
  SchemeHashTable schemes;
  string schemeFile;
  string rootsFile;

  // Load Data
  if (argc < 3) {
    schemeFile = "schemes.txt";
    rootsFile = "racines.txt";
    cout << "Loading default files, to load custom files use: " << argv[0] << " <roots_file> <schemes_file>" << endl;
  } else {
    rootsFile = argv[1];
    schemeFile = argv[2];
  }

  schemes.loadFromFile(schemeFile);
  tree.loadFromFile(rootsFile);

  int mainChoice;

  while (true) {
    cout << "\n========================================" << endl;
    cout << "   ARABIC MORPHOLOGY SYSTEM" << endl;
    cout << "========================================" << endl;
    cout << "1. Morphology Operations (Generate/Validate)" << endl;
    cout << "2. Manage Roots (Add/Edit/Delete)" << endl;
    cout << "3. Manage Schemes (Add/Edit/Delete)" << endl;
    cout << "0. Exit" << endl;
    cout << "========================================" << endl;
    cout << "Select Option: ";
    cin >> mainChoice;

    if (cin.fail()) {
      cin.clear();
      cin.ignore(10000, '\n');
      mainChoice = -1;
    }

    switch (mainChoice) {
      case 1: morphologyMenu(tree, schemes); break;
      case 2: manageRoots(tree); break;
      case 3: manageSchemes(schemes); break;
      case 0: 
              cout << "\nSaving data before exit..." << endl;
              tree.saveToFile(rootsFile);
              schemes.saveToFile(schemeFile);
              cout << "Exiting... Goodbye!" << endl;
              return 0;
      default: 
              cout << "Invalid selection. Try again." << endl;
    }
  }
}
