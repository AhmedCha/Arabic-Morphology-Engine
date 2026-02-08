#include <iostream>
#include <fstream>
#include <string>
#include "AVLTree.h"
#include "treePrinter.h"
#include "schemeHashTable.h"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cerr << "Usage: " << argv[0] << " <roots_file> <schemes_file>" << endl;
    return 1;
  }

  string rootsFile = argv[1];
  string schemesFile = argv[2];

  AVLTree<string> tree;
  tree.loadFromFile(rootsFile);

  SchemeHashTable schemesTable;
  schemesTable.loadFromFile(schemesFile);

  cout << "--- Current Roots Tree ---" << endl;
  TreePrinter::print(tree);

  cout << "\n--- Current Schemes ---" << endl;
  schemesTable.display(); 

  return 0;
}
