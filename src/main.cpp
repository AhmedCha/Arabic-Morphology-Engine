#include <iostream>
#include <fstream>
#include <string>
#include "AVLTree.h" // Your template header
#include "treePrinter.h"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <filename>" << endl;
    return 1;
  }

  AVLTree<string> tree;
  string filename = argv[1];
  ifstream file(filename);

  if (!file.is_open()) {
    cerr << "Error: Could not open " << filename << endl;
    return 1;
  }

  string line;
  while (getline(file, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty()) tree.insert(line);
  }
  file.close();
  return 0;
}
