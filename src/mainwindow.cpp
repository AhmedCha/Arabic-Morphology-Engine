#include "mainwindow.h"

#include "morphologyTab.h"
#include "rootsTab.h"
#include "schemesTab.h"
#include "corpusTab.h"
#include "corpusAnalyzer.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QMenuBar>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QInputDialog>
#include <QApplication>
#include <QActionGroup>

MainWindow::MainWindow(AVLTree<std::string>* tree, SchemeHashTable* schemes, QWidget *parent)
  : QMainWindow(parent), m_tree(tree), m_schemes(schemes) 
{
  resize(1000, 700);

  setupMenuBar();

  QWidget* centralWidget = new QWidget(this);
  QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

  tabWidget = new QTabWidget(this);

  // Instantiate your separated tabs
  morphologyTab = new MorphologyTab(m_tree, m_schemes, this);
  rootsTab = new RootsTab(m_tree, this);
  schemesTab = new SchemesTab(m_schemes, this);
  corpusTab = new CorpusTab(m_tree, m_schemes, this);

  // Connect Corpus modifications to the Roots tab so it updates live
  connect(corpusTab, &CorpusTab::dataModified, rootsTab, &RootsTab::refreshTable);

  tabWidget->addTab(morphologyTab, "");
  tabWidget->addTab(rootsTab, "");
  tabWidget->addTab(schemesTab, "");
  tabWidget->addTab(corpusTab, "");

  mainLayout->addWidget(tabWidget);
  setCentralWidget(centralWidget);

  // Prompt for language once on startup
  promptInitialLanguage();

#ifdef AVL_VISUALIZER
  visualizerTimer = new QTimer(this);
  connect(visualizerTimer, &QTimer::timeout, this, &MainWindow::pollVisualizerCommands);
  visualizerTimer->start(100); // 100ms interval
#endif
}

void MainWindow::promptInitialLanguage() {
  QStringList langs = {"English", "Français", "العربية"};
  bool ok;
  QString choice = QInputDialog::getItem(this, "Select Language / اختر اللغة", "Language:", langs, 0, false, &ok);

  if (ok) {
    if (choice == "Français") setGlobalLanguage(1);
    else if (choice == "العربية") setGlobalLanguage(2);
    else setGlobalLanguage(0);
  } else {
    setGlobalLanguage(0);
  }
}

void MainWindow::setGlobalLanguage(int langIndex) {
  currentLang = langIndex;

  actEnglish->setChecked(langIndex == 0);
  actFrench->setChecked(langIndex == 1);
  actArabic->setChecked(langIndex == 2);

  // Flip the Main Window layout direction for Arabic
  Qt::LayoutDirection dir = (langIndex == 2) ? Qt::RightToLeft : Qt::LeftToRight;
  this->setLayoutDirection(dir);
  tabWidget->setLayoutDirection(dir);

  // Translate MainWindow UI
  retranslateUi();

  // Broadcast the language change to all tabs directly as integers
  morphologyTab->setLanguage(langIndex);
  rootsTab->setLanguage(langIndex);
  schemesTab->setLanguage(langIndex);
  corpusTab->setLanguage(langIndex);
}

QString MainWindow::t(const QString& en, const QString& fr, const QString& ar) {
  if (currentLang == 1) return fr;
  if (currentLang == 2) return ar;
  return en;
}

void MainWindow::retranslateUi() {
  setWindowTitle(t("Arabic Morphology System", "Système de Morphologie Arabe", "نظام الصرف العربي"));

  // Update Tab Titles
  tabWidget->setTabText(0, t("Morphology Engine", "Moteur Morphologique", "المحرك الصرفي"));
  tabWidget->setTabText(1, t("Manage Roots", "Gérer les Racines", "إدارة الجذور"));
  tabWidget->setTabText(2, t("Manage Schemes", "Gérer les Schèmes", "إدارة الأوزان"));
  tabWidget->setTabText(3, t("Corpus Analyzer", "Analyseur de Corpus", "محلل النصوص"));

  // Update Menus
  fileMenu->setTitle(t("File", "Fichier", "ملف"));
  loadRootsAct->setText(t("Load Roots File...", "Charger fichier de racines...", "تحميل ملف الجذور..."));
  loadSchemesAct->setText(t("Load Schemes File...", "Charger fichier de schèmes...", "تحميل ملف الأوزان..."));
  saveAct->setText(t("Save All Data", "Sauvegarder tout", "حفظ جميع البيانات"));
  exitAct->setText(t("Exit", "Quitter", "خروج"));

  langMenu->setTitle(t("Language / اللغة", "Langue / اللغة", "اللغة / Language"));
}

void MainWindow::setupMenuBar() {
  fileMenu = menuBar()->addMenu("");

  loadRootsAct = new QAction("", this);
  connect(loadRootsAct, &QAction::triggered, this, &MainWindow::onLoadRoots);
  fileMenu->addAction(loadRootsAct);

  loadSchemesAct = new QAction("", this);
  connect(loadSchemesAct, &QAction::triggered, this, &MainWindow::onLoadSchemes);
  fileMenu->addAction(loadSchemesAct);
  fileMenu->addSeparator();

  saveAct = new QAction("", this);
  saveAct->setShortcut(QKeySequence::Save);
  connect(saveAct, &QAction::triggered, this, &MainWindow::onSaveData);
  fileMenu->addAction(saveAct);
  fileMenu->addSeparator();

  exitAct = new QAction("", this);
  exitAct->setShortcut(QKeySequence::Quit);
  connect(exitAct, &QAction::triggered, this, &QWidget::close);
  fileMenu->addAction(exitAct);

  // Language Menu
  langMenu = menuBar()->addMenu("");
  QActionGroup* langGroup = new QActionGroup(this);

  actEnglish = langMenu->addAction("English");
  actFrench = langMenu->addAction("Français");
  actArabic = langMenu->addAction("العربية");

  actEnglish->setCheckable(true);
  actFrench->setCheckable(true);
  actArabic->setCheckable(true);

  langGroup->addAction(actEnglish);
  langGroup->addAction(actFrench);
  langGroup->addAction(actArabic);

  connect(actEnglish, &QAction::triggered, this, [this](){ setGlobalLanguage(0); });
  connect(actFrench, &QAction::triggered, this, [this](){ setGlobalLanguage(1); });
  connect(actArabic, &QAction::triggered, this, [this](){ setGlobalLanguage(2); });
}

void MainWindow::closeEvent(QCloseEvent *event) {
  m_tree->saveToFile("racines.txt");
  m_schemes->saveToFile("schemes.txt");
  event->accept();
}

void MainWindow::onLoadRoots() {
  QString fileName = QFileDialog::getOpenFileName(this, t("Load Roots File", "Charger", "تحميل"), "", "Text Files (*.txt);;All Files (*)");
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

  QTextStream in(&file);
  int count = 0;
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (!line.isEmpty()) {
      QString root = line.split(QRegularExpression("\\s+")).first();
      m_tree->insert(root.toStdString());
      count++;
    }
  }
  file.close();

  rootsTab->refreshTable(); 
  QMessageBox::information(this, t("Success", "Succès", "نجاح"), t("Successfully loaded roots!", "Racines chargées!", "تم تحميل الجذور بنجاح!"));
}

void MainWindow::onLoadSchemes() {
  QString fileName = QFileDialog::getOpenFileName(this, t("Load Schemes File", "Charger", "تحميل"), "", "Text Files (*.txt);;All Files (*)");
  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

  QTextStream in(&file);
  int count = 0;
  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty()) continue;

    QStringList parts = line.split(QRegularExpression("\\s+"));
    if (parts.size() >= 2) {
      m_schemes->insert(parts[0].toStdString(), parts[1].toStdString());
      count++;
    }
  }
  file.close();

  schemesTab->refreshTable(); 
  QMessageBox::information(this, t("Success", "Succès", "نجاح"), t("Successfully loaded schemes!", "Schèmes chargés!", "تم تحميل الأوزان بنجاح!"));
}

void MainWindow::onSaveData() {
  m_tree->saveToFile("racines.txt");
  m_schemes->saveToFile("schemes.txt");
  QMessageBox::information(this, t("Success", "Succès", "نجاح"), t("Data successfully saved.", "Données sauvegardées.", "تم حفظ البيانات بنجاح."));
}

#ifdef AVL_VISUALIZER
void MainWindow::pollVisualizerCommands() {
  QFile file("/tmp/avl_cmd.txt");
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
  
  QTextStream in(&file);
  QString content = in.readAll();
  file.close();
  
  if (content.trimmed().isEmpty()) return;
  
  QFile clearFile("/tmp/avl_cmd.txt");
  if (clearFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      clearFile.close();
  }

  QStringList lines = content.split('\n', Qt::SkipEmptyParts);
  for (const QString& line : lines) {
    if (line.trimmed().isEmpty()) continue;
    
    QStringList parts = line.split(":");
    if (parts.size() < 2) continue;
    
    QString cmd = parts[0];
    QString arg = parts.mid(1).join(":");
    
    if (cmd == "insert") {
        m_tree->insert(arg.toStdString());
        rootsTab->refreshTable();
    } else if (cmd == "delete") {
        m_tree->remove(arg.toStdString());
        rootsTab->refreshTable();
    } else if (cmd == "corpus") {
        corpusAnalyzer::analyzeFile(arg.toStdString(), *m_tree, *m_schemes, false);
        rootsTab->refreshTable();
    } else if (cmd == "family") {
        std::vector<DerivedWord> family = m_tree->getFamily(arg.toStdString());
        
        QString json = "{\"type\":\"family_result\",\"key\":\"" + arg + "\",\"family\":[";
        for (size_t i = 0; i < family.size(); ++i) {
            json += "{\"word\":\"" + QString::fromStdString(family[i].word) + "\",\"frequency\":" + QString::number(family[i].frequency) + "}";
            if (i < family.size() - 1) json += ",";
        }
        json += "]}";
        
        QFile evFile("/tmp/avl_events.jsonl");
        if (evFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&evFile);
            out << json << "\n";
            evFile.close();
        }
    }
  }
}
#endif
