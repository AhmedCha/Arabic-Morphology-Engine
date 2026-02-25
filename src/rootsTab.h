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
#include <QGroupBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QGraphicsItemGroup>
#include <QVariantAnimation>
#include <QEventLoop>
#include <QMap>
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
    bool isAnimating = false; // Prevent overlapping animations

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

    // Stats Panel Elements
    QGroupBox* statsGroup;
    QLabel* opsCountLabel;
    QLabel* rotCountLabel;

    // Store graphical representations by their node key
    QMap<QString, QGraphicsEllipseItem*> nodeEllipses;
    QMap<QString, QGraphicsTextItem*> nodeTexts;
    QGraphicsItemGroup* edgesGroup = nullptr; 

    QMap<QString, QPointF> startPositions;
    QMap<QString, QPointF> targetPositions;

    QString t(const QString& en, const QString& fr, const QString& ar) {
      if (currentLang == 1) return fr;
      if (currentLang == 2) return ar;
      return en;
    }

    AVLNode<std::string>* getSelectedNode() {
      auto items = scene->selectedItems();
      if (items.isEmpty()) return nullptr;
      std::string target = items.first()->data(0).toString().toStdString();
      AVLNode<std::string>* curr = m_tree->getRoot();
      while (curr) {
        if (target < curr->key) curr = curr->left;
        else if (target > curr->key) curr = curr->right;
        else return curr;
      }
      return nullptr;
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

      if (statsGroup) statsGroup->setTitle(t("Statistics", "Statistiques", "إحصائيات"));
      updateStats();
    }

    void updateStats() {
      if (!m_tree) return;
      opsCountLabel->setText(t("Operations:\n", "Opérations:\n", "العمليات:\n") + QString::number(m_tree->getOperationCount()));
      rotCountLabel->setText(t("Rotations:\n", "Rotations:\n", "الدورانات:\n") + QString::number(m_tree->getRotationCount()));
    }

    void setUiControlsEnabled(bool state) {
        addRootBtn->setEnabled(state);
        editRootBtn->setEnabled(state);
        deleteRootBtn->setEnabled(state);
        newRootInput->setEnabled(state);
    }

  public:
    RootsTab(AVLTree<std::string>* tree, QWidget* parent = nullptr) 
      : QWidget(parent), m_tree(tree) {
        
        // --- BIND AVL TREE CALLBACKS ---
        m_tree->updateStatsCb = [this]() { updateStats(); };
        m_tree->animateCb = [this]() { refreshTable(); };

        // OUTER LAYOUT
        QHBoxLayout* outerLayout = new QHBoxLayout(this);
        outerLayout->setContentsMargins(5, 5, 5, 5);

        QWidget* leftContainer = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(leftContainer);
        mainLayout->setContentsMargins(0, 0, 0, 0);

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

        QSplitter* splitter = new QSplitter(Qt::Vertical);

        scene = new QGraphicsScene(this);
        connect(scene, &QGraphicsScene::selectionChanged, this, &RootsTab::onNodeSelected);
        graphicsView = new ZoomableView(scene);
        splitter->addWidget(graphicsView);

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

        // --- RIGHT SIDE: Stats Panel ---
        QWidget* rightContainer = new QWidget();
        rightContainer->setFixedWidth(130); 
        QVBoxLayout* rightLayout = new QVBoxLayout(rightContainer);
        rightLayout->setContentsMargins(5, 0, 0, 0);

        statsGroup = new QGroupBox();
        QVBoxLayout* statsLayout = new QVBoxLayout(statsGroup);
        opsCountLabel = new QLabel("Operations:\n0");
        rotCountLabel = new QLabel("Rotations:\n0");

        QFont statFont("Arial", 11, QFont::Bold);
        opsCountLabel->setFont(statFont);
        opsCountLabel->setAlignment(Qt::AlignCenter);
        
        rotCountLabel->setFont(statFont);
        rotCountLabel->setAlignment(Qt::AlignCenter);

        statsLayout->addWidget(opsCountLabel);
        statsLayout->addSpacing(15);
        statsLayout->addWidget(rotCountLabel);
        statsLayout->addStretch(); 
        
        rightLayout->addWidget(statsGroup);

        outerLayout->addWidget(leftContainer, 1);
        outerLayout->addWidget(rightContainer);

        retranslateUi();
        refreshTable();
      }

    public slots:
      void setLanguage(int langIndex) {
        currentLang = langIndex;
        Qt::LayoutDirection dir = (langIndex == 2) ? Qt::RightToLeft : Qt::LeftToRight;
        this->setLayoutDirection(dir);
        familyTable->setLayoutDirection(dir);
        retranslateUi();

        if (AVLNode<std::string>* curr = getSelectedNode()) {
          populateTable(curr);
        }
      }

    void refreshTable() {
      if (isAnimating) return; // Prevent nested event loops if multiple triggers occur
      isAnimating = true;

      if (!edgesGroup) {
        edgesGroup = new QGraphicsItemGroup();
        scene->addItem(edgesGroup);
      }

      updateStats();

      // 1. Calculate the new layout mathematically
      targetPositions.clear();
      if (auto r = m_tree->getRoot()) {
        int height = r->height;
        double initial_hSpacing = 50.0 * std::pow(2.0, std::max(0, height - 2));
        calculateLayout(r, 0, 0, initial_hSpacing); 
      }

      // 2. Setup the smooth animation and EventLoop Block
      QEventLoop loop;
      QVariantAnimation* anim = new QVariantAnimation(this);
      anim->setDuration(600); // 600 milliseconds smooth transition
      anim->setStartValue(0.0);
      anim->setEndValue(1.0);
      anim->setEasingCurve(QEasingCurve::InOutQuad);

      connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
          qreal step = value.toReal();
          updateScenePositions(step);
          });

      connect(anim, &QVariantAnimation::finished, this, [this, &loop]() {
          cleanupDeletedNodes();
          drawEdges(); // Snap lines to final positions
          loop.quit(); // RESUME THE AVL TREE C++ CODE
          });

      anim->start(QAbstractAnimation::DeleteWhenStopped);
      loop.exec(); // This pauses the execution right here until the animation finishes

      scene->setSceneRect(scene->itemsBoundingRect().adjusted(-100, -100, 100, 100));
      isAnimating = false;
    }

    private slots:
      void calculateLayout(AVLNode<std::string>* node, double x, double y, double hSpacing) {
        if (!node) return;
        double vSpacing = 60.0; 

        QString key = QString::fromStdString(node->key);
        targetPositions[key] = QPointF(x, y);

        if (!nodeEllipses.contains(key)) {
          createGraphicsItemsForNode(key, x, y - 50); 
        }

        if (node->left) calculateLayout(node->left, x - hSpacing, y + vSpacing, hSpacing / 2.0);
        if (node->right) calculateLayout(node->right, x + hSpacing, y + vSpacing, hSpacing / 2.0);
      }

    void createGraphicsItemsForNode(const QString& key, double startX, double startY) {
      double nodeWidth = 90.0; 
      double nodeHeight = 55.0;

      QGraphicsEllipseItem* ellipse = scene->addEllipse(
          startX - nodeWidth / 2, startY - nodeHeight / 2, nodeWidth, nodeHeight,
          QPen(QColor("#2c3e50"), 2), QBrush(QColor("#27ae60"))
          );
      ellipse->setFlag(QGraphicsItem::ItemIsSelectable);
      ellipse->setData(0, key);
      ellipse->setZValue(1);

      QGraphicsTextItem* text = scene->addText("");
      text->setHtml("<div dir='rtl' style='text-align:center;'>" + key + "</div>");
      text->setFont(QFont("Arial", 13, QFont::Bold));
      text->setDefaultTextColor(Qt::white);
      text->setZValue(2);
      text->setAcceptedMouseButtons(Qt::NoButton);

      nodeEllipses[key] = ellipse;
      nodeTexts[key] = text;

      startPositions[key] = QPointF(startX, startY);
    }

    void updateScenePositions(qreal step) {
      double nodeWidth = 90.0; 
      double nodeHeight = 55.0;

      foreach(QGraphicsItem* item, edgesGroup->childItems()) {
        delete item;
      }

      for (auto it = nodeEllipses.begin(); it != nodeEllipses.end(); ++it) {
        QString key = it.key();
        QGraphicsEllipseItem* ellipse = it.value();
        QGraphicsTextItem* text = nodeTexts[key];

        QPointF start = startPositions.value(key, ellipse->sceneBoundingRect().center());
        QPointF end = targetPositions.value(key, start); 

        double currentX = start.x() + (end.x() - start.x()) * step;
        double currentY = start.y() + (end.y() - start.y()) * step;

        ellipse->setRect(currentX - nodeWidth / 2, currentY - nodeHeight / 2, nodeWidth, nodeHeight);

        QRectF tr = text->boundingRect();
        text->setPos(currentX - tr.width() / 2, currentY - tr.height() / 2);
      }
    }

    void drawEdges() {
      if (auto r = m_tree->getRoot()) {
        drawEdgesHelper(r);
      }
    }

    void drawEdgesHelper(AVLNode<std::string>* node) {
      if (!node) return;
      QString key = QString::fromStdString(node->key);
      QPointF pos = targetPositions[key];

      if (node->left) {
        QString leftKey = QString::fromStdString(node->left->key);
        QPointF leftPos = targetPositions[leftKey];
        QGraphicsLineItem* line = scene->addLine(pos.x(), pos.y(), leftPos.x(), leftPos.y(), QPen(QColor("#95a5a6"), 2));
        edgesGroup->addToGroup(line);
        line->setZValue(-1);
        drawEdgesHelper(node->left);
      }
      if (node->right) {
        QString rightKey = QString::fromStdString(node->right->key);
        QPointF rightPos = targetPositions[rightKey];
        QGraphicsLineItem* line = scene->addLine(pos.x(), pos.y(), rightPos.x(), rightPos.y(), QPen(QColor("#95a5a6"), 2));
        edgesGroup->addToGroup(line);
        line->setZValue(-1);
        drawEdgesHelper(node->right);
      }
    }

    void cleanupDeletedNodes() {
      QList<QString> toDelete;
      for (auto key : nodeEllipses.keys()) {
        if (!targetPositions.contains(key)) {
          toDelete.append(key);
        } else {
          startPositions[key] = targetPositions[key];
        }
      }

      for (auto key : toDelete) {
        delete nodeEllipses[key];
        delete nodeTexts[key];
        nodeEllipses.remove(key);
        nodeTexts.remove(key);
        startPositions.remove(key);
      }
    }

    // --- ROOT MANAGEMENT ---
    void onAddRoot() {
      QString input = newRootInput->text().trimmed();
      if (input.isEmpty()) return;

      if (input.length() != 3) {
        QMessageBox::warning(this, t("Error", "Erreur", "خطأ"), 
            t("Root must be 3 letters.", "La racine doit contenir 3 lettres.", "يجب أن يتكون الجذر من 3 أحرف."));
        return;
      }

      setUiControlsEnabled(false); // Lock UI while animating
      m_tree->insert(input.toStdString());
      refreshTable(); // Final snap
      setUiControlsEnabled(true); // Unlock UI

      newRootInput->clear();
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

        auto savedFamily = curr->derivedWords;
        std::string oldKey = curr->key; 
        
        setUiControlsEnabled(false);
        m_tree->remove(oldKey);
        m_tree->insert(newKey.toStdString());
        setUiControlsEnabled(true);

        AVLNode<std::string>* newCurr = m_tree->getRoot();
        std::string target = newKey.toStdString();
        while (newCurr) {
          if (target < newCurr->key) newCurr = newCurr->left;
          else if (target > newCurr->key) newCurr = newCurr->right;
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
        setUiControlsEnabled(false);
        m_tree->remove(curr->key);
        setUiControlsEnabled(true);
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
      const QChar RLM(0x200F); 

      for (size_t i = 0; i < curr->derivedWords.size(); ++i) {
        QString displayWord = RLM + QString::fromStdString(curr->derivedWords[i].word);
        QTableWidgetItem* wordItem = new QTableWidgetItem(displayWord);

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
