#ifndef MENU_H
#define MENU_H

#include <iostream>
#include <string>
#include <limits>
#include "AVLTree.h"
#include "schemeHashTable.h"
#include "morphologyEngine.h"
#include "treePrinter.h"

using namespace std;

// SUB-MENU: MANAGE ROOTS
inline void manageRoots(AVLTree<string>& tree) {
  int choice;
  string inputRoot, newRoot;

  while (true) {
    cout << "\n--- MANAGE ROOTS ---" << endl;
    cout << "1. Add New Root" << endl;
    cout << "2. Edit Existing Root" << endl; 
    cout << "3. Delete Root" << endl;
    cout << "4. View All Roots" << endl;
    cout << "0. Back to Main Menu" << endl;
    cout << "Choice: ";
    cin >> choice;

    if (choice == 0) return;

    switch (choice) {
      case 1: // ADD
        cout << "Enter new root (e.g., ktb): ";
        cin >> inputRoot;
        if (tree.search(inputRoot)) {
          cout << "Root '" << inputRoot << "' already exists!" << endl;
        } else {
          tree.insert(inputRoot);
          cout << "Root '" << inputRoot << "' added successfully." << endl;
        }
        break;

      case 2: // EDIT
        cout << "Enter the root to edit: ";
        cin >> inputRoot;
        if (!tree.search(inputRoot)) {
          cout << "Root '" << inputRoot << "' not found." << endl;
        } else {
          cout << "Enter the NEW spelling: ";
          cin >> newRoot;
          tree.remove(inputRoot); 
          tree.insert(newRoot);   
          cout << "Root updated from '" << inputRoot << "' to '" << newRoot << "'." << endl;
        }
        break;

      case 3: // DELETE
        cout << "Enter root to delete: ";
        cin >> inputRoot;
        if (!tree.search(inputRoot)) {
          cout << "Root '" << inputRoot << "' not found." << endl;
        } else {
          tree.remove(inputRoot);
          cout << "Root '" << inputRoot << "' deleted." << endl;
        }
        break;

      case 4: // VIEW
        cout << "\nCurrent Roots Tree:" << endl;
        TreePrinter::print(tree);
        break;

      default:
        cout << "Invalid choice." << endl;
    }
  }
}

// SUB-MENU: MANAGE SCHEMES
inline void manageSchemes(SchemeHashTable& schemes) {
  int choice;
  string oldName, newName, pattern;

  while (true) {
    cout << "\n--- MANAGE SCHEMES ---" << endl;
    cout << "1. Add New Scheme" << endl;
    cout << "2. Edit Scheme Name (Auto-update Pattern)" << endl;
    cout << "3. Delete Scheme" << endl;
    cout << "4. View All Schemes" << endl;
    cout << "0. Back to Main Menu" << endl;
    cout << "Choice: ";
    cin >> choice;

    if (choice == 0) return;

    switch (choice) {
      case 1: // ADD
        cout << "Enter Scheme Name (e.g., مفعول): ";
        cin >> newName;
        if (schemes.getPattern(newName) != "") {
          cout << "Scheme '" << newName << "' already exists." << endl;
        } else {
          pattern = MorphologyEngine::derivePatternFromName(newName);
          schemes.insert(newName, pattern);
          cout << "Scheme '" << newName << "' added successfully." << endl;
        }
        break;

      case 2: // EDIT
        cout << "Enter OLD Scheme Name to edit: ";
        cin >> oldName;
        if (schemes.getPattern(oldName) == "") {
          cout << "Scheme '" << oldName << "' not found." << endl;
        } else {
          cout << "Enter NEW Scheme Name: ";
          cin >> newName;
          schemes.remove(oldName);
          pattern = MorphologyEngine::derivePatternFromName(newName);
          schemes.insert(newName, pattern);
          cout << "Scheme updated from '" << oldName << "' to '" << newName << "'." << endl;
        }
        break;

      case 3: // DELETE
        cout << "Enter Scheme Name to delete: ";
        cin >> oldName;
        schemes.remove(oldName);
        break;

      case 4: // VIEW
        schemes.display();
        break;

      default:
        cout << "Invalid choice." << endl;
    }
  }
}

// SUB-MENU: MORPHOLOGICAL ENGINE
inline void morphologyMenu(AVLTree<string>& tree, SchemeHashTable& schemes) {
  int choice;
  string root, schemeName, word;

  while (true) {
    cout << "\n--- ⚙ MORPHOLOGY ENGINE ---" << endl;
    cout << "1. Generate Word (Single)" << endl;
    cout << "2. Validate Word" << endl;
    cout << "3. Detect Root" << endl;
    cout << "4. Generate Family (All Schemes)" << endl;
    cout << "5. View Stored Family of Root" << endl;
    cout << "0. Back" << endl;
    cin >> choice;

    if (choice == 0) return;

    switch (choice) {
      case 1: // GENERATE
        cout << "Enter Root: "; cin >> root;
        cout << "Enter Scheme Name: "; cin >> schemeName;
        {
          string pattern = schemes.getPattern(schemeName);

          vector<Scheme> all = schemes.getAllSchemes();
          bool found = false;
          for(auto& s : all) {
            if(s.name == schemeName) {
              cout << "-> " << MorphologyEngine::generate(root, s.parsedPattern) << endl;
              found = true; break;
            }
          }
          if(!found) cout << "Scheme not found." << endl;
        }
        break;

      case 2: // VALIDATE
        cout << "Enter Word: "; cin >> word;
        cout << "Enter Root: "; cin >> root;
        {
          string foundScheme;
          if (MorphologyEngine::validate(word, root, schemes, foundScheme, tree))
            cout << "✔ VALID (Matches: " << foundScheme << ") - Saved to Tree" << endl;
          else
            cout << "✘ INVALID" << endl;
        }
        break;

      case 3: // DETECT ROOT
        cout << "Enter Word to reverse-engineer: ";
        cin >> word;
        // Call the optimized finder you already wrote!
        MorphologyEngine::findRoot(word, tree, schemes);
        break;

      case 4: // FAMILY GENERATION
        cout << "Enter Root: "; cin >> root;
        MorphologyEngine::generateFamily(root, tree, schemes);
        break;

      case 5: // VIEW STORED
        cout << "Enter Root: "; cin >> root;
        tree.showFamily(root);
        break;
    }
  }
}

#endif
