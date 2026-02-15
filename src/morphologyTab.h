#ifndef MORPHOLOGYTAB_H
#define MORPHOLOGYTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QString>
#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>

#include "AVLTree.h"
#include "schemeHashTable.h"
#include "morphologyEngine.h"

class MorphologyTab : public QWidget {
  Q_OBJECT
  private:
    AVLTree<std::string>* m_tree;
    SchemeHashTable* m_schemes;

    int currentLang = 0; // 0 = EN, 1 = FR, 2 = AR

    // UI Elements
    QGroupBox* generateGroup;
    QLabel* rootLabel;
    QLineEdit* rootInputForFamily;
    QPushButton* generateBtn;

    QGroupBox* findGroup;
    QLabel* wordLabel;
    QLineEdit* wordInputForRootSearch;
    QPushButton* findBtn;

    // Custom Generation Elements
    QGroupBox* customGroup;
    QLabel* customRootLabel;
    QLineEdit* customRootInput;
    QPushButton* openSchemesBtn; // Renamed to reflect new behavior

    // Translation Helper
    QString t(const QString& en, const QString& fr, const QString& ar) {
      if (currentLang == 1) return fr;
      if (currentLang == 2) return ar;
      return en;
    }

    void retranslateUi() {
      // Generate Family
      generateGroup->setTitle(t("Generate Family (Database Root)", "Générer la Famille (Racine de la base)", "توليد العائلة (جذر من القاعدة)"));
      rootLabel->setText(t("Root:", "Racine:", "الجذر:"));
      rootInputForFamily->setPlaceholderText(t("Enter root (e.g. كتب)", "Entrez la racine (ex. كتب)", "أدخل الجذر (مثل: كتب)"));
      generateBtn->setText(t("Generate & Show", "Générer et Afficher", "توليد وعرض"));

      // Reverse Search
      findGroup->setTitle(t("Reverse Search (Find Root from Word)", "Recherche Inversée (Trouver la racine)", "البحث العكسي (إيجاد الجذر من الكلمة)"));
      wordLabel->setText(t("Word:", "Mot:", "الكلمة:"));
      wordInputForRootSearch->setPlaceholderText(t("Enter derived word (e.g. مكتوب)", "Entrez le mot dérivé (ex. مكتوب)", "أدخل الكلمة المشتقة (مثل: مكتوب)"));
      findBtn->setText(t("Find Root", "Trouver Racine", "إيجاد الجذر"));

      // Custom Generate
      customGroup->setTitle(t("Custom Derivation (Test any 3-letter root)", "Dérivation Personnalisée (Test racine 3 lettres)", "اشتقاق مخصص (تجربة أي جذر ثلاثي)"));
      customRootLabel->setText(t("Custom Root:", "Racine Perso:", "جذر مخصص:"));
      customRootInput->setPlaceholderText(t("Exactly 3 letters...", "Exactement 3 lettres...", "3 أحرف بالضبط..."));
      openSchemesBtn->setText(t("Select Schemes & Generate...", "Sélectionner Schèmes & Générer...", "تحديد الأوزان والتوليد..."));
    }

    // Helper to display the final popup with generated words
    // Helper to display the final popup with generated words
    void showResultsDialog(const QString& title, const std::vector<std::pair<QString, QString>>& results) {
      if (results.empty()) return;

      QDialog dialog(this);
      dialog.setWindowTitle(title);
      dialog.resize(450, 400);
      dialog.setLayoutDirection((currentLang == 2) ? Qt::RightToLeft : Qt::LeftToRight);

      QVBoxLayout layout(&dialog);
      QTableWidget table(results.size(), 2);
      table.setHorizontalHeaderLabels({t("Generated Word", "Mot Généré", "الكلمة المشتقة"), t("Scheme", "Schème", "الوزن")});
      table.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
      table.setEditTriggers(QAbstractItemView::NoEditTriggers);
      table.setSelectionBehavior(QAbstractItemView::SelectRows);

      // 1. MAKE SURE SORTING IS OFF WHILE INSERTING
      table.setSortingEnabled(false); 

      const QChar RLM(0x200F); 

      for (size_t i = 0; i < results.size(); ++i) {
        QTableWidgetItem* wordItem = new QTableWidgetItem(RLM + results[i].first);
        QTableWidgetItem* schemeItem = new QTableWidgetItem(RLM + results[i].second);

        wordItem->setTextAlignment(Qt::AlignCenter);
        schemeItem->setTextAlignment(Qt::AlignCenter);

        table.setItem(i, 0, wordItem);
        table.setItem(i, 1, schemeItem);
      }

      // 2. TURN SORTING ON AFTER EVERYTHING IS INSERTED
      table.setSortingEnabled(true);

      layout.addWidget(&table);

      QPushButton* closeBtn = new QPushButton(t("Close", "Fermer", "إغلاق"));
      connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
      layout.addWidget(closeBtn);

      dialog.exec();
    }

  public:
    MorphologyTab(AVLTree<std::string>* tree, SchemeHashTable* schemes, QWidget* parent = nullptr) 
      : QWidget(parent), m_tree(tree), m_schemes(schemes) 
    {
      QVBoxLayout* layout = new QVBoxLayout(this);
      layout->setContentsMargins(10, 10, 10, 10);

      // --- 1. Generation Group (Existing) ---
      generateGroup = new QGroupBox();
      QHBoxLayout* generateLayout = new QHBoxLayout(generateGroup);
      rootLabel = new QLabel();
      rootInputForFamily = new QLineEdit();
      generateBtn = new QPushButton();
      connect(rootInputForFamily, &QLineEdit::returnPressed, this, &MorphologyTab::onGenerateFamily);
      connect(generateBtn, &QPushButton::clicked, this, &MorphologyTab::onGenerateFamily);
      generateLayout->addWidget(rootLabel);
      generateLayout->addWidget(rootInputForFamily);
      generateLayout->addWidget(generateBtn);

      // --- 2. Custom Generation Group (UPDATED) ---
      customGroup = new QGroupBox();
      QHBoxLayout* customLayout = new QHBoxLayout(customGroup);
      customRootLabel = new QLabel();
      customRootInput = new QLineEdit();
      openSchemesBtn = new QPushButton();

      connect(customRootInput, &QLineEdit::returnPressed, this, &MorphologyTab::onOpenSchemesPopup);
      connect(openSchemesBtn, &QPushButton::clicked, this, &MorphologyTab::onOpenSchemesPopup);

      customLayout->addWidget(customRootLabel);
      customLayout->addWidget(customRootInput);
      customLayout->addWidget(openSchemesBtn);

      // --- 3. Search Group (Existing) ---
      findGroup = new QGroupBox();
      QHBoxLayout* findLayout = new QHBoxLayout(findGroup);
      wordLabel = new QLabel();
      wordInputForRootSearch = new QLineEdit();
      findBtn = new QPushButton();
      connect(wordInputForRootSearch, &QLineEdit::returnPressed, this, &MorphologyTab::onFindRoot);
      connect(findBtn, &QPushButton::clicked, this, &MorphologyTab::onFindRoot);
      findLayout->addWidget(wordLabel);
      findLayout->addWidget(wordInputForRootSearch);
      findLayout->addWidget(findBtn);

      // Add to main layout
      layout->addWidget(generateGroup);
      layout->addWidget(customGroup);
      layout->addWidget(findGroup);
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

    private slots:
      void onGenerateFamily() {
        std::string root = rootInputForFamily->text().trimmed().toStdString();
        if (root.empty()) return;

        int wordsGenerated = MorphologyEngine::generateFamily(root, *m_tree, *m_schemes);

        if (wordsGenerated == -1) {
          QMessageBox::warning(this, 
              t("Error", "Erreur", "خطأ"), 
              t("Root not found in the database.", "Racine introuvable dans la base de données.", "الجذر غير موجود في قاعدة البيانات."));
          return;
        }

        std::vector<std::pair<QString, QString>> results;
        std::vector<Scheme> allSchemes = m_schemes->getAllSchemes();

        for (const Scheme& s : allSchemes) {
          std::string pattern = MorphologyEngine::derivePatternFromName(s.name);
          std::string generatedWord = MorphologyEngine::applyScheme(root, pattern); 
          if (!generatedWord.empty()) {
            results.push_back({QString::fromStdString(generatedWord), QString::fromStdString(s.name)});
          }
        }

        QString title = t("Generated Family", "Famille Générée", "العائلة المشتقة");
        showResultsDialog(title, results);
      }

    void onOpenSchemesPopup() {
      QString customRoot = customRootInput->text().trimmed();
      std::string rootStr = MorphologyEngine::sanitize(customRoot.toStdString());

      // Use your engine's UTF8 splitter to perfectly validate 3 letters
      if (MorphologyEngine::splitUTF8(rootStr).size() != 3) {
        QMessageBox::warning(this, 
            t("Invalid Length", "Longueur Invalide", "طول غير صالح"), 
            t("The root must be exactly 3 letters long.", 
              "La racine doit comporter exactement 3 lettres.", 
              "يجب أن يتكون الجذر من 3 أحرف بالضبط."));
        return;
      }

      // --- Create the Selection Popup ---
      QDialog selectionDialog(this);
      selectionDialog.setWindowTitle(t("Select Schemes (Click header to sort)", "Sélectionner les Schèmes (Cliquez l'en-tête pour trier)", "تحديد الأوزان (انقر على العنوان للفرز)"));
      selectionDialog.resize(350, 450);
      selectionDialog.setLayoutDirection((currentLang == 2) ? Qt::RightToLeft : Qt::LeftToRight);

      QVBoxLayout* popupLayout = new QVBoxLayout(&selectionDialog);

      std::vector<Scheme> allSchemes = m_schemes->getAllSchemes();
      QTableWidget table(allSchemes.size(), 1);
      table.setHorizontalHeaderLabels({t("Scheme Name", "Nom du Schème", "اسم الوزن")});
      table.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
      table.setEditTriggers(QAbstractItemView::NoEditTriggers);
      table.setSelectionMode(QAbstractItemView::NoSelection);

      // Disable sorting temporarily while inserting items
      table.setSortingEnabled(false); 

      const QChar RLM(0x200F);

      for (size_t i = 0; i < allSchemes.size(); ++i) {
        QTableWidgetItem* item = new QTableWidgetItem(RLM + QString::fromStdString(allSchemes[i].name));

        // Make the item checkable
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Unchecked); 

        // Store the pattern invisibly so we don't have to re-derive it
        std::string pattern = MorphologyEngine::derivePatternFromName(allSchemes[i].name);
        item->setData(Qt::UserRole, QString::fromStdString(pattern)); 

        table.setItem(i, 0, item);
      }

      // Re-enable sorting so the user can click the header
      table.setSortingEnabled(true);
      popupLayout->addWidget(&table);

      QPushButton* generateFromPopupBtn = new QPushButton(t("Generate", "Générer", "توليد"));
      connect(generateFromPopupBtn, &QPushButton::clicked, &selectionDialog, &QDialog::accept);
      popupLayout->addWidget(generateFromPopupBtn);

      // If the user clicks "Generate" (Accepts the dialog)
      if (selectionDialog.exec() == QDialog::Accepted) {
        std::vector<std::pair<QString, QString>> results;

        // Loop through the table rows to find checked items
        for (int i = 0; i < table.rowCount(); ++i) {
          QTableWidgetItem* item = table.item(i, 0);
          if (item->checkState() == Qt::Checked) {
            QString schemeName = item->text().remove(RLM); // Clean the display text
            std::string pattern = item->data(Qt::UserRole).toString().toStdString();

            std::string generatedWord = MorphologyEngine::applyScheme(rootStr, pattern);
            results.push_back({QString::fromStdString(generatedWord), schemeName});
          }
        }

        if (results.empty()) {
          QMessageBox::information(this, 
              t("No Schemes Selected", "Aucun schème sélectionné", "لم يتم تحديد أوزان"), 
              t("Please check at least one scheme to generate words.", 
                "Veuillez cocher au moins un schème.", 
                "يرجى تحديد وزن واحد على الأقل."));
          return;
        }

        QString title = t("Custom Generation Results", "Résultats de Génération", "نتائج الاشتقاق المخصص");
        showResultsDialog(title, results);
      }
    }

    void onFindRoot() {
      std::string word = wordInputForRootSearch->text().trimmed().toStdString();
      if (word.empty()) return;

      RootSearchResult result = MorphologyEngine::findRoot(word, *m_tree, *m_schemes);

      if (result.found) {
        QString msg = t("Match Found!\n\nWord: %1\nRoot: %2\nScheme: %3", 
            "Correspondance Trouvée!\n\nMot: %1\nRacine: %2\nSchème: %3", 
            "تم العثور على تطابق!\n\nالكلمة: %1\nالجذر: %2\nالوزن: %3")
          .arg(QString::fromStdString(word), 
              QString::fromStdString(result.root), 
              QString::fromStdString(result.schemeName));

        QMessageBox::information(this, t("Root Found", "Racine Trouvée", "تم العثور على الجذر"), msg);
      } else {
        QMessageBox::information(this, 
            t("Not Found", "Introuvable", "غير موجود"), 
            t("No valid root found in the database.", "Aucune racine valide trouvée dans la base.", "لم يتم العثور على جذر صالح في قاعدة البيانات."));
      }
    }
};

#endif
