#ifndef CORPUSTAB_H
#define CORPUSTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>

#include "AVLTree.h"
#include "schemeHashTable.h"
#include "corpusAnalyzer.h"

class CorpusTab : public QWidget {
  Q_OBJECT
  private:
    AVLTree<std::string>* m_tree;
    SchemeHashTable* m_schemes;

    int currentLang = 0; // 0 = EN, 1 = FR, 2 = AR

    // UI Elements
    QLabel* descLabel;
    QPushButton* analyzeBtn;

    // Translation Helper
    QString t(const QString& en, const QString& fr, const QString& ar) {
      if (currentLang == 1) return fr;
      if (currentLang == 2) return ar;
      return en;
    }

    void retranslateUi() {
      descLabel->setText(t("Select a .txt or .pdf file to extract derived words and analyze roots.", 
            "Sélectionnez un fichier .txt ou .pdf pour extraire les mots dérivés et analyser les racines.", 
            "حدد ملف .txt أو .pdf لاستخراج الكلمات المشتقة وتحليل الجذور."));

      analyzeBtn->setText(t("Select File & Analyze", 
            "Sélectionner le Fichier et Analyser", 
            "تحديد الملف والتحليل"));
    }

    // Helper to display the interactive results table
    void showAnalysisResultsDialog(const AnalysisReport& report) {
      QDialog dialog(this);
      dialog.setWindowTitle(t("Analysis Results", "Résultats de l'Analyse", "نتائج التحليل"));
      dialog.resize(600, 500);
      dialog.setLayoutDirection((currentLang == 2) ? Qt::RightToLeft : Qt::LeftToRight);

      QVBoxLayout layout(&dialog);

      // Summary Label at the top
      QLabel* summaryLabel = new QLabel(
          t("Total Words: %1 | Derived Found: %2 | New Roots: %3", 
            "Mots Totaux: %1 | Dérivés Trouvés: %2 | Nouvelles Racines: %3", 
            "إجمالي الكلمات: %1 | المشتقات: %2 | الجذور الجديدة: %3")
          .arg(report.totalWordsProcessed)
          .arg(report.derivedWordsLogged)
          .arg(report.newRootsFound)
          );
      summaryLabel->setAlignment(Qt::AlignCenter);
      QFont boldFont = summaryLabel->font();
      boldFont.setBold(true);
      summaryLabel->setFont(boldFont);
      layout.addWidget(summaryLabel);

      // Results Table
      QTableWidget table(report.extractedWords.size(), 4);
      table.setHorizontalHeaderLabels({
          t("Root", "Racine", "الجذر"), 
          t("Derived Word", "Mot Dérivé", "الكلمة المشتقة"), 
          t("Scheme", "Schème", "الوزن"), 
          t("Frequency", "Fréquence", "التكرار")
          });

      table.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
      table.setEditTriggers(QAbstractItemView::NoEditTriggers);
      table.setSelectionBehavior(QAbstractItemView::SelectRows);

      table.setSortingEnabled(false); // Turn off sorting while populating

      const QChar RLM(0x200F); 

      for (size_t i = 0; i < report.extractedWords.size(); ++i) {
        const auto& wordData = report.extractedWords[i];

        QTableWidgetItem* rootItem = new QTableWidgetItem(RLM + QString::fromStdString(wordData.root));
        QTableWidgetItem* wordItem = new QTableWidgetItem(RLM + QString::fromStdString(wordData.derivedWord));
        QTableWidgetItem* schemeItem = new QTableWidgetItem(RLM + QString::fromStdString(wordData.schemeName));

        // For frequency, we use setData with Qt::EditRole so Qt sorts it as a NUMBER, not a string!
        QTableWidgetItem* freqItem = new QTableWidgetItem();
        freqItem->setData(Qt::EditRole, wordData.frequency); 

        rootItem->setTextAlignment(Qt::AlignCenter);
        wordItem->setTextAlignment(Qt::AlignCenter);
        schemeItem->setTextAlignment(Qt::AlignCenter);
        freqItem->setTextAlignment(Qt::AlignCenter);

        table.setItem(i, 0, rootItem);
        table.setItem(i, 1, wordItem);
        table.setItem(i, 2, schemeItem);
        table.setItem(i, 3, freqItem);
      }

      // Re-enable sorting and automatically sort by Frequency (Column 3) Descending
      table.setSortingEnabled(true);
      table.sortItems(3, Qt::DescendingOrder);

      layout.addWidget(&table);

      QPushButton* closeBtn = new QPushButton(t("Close", "Fermer", "إغلاق"));
      connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
      layout.addWidget(closeBtn);

      dialog.exec();
    }

  public:
    CorpusTab(AVLTree<std::string>* tree, SchemeHashTable* schemes, QWidget* parent = nullptr) 
      : QWidget(parent), m_tree(tree), m_schemes(schemes) 
    {
      QVBoxLayout* layout = new QVBoxLayout(this);
      layout->setContentsMargins(10, 10, 10, 10);

      descLabel = new QLabel();
      descLabel->setWordWrap(true);
      descLabel->setAlignment(Qt::AlignCenter);

      analyzeBtn = new QPushButton();
      analyzeBtn->setMinimumHeight(50);

      connect(analyzeBtn, &QPushButton::clicked, this, &CorpusTab::onAnalyzeCorpus);

      layout->addStretch();
      layout->addWidget(descLabel);
      layout->addWidget(analyzeBtn);
      layout->addStretch();

      retranslateUi();
    }

    public slots:
      void setLanguage(int langIndex) {
        currentLang = langIndex;
        Qt::LayoutDirection dir = (langIndex == 2) ? Qt::RightToLeft : Qt::LeftToRight;
        this->setLayoutDirection(dir);
        retranslateUi();
      }

signals:
    void dataModified();

    private slots:
      void onAnalyzeCorpus() {
        QString filter = t("Text Files (*.txt);;PDF Files (*.pdf)", 
            "Fichiers Texte (*.txt);;Fichiers PDF (*.pdf)", 
            "ملفات نصية (*.txt);;ملفات PDF (*.pdf)");

        QString fileName = QFileDialog::getOpenFileName(this, 
            t("Open Corpus File", "Ouvrir Fichier Corpus", "فتح ملف النصوص"), 
            "", filter);

        if (fileName.isEmpty()) return;

        // --- ASK FOR ANALYSIS MODE ---
        QMessageBox modeBox(this);
        modeBox.setWindowTitle(t("Analysis Mode", "Mode d'Analyse", "وضع التحليل"));
        modeBox.setText(t("How would you like to extract roots?", 
              "Comment souhaitez-vous extraire les racines ?", 
              "كيف ترغب في استخراج الجذور؟"));

        QPushButton* strictBtn = modeBox.addButton(t("Strict (Available Roots Only)", "Strict (Racines disponibles)", "صارم (الجذور المتاحة فقط)"), QMessageBox::ActionRole);
        QPushButton* bruteBtn = modeBox.addButton(t("Brute-Force (Discover New Roots)", "Force Brute (Découvrir nouvelles)", "شامل (اكتشاف جذور جديدة)"), QMessageBox::ActionRole);
        modeBox.addButton(QMessageBox::Cancel);

        modeBox.exec();

        if (modeBox.clickedButton() == modeBox.button(QMessageBox::Cancel)) {
          return; // User canceled the operation
        }

        bool strictMode = (modeBox.clickedButton() == strictBtn);

        // Run Analysis
        AnalysisReport report = corpusAnalyzer::analyzeFile(fileName.toStdString(), *m_tree, *m_schemes, strictMode);

        if (!report.success) {
          QMessageBox::critical(this, 
              t("Error", "Erreur", "خطأ"), 
              QString::fromStdString(report.errorMessage));
          return;
        }

        // Show the interactive results dialog instead of the plain message box
        showAnalysisResultsDialog(report);

        // Tell the rest of the app that new roots might have been added
        if (report.newRootsFound > 0) {
          emit dataModified(); 
        }
      }
};

#endif
