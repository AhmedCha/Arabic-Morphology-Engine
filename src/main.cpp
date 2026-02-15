#include <QApplication>
#include <string>

// Include your core logic
#include "AVLTree.h"
#include "schemeHashTable.h"

// Include the unified GUI header
#include "mainwindow.h"

int main(int argc, char *argv[]) {
  // 1. Initialize the Qt Application
  QApplication app(argc, argv);

  // 2. Instantiate your core data structures
  AVLTree<std::string> tree;
  SchemeHashTable schemes;

  // 3. Load default data (if the files exist in the build directory)
  // You can also pass command line arguments here if you prefer
  tree.loadFromFile("racines.txt");
  schemes.loadFromFile("schemes.txt");

  // 4. Create the main window, passing pointers to your data
  MainWindow window(&tree, &schemes);
  window.show();

  // 5. Start the Qt event loop
  return app.exec();
}
