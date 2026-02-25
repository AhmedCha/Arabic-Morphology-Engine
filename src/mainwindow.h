#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QMenu>
#include <QAction>
#include <QTimer>

#include "AVLTree.h"
#include "schemeHashTable.h"

// Forward declarations of your tabs
class MorphologyTab;
class RootsTab;
class SchemesTab;
class CorpusTab;

class MainWindow : public QMainWindow {
  Q_OBJECT

  public:
    MainWindow(AVLTree<std::string>* tree, SchemeHashTable* schemes, QWidget *parent = nullptr);

  protected:
    void closeEvent(QCloseEvent *event) override;

    private slots:
      void onLoadRoots();
    void onLoadSchemes();
    void onSaveData();
    void promptInitialLanguage();
    void setGlobalLanguage(int langIndex); // 0 = English, 1 = French, 2 = Arabic
#ifdef AVL_VISUALIZER
    void pollVisualizerCommands();
#endif

  private:
    void setupMenuBar();
    void retranslateUi();
    QString t(const QString& en, const QString& fr, const QString& ar);

    AVLTree<std::string>* m_tree;
    SchemeHashTable* m_schemes;

    QTabWidget* tabWidget;
    MorphologyTab* morphologyTab;
    RootsTab* rootsTab;
    SchemesTab* schemesTab;
    CorpusTab* corpusTab;

    int currentLang = 0; // 0 = EN, 1 = FR, 2 = AR

    // Menus
    QMenu* fileMenu;
    QAction* loadRootsAct;
    QAction* loadSchemesAct;
    QAction* saveAct;
    QAction* exitAct;

    QMenu* langMenu;
    QAction* actEnglish;
    QAction* actFrench;
    QAction* actArabic;

#ifdef AVL_VISUALIZER
    QTimer* visualizerTimer;
#endif
};

#endif
