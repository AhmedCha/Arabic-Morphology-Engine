#include <QApplication>
#include <string>

// Include your core logic
#include "AVLTree.h"
#include "schemeHashTable.h"

// Include the unified GUI header
#include "mainwindow.h"

int main(int argc, char *argv[]) {
  // Initialize the Qt Application
  QApplication app(argc, argv);

  // Instantiate your core data structures
  AVLTree<std::string> tree;
  SchemeHashTable schemes;

  // Load default data (if the files exist in the build directory)
  // You can also pass command line arguments here if you prefer
  tree.loadFromFile("racines.txt");
  schemes.loadFromFile("schemes.txt");

  // Create the main window, passing pointers to your data
  MainWindow window(&tree, &schemes);
  window.show();

  // Start the Qt event loop
  return app.exec();
}
