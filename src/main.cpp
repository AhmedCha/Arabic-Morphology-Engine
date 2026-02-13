#include <iostream>
#include <string>
#include "AVLTree.h"
#include "schemeHashTable.h"
#include "menu.h"

using namespace std;

int main(int argc, char* argv[]) {

  AVLTree<string> tree;
  SchemeHashTable schemes;

  // Load Data
  if (argc < 3) {
    schemes.loadFromFile("schemes.txt");
    tree.loadFromFile("racines.txt");
    cout << "Loading default files, to load custom files use: " << argv[0] << " <roots_file> <schemes_file>" << endl;
  } else {
    tree.loadFromFile(argv[1]);
    schemes.loadFromFile(argv[2]);
  }

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
      case 0: cout << "Exiting... Goodbye!" << endl; return 0;
      default: cout << "Invalid selection. Try again." << endl;
    }
  }

  // Save on exit
  cout << "Saving data..." << endl;
  tree.saveToFile(argv[1]);
  schemes.saveToFile(argv[2]);
  cout << "Exiting... Goodbye!" << endl;

  return 0;
}
