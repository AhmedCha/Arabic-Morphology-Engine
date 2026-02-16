#ifndef ROOTSTAB_H
#define ROOTSTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QSplitter>
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QPen>
#include <QBrush>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QScrollBar> 
#include <QInputDialog>
#include <QMessageBox>
#include <cmath>
#include <algorithm>

#include "AVLTree.h"

// --- ZoomableView ---
class ZoomableView : public QGraphicsView {
  public:
    ZoomableView(QGraphicsScene* scene) : QGraphicsView(scene) {
      setRenderHint(QPainter::Antialiasing);
      setDragMode(QGraphicsView::ScrollHandDrag); 
      setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }
  protected:
    void wheelEvent(QWheelEvent* event) override {
      if (event->modifiers() & Qt::ControlModifier) {
        double scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) scale(scaleFactor, scaleFactor);
        else scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        event->accept();
      } else if (event->modifiers() & Qt::AltModifier) {
        QScrollBar* hBar = horizontalScrollBar();
        hBar->setValue(hBar->value() - event->angleDelta().y());
        event->accept();
      } else {
        QGraphicsView::wheelEvent(event);
      }
    }
    void keyPressEvent(QKeyEvent* event) override {
      if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) scale(1.1, 1.1);
      else if (event->key() == Qt::Key_Minus) scale(0.9, 0.9);
      else QGraphicsView::keyPressEvent(event);
    }
};

// --- RootsTab ---
class RootsTab : public QWidget {
  Q_OBJECT

  private:
    int currentLang = 0; // 0 = EN, 1 = FR, 2 = AR
    AVLTree<std::string>* m_tree;

    // UI Elements
    QLineEdit* newRootInput;
    QPushButton* addRootBtn;
    QPushButton* editRootBtn;
    QPushButton* deleteRootBtn;

    QLineEdit* newWordInput;
    QLineEdit* newFreqInput;
    QPushButton* addWordBtn;
    QPushButton* editWordBtn;
    QPushButton* deleteWordBtn;

    QGraphicsScene* scene;
    ZoomableView* graphicsView;
    QTableWidget* familyTable;

    // Translation Helper
    QString t(const QString& en, const QString& fr, const QString& ar) {
      if (currentLang == 1) return fr;
      if (currentLang == 2) return ar;
      return en;
    }

    // Helper to get selected node
    AVLNode<std::string>* getSelectedNode() {
      auto items = scene->selectedItems();
      if (items.isEmpty()) return nullptr;
      std::string target = items.first()->data(0).toString().toStdString();
      AVLNode<std::string>* curr = m_tree->getRoot();
      while (curr) {
        if (target < curr->key) curr = curr->left.get();
        else if (target > curr->key) curr = curr->right.get();
        else return curr;
      }
      return nullptr;
    }

    void drawNode(AVLNode<std::string>* node, double x, double y, double hSpacing) {
      if (!node) return;
      double vSpacing = 60.0; 
      double nodeWidth = 90.0; 
      double nodeHeight = 55.0;

      if (node->left) {
        scene->addLine(x, y, x - hSpacing, y + vSpacing, QPen(QColor("#95a5a6"), 2))->setZValue(-1);
        drawNode(node->left.get(), x - hSpacing, y + vSpacing, hSpacing / 2.0);
      }
      if (node->right) {
        scene->addLine(x, y, x + hSpacing, y + vSpacing, QPen(QColor("#95a5a6"), 2))->setZValue(-1);
        drawNode(node->right.get(), x + hSpacing, y + vSpacing, hSpacing / 2.0);
      }

      QGraphicsEllipseItem* ellipse = scene->addEllipse(
          x - nodeWidth / 2, y - nodeHeight / 2, nodeWidth, nodeHeight,
          QPen(QColor("#2c3e50"), 2), QBrush(QColor("#27ae60"))
          );
      ellipse->setFlag(QGraphicsItem::ItemIsSelectable);
      ellipse->setData(0, QString::fromStdString(node->key));
      ellipse->setZValue(1);

      QGraphicsTextItem* text = scene->addText("");
      // BiDi HTML trick for the canvas nodes to prevent scrambling
      text->setHtml("<div dir='rtl' style='text-align:center;'>" + QString::fromStdString(node->key) + "</div>");
      text->setFont(QFont("Arial", 13, QFont::Bold));
      text->setDefaultTextColor(Qt::white);
      text->setZValue(2);

      QRectF tr = text->boundingRect();
      text->setPos(x - tr.width() / 2, y - tr.height() / 2);
      text->setAcceptedMouseButtons(Qt::NoButton);
    }

    void retranslateUi() {
      newRootInput->setPlaceholderText(t("Enter 3-letter root...", "Entrez la racine...", "أدخل جذر من 3 أحرف..."));
      addRootBtn->setText(t("Add Root", "Ajouter", "إضافة جذر"));
      editRootBtn->setText(t("Edit Selected", "Modifier", "تعديل المحدد"));
      deleteRootBtn->setText(t("Delete Selected", "Supprimer", "حذف المحدد"));

      familyTable->setHorizontalHeaderLabels({
          t("Derived Word / Scheme", "Mot Dérivé / Schème", "الكلمة المشتقة / الوزن"), 
          t("Frequency", "Fréquence", "التكرار")
          });

      newWordInput->setPlaceholderText(t("New Word...", "Nouveau Mot...", "كلمة جديدة..."));
      newFreqInput->setPlaceholderText(t("Freq", "Fréq", "التكرار"));
      addWordBtn->setText(t("Add Word", "Ajouter", "إضافة كلمة"));
      editWordBtn->setText(t("Edit", "Modifier", "تعديل"));
      deleteWordBtn->setText(t("Delete", "Supprimer", "حذف"));
    }

  public:
    RootsTab(AVLTree<std::string>* tree, QWidget* parent = nullptr) 
      : QWidget(parent), m_tree(tree) {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);

        // -- TOP CONTROLS (Root Management) --
        QHBoxLayout* addLayout = new QHBoxLayout();
        newRootInput = new QLineEdit();
        addRootBtn = new QPushButton();
        editRootBtn = new QPushButton();
        deleteRootBtn = new QPushButton();

        connect(newRootInput, &QLineEdit::returnPressed, this, &RootsTab::onAddRoot);
        connect(addRootBtn, &QPushButton::clicked, this, &RootsTab::onAddRoot);
        connect(editRootBtn, &QPushButton::clicked, this, &RootsTab::onEditRoot);
        connect(deleteRootBtn, &QPushButton::clicked, this, &RootsTab::onDeleteRoot);

        addLayout->addWidget(newRootInput);
        addLayout->addWidget(addRootBtn);
        addLayout->addWidget(editRootBtn);
        addLayout->addWidget(deleteRootBtn);
        mainLayout->addLayout(addLayout);

        // -- SPLITTER --
        QSplitter* splitter = new QSplitter(Qt::Vertical);

        scene = new QGraphicsScene(this);
        connect(scene, &QGraphicsScene::selectionChanged, this, &RootsTab::onNodeSelected);
        graphicsView = new ZoomableView(scene);
        splitter->addWidget(graphicsView);

        // -- BOTTOM SECTION (Table + Scheme Management) --
        QWidget* bottomWidget = new QWidget();
        QVBoxLayout* bottomLayout = new QVBoxLayout(bottomWidget);
        bottomLayout->setContentsMargins(0,0,0,0);

        familyTable = new QTableWidget(0, 2);
        familyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        familyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        bottomLayout->addWidget(familyTable);

        QHBoxLayout* wordControlsLayout = new QHBoxLayout();
        newWordInput = new QLineEdit();
        newFreqInput = new QLineEdit();
        newFreqInput->setMaximumWidth(100);

        addWordBtn = new QPushButton();
        editWordBtn = new QPushButton();
        deleteWordBtn = new QPushButton();

        connect(newWordInput, &QLineEdit::returnPressed, this, &RootsTab::onAddWord);
        connect(newFreqInput, &QLineEdit::returnPressed, this, &RootsTab::onAddWord);
        connect(addWordBtn, &QPushButton::clicked, this, &RootsTab::onAddWord);
        connect(editWordBtn, &QPushButton::clicked, this, &RootsTab::onEditWord);
        connect(deleteWordBtn, &QPushButton::clicked, this, &RootsTab::onDeleteWord);

        wordControlsLayout->addWidget(newWordInput);
        wordControlsLayout->addWidget(newFreqInput);
        wordControlsLayout->addWidget(addWordBtn);
        wordControlsLayout->addWidget(editWordBtn);
        wordControlsLayout->addWidget(deleteWordBtn);
        bottomLayout->addLayout(wordControlsLayout);

        splitter->addWidget(bottomWidget);
        splitter->setSizes({600, 250});
        mainLayout->addWidget(splitter);

        retranslateUi();
        refreshTable();
      }

    public slots:
      void setLanguage(int langIndex) {
        currentLang = langIndex;

        // Flip the entire layout direction!
        Qt::LayoutDirection dir = (langIndex == 2) ? Qt::RightToLeft : Qt::LeftToRight;
        this->setLayoutDirection(dir);
        familyTable->setLayoutDirection(dir);

        retranslateUi();

        // Re-populate table so word alignments update instantly
        if (AVLNode<std::string>* curr = getSelectedNode()) {
          populateTable(curr);
        }
      }

    void refreshTable() {
      scene->clear();
      familyTable->setRowCount(0);

      if (auto r = m_tree->getRoot()) {
        int height = r->height;
        double initial_hSpacing = 50.0 * std::pow(2.0, std::max(0, height - 2));
        drawNode(r, 0, 0, initial_hSpacing); 
        scene->setSceneRect(scene->itemsBoundingRect().adjusted(-100, -100, 100, 100));
      }
    }

    private slots:
      // --- ROOT MANAGEMENT ---
      void onAddRoot() {
        QString input = newRootInput->text().trimmed();
        if (input.isEmpty()) return;

        if (input.length() != 3) {
          QMessageBox::warning(this, t("Error", "Erreur", "خطأ"), 
              t("Root must be 3 letters.", "La racine doit contenir 3 lettres.", "يجب أن يتكون الجذر من 3 أحرف."));
          return;
        }

        m_tree->insert(input.toStdString());
        newRootInput->clear();
        refreshTable();
      }

    void onEditRoot() {
      AVLNode<std::string>* curr = getSelectedNode();
      if (!curr) return;

      bool ok;
      QString newKey = QInputDialog::getText(this, 
          t("Edit Root", "Modifier Racine", "تعديل الجذر"), 
          t("Enter new root:", "Entrez la nouvelle racine:", "أدخل الجذر الجديد:"), 
          QLineEdit::Normal, QString::fromStdString(curr->key), &ok);

      newKey = newKey.trimmed();

      if (ok && !newKey.isEmpty() && newKey.toStdString() != curr->key) {
        if (newKey.length() != 3) {
          QMessageBox::warning(this, t("Error", "Erreur", "خطأ"), 
              t("Root must be 3 letters.", "La racine doit contenir 3 lettres.", "يجب أن يتكون الجذر من 3 أحرف."));
          return;
        }

        // Grab the derived words so they aren't lost
        auto savedFamily = curr->derivedWords;
        m_tree->remove(curr->key);
        m_tree->insert(newKey.toStdString());

        AVLNode<std::string>* newCurr = m_tree->getRoot();
        std::string target = newKey.toStdString();
        while (newCurr) {
          if (target < newCurr->key) newCurr = newCurr->left.get();
          else if (target > newCurr->key) newCurr = newCurr->right.get();
          else {
            newCurr->derivedWords = savedFamily;
            break;
          }
        }
        refreshTable();
      }
    }

    void onDeleteRoot() {
      AVLNode<std::string>* curr = getSelectedNode();
      if (!curr) return;

      auto reply = QMessageBox::question(this, 
          t("Confirm Delete", "Confirmer la suppression", "تأكيد الحذف"), 
          t("Delete this root?", "Supprimer cette racine?", "هل أنت متأكد من حذف هذا الجذر؟"), 
          QMessageBox::Yes | QMessageBox::No);

      if (reply == QMessageBox::Yes) {
        m_tree->remove(curr->key);
        refreshTable();
      }
    }

    // --- SCHEME / WORD MANAGEMENT ---
    void onNodeSelected() {
      populateTable(getSelectedNode());
    }

    void populateTable(AVLNode<std::string>* curr) {
      familyTable->setSortingEnabled(false); 
      familyTable->setRowCount(0);
      if (!curr) return;

      familyTable->setRowCount(curr->derivedWords.size());
      const QChar RLM(0x200F); // Bulletproof BiDi rendering

      for (size_t i = 0; i < curr->derivedWords.size(); ++i) {
        QString displayWord = RLM + QString::fromStdString(curr->derivedWords[i].word);
        QTableWidgetItem* wordItem = new QTableWidgetItem(displayWord);

        // Align text properly depending on language
        wordItem->setTextAlignment(currentLang == 2 ? Qt::AlignRight | Qt::AlignVCenter : Qt::AlignLeft | Qt::AlignVCenter);
        familyTable->setItem(i, 0, wordItem);

        QTableWidgetItem* freqItem = new QTableWidgetItem();
        freqItem->setData(Qt::EditRole, curr->derivedWords[i].frequency); 
        freqItem->setTextAlignment(Qt::AlignCenter);
        familyTable->setItem(i, 1, freqItem);
      }
      familyTable->setSortingEnabled(true);
      familyTable->sortByColumn(1, Qt::DescendingOrder);
    }

    void onAddWord() {
      AVLNode<std::string>* curr = getSelectedNode();
      if (!curr) {
        QMessageBox::warning(this, t("Select Root", "Sélectionnez une racine", "اختر جذراً"), 
            t("Please select a Root node first.", "Veuillez d'abord sélectionner un nœud.", "الرجاء تحديد جذر من الشجرة أولاً."));
        return;
      }

      QString word = newWordInput->text();
      int freq = newFreqInput->text().toInt();

      if (!word.isEmpty()) {
        curr->derivedWords.push_back({word.toStdString(), freq}); 
        newWordInput->clear();
        newFreqInput->clear();
        populateTable(curr); 
      }
    }

    void onEditWord() {
      AVLNode<std::string>* curr = getSelectedNode();
      int row = familyTable->currentRow();
      if (!curr || row < 0) return;

      // Clean the RLM out of the text if it's there before editing
      QString oldWord = familyTable->item(row, 0)->text().remove(QChar(0x200F));
      int oldFreq = familyTable->item(row, 1)->data(Qt::EditRole).toInt();

      bool ok;
      QString newWord = QInputDialog::getText(this, 
          t("Edit Word", "Modifier Mot", "تعديل الكلمة"), 
          t("Edit word:", "Modifier mot:", "تعديل الكلمة:"), 
          QLineEdit::Normal, oldWord, &ok);
      if (!ok || newWord.isEmpty()) return;

      int newFreq = QInputDialog::getInt(this, 
          t("Edit Frequency", "Modifier Fréquence", "تعديل التكرار"), 
          t("Edit frequency:", "Modifier fréquence:", "تعديل التكرار:"), 
          oldFreq, 0, 1000000, 1, &ok);
      if (!ok) return;

      for (auto& dw : curr->derivedWords) {
        if (dw.word == oldWord.toStdString()) {
          dw.word = newWord.toStdString();
          dw.frequency = newFreq;
          break;
        }
      }
      populateTable(curr);
    }

    void onDeleteWord() {
      AVLNode<std::string>* curr = getSelectedNode();
      int row = familyTable->currentRow();
      if (!curr || row < 0) return;

      QString targetWord = familyTable->item(row, 0)->text().remove(QChar(0x200F));

      auto& words = curr->derivedWords;
      words.erase(std::remove_if(words.begin(), words.end(), 
            [&](const auto& dw) { return dw.word == targetWord.toStdString(); }), 
          words.end());

      populateTable(curr);
    }
};

#endif
