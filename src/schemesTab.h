#ifndef SCHEMESTAB_H
#define SCHEMESTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QCheckBox>

#include "schemeHashTable.h"
#include "morphologyEngine.h"

class SchemesTab : public QWidget {
  Q_OBJECT

  private:
    int currentLang = 0; // 0 = EN, 1 = FR, 2 = AR
    SchemeHashTable* m_schemes;

    // UI Elements
    QLineEdit* schemeNameInput;
    QPushButton* addBtn;
    QPushButton* editBtn;
    QPushButton* deleteBtn;
    QCheckBox* togglePatternCb;
    QTableWidget* schemesTable;

    // Translation Helper
    QString t(const QString& en, const QString& fr, const QString& ar) {
      if (currentLang == 1) return fr;
      if (currentLang == 2) return ar;
      return en;
    }

    void retranslateUi() {
      schemeNameInput->setPlaceholderText(t("Scheme Name (e.g. مفعول)", "Nom du schème (ex. مفعول)", "اسم الوزن (مثل: مفعول)"));
      addBtn->setText(t("Add / Auto-Generate", "Ajouter / Générer auto", "إضافة / توليد تلقائي"));
      editBtn->setText(t("Edit Selected", "Modifier", "تعديل المحدد"));
      deleteBtn->setText(t("Delete Selected", "Supprimer", "حذف المحدد"));

      togglePatternCb->setText(t("Show Derived Patterns", "Afficher les modèles dérivés", "إظهار الأنماط المشتقة"));

      schemesTable->setHorizontalHeaderLabels({
          t("Scheme Name", "Nom du Schème", "اسم الوزن"), 
          t("Derived Pattern", "Modèle Dérivé", "النمط المشتق")
          });
    }

  public:
    SchemesTab(SchemeHashTable* schemes, QWidget* parent = nullptr) 
      : QWidget(parent), m_schemes(schemes) 
    {
      QVBoxLayout* layout = new QVBoxLayout(this);
      layout->setContentsMargins(5, 5, 5, 5);

      // --- TOP CONTROLS (Add) ---
      QHBoxLayout* addLayout = new QHBoxLayout();
      schemeNameInput = new QLineEdit();
      addBtn = new QPushButton();

      connect(schemeNameInput, &QLineEdit::returnPressed, this, &SchemesTab::onAddScheme);
      connect(addBtn, &QPushButton::clicked, this, &SchemesTab::onAddScheme);

      addLayout->addWidget(schemeNameInput);
      addLayout->addWidget(addBtn);
      layout->addLayout(addLayout);

      // --- MIDDLE CONTROLS (Edit / Delete / Toggle) ---
      QHBoxLayout* actionLayout = new QHBoxLayout();
      editBtn = new QPushButton();
      deleteBtn = new QPushButton();
      togglePatternCb = new QCheckBox();
      // Notice we moved the connection and setChecked to the bottom!

      connect(editBtn, &QPushButton::clicked, this, &SchemesTab::onEditScheme);
      connect(deleteBtn, &QPushButton::clicked, this, &SchemesTab::onDeleteScheme);

      actionLayout->addWidget(togglePatternCb);
      actionLayout->addStretch(); 
      actionLayout->addWidget(editBtn);
      actionLayout->addWidget(deleteBtn);
      layout->addLayout(actionLayout);

      // --- BOTTOM TABLE ---
      schemesTable = new QTableWidget(0, 2); 
      schemesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
      schemesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
      schemesTable->setEditTriggers(QAbstractItemView::NoEditTriggers); 

      layout->addWidget(schemesTable);

      // --- SETUP TOGGLE BEHAVIOR (Safe to do now that table exists) ---
      connect(togglePatternCb, &QCheckBox::toggled, this, [this](bool checked){
          schemesTable->setColumnHidden(1, !checked); 
          });

      // This will automatically emit the toggled signal and hide column 1 on startup
      togglePatternCb->setChecked(false); 

      retranslateUi();
      refreshTable();
      schemesTable->setColumnHidden(1, true);
    }

    public slots:
      void setLanguage(int langIndex) {
        currentLang = langIndex;

        Qt::LayoutDirection dir = (langIndex == 2) ? Qt::RightToLeft : Qt::LeftToRight;
        this->setLayoutDirection(dir);
        schemesTable->setLayoutDirection(dir);

        retranslateUi();
        refreshTable(); 
      }

    void refreshTable() {
      schemesTable->setSortingEnabled(false);
      schemesTable->setRowCount(0);

      std::vector<Scheme> allSchemes = m_schemes->getAllSchemes();
      schemesTable->setRowCount(allSchemes.size());

      const QChar RLM(0x200F); 
      const QChar RLO(0x202E); 
      const QChar PDF(0x202C); 

      for (size_t i = 0; i < allSchemes.size(); ++i) {
        QString displayName = RLM + QString::fromStdString(allSchemes[i].name);
        QTableWidgetItem* nameItem = new QTableWidgetItem(displayName);
        nameItem->setTextAlignment(currentLang == 2 ? Qt::AlignRight | Qt::AlignVCenter : Qt::AlignLeft | Qt::AlignVCenter);
        schemesTable->setItem(i, 0, nameItem);

        std::string pattern = MorphologyEngine::derivePatternFromName(allSchemes[i].name);
        QString displayPattern = RLO + QString::fromStdString(pattern) + PDF;
        QTableWidgetItem* patternItem = new QTableWidgetItem(displayPattern);
        patternItem->setTextAlignment(Qt::AlignCenter);
        schemesTable->setItem(i, 1, patternItem);
      }

      schemesTable->setSortingEnabled(true);
    }

    private slots:
      void onAddScheme() {
        std::string name = schemeNameInput->text().trimmed().toStdString();
        if (!name.empty()) {
          std::string derivedPattern = MorphologyEngine::derivePatternFromName(name);
          m_schemes->insert(name, derivedPattern);
          schemeNameInput->clear();
          refreshTable();
        }
      }

    void onEditScheme() {
      int row = schemesTable->currentRow();
      if (row < 0) return;

      QString oldName = schemesTable->item(row, 0)->text().remove(QChar(0x200F));

      bool ok;
      QString newName = QInputDialog::getText(this, 
          t("Edit Scheme", "Modifier Schème", "تعديل الوزن"), 
          t("Modify Scheme Name:", "Modifier le nom du schème:", "تعديل اسم الوزن:"), 
          QLineEdit::Normal, oldName, &ok);

      newName = newName.trimmed();

      if (ok && !newName.isEmpty() && newName != oldName) {
        m_schemes->remove(oldName.toStdString()); 
        std::string derivedPattern = MorphologyEngine::derivePatternFromName(newName.toStdString());
        m_schemes->insert(newName.toStdString(), derivedPattern);
        refreshTable();
      }
    }

    void onDeleteScheme() {
      int row = schemesTable->currentRow();
      if (row < 0) return;

      QString targetName = schemesTable->item(row, 0)->text().remove(QChar(0x200F));

      auto reply = QMessageBox::question(this, 
          t("Confirm Delete", "Confirmer la suppression", "تأكيد الحذف"), 
          t("Are you sure you want to delete the scheme '", "Voulez-vous vraiment supprimer le schème '", "هل أنت متأكد من حذف الوزن '") + targetName + "'?", 
          QMessageBox::Yes | QMessageBox::No);

      if (reply == QMessageBox::Yes) {
        m_schemes->remove(targetName.toStdString());
        refreshTable();
      }
    }
};

#endif
