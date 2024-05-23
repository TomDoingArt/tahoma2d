#include "vectorinspector.h"
#include <multicolumnsortproxymodel.h>

#include "tapp.h"

#include "toonz/txshlevelhandle.h"
#include "toonz/txshleveltypes.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/txshsimplelevel.h"
#include "toonz/tframehandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/txshcell.h"

#include "tvectorimage.h"
#include "toonz/tvectorimageutils.h"
#include "tstroke.h"
#include "toonz/tobjecthandle.h"
#include "toonzqt/selection.h"
#include "toonzqt/tselectionhandle.h"
#include "tools/tool.h"
#include "tools/toolhandle.h"
#include "tools/strokeselection.h"
#include "../tnztools/vectorselectiontool.h"

#include <QApplication>
#include <QPainter>
#include <QScrollBar>
#include <QFrame>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QDebug>

#include <QMutex>

#include <QtWidgets>

//-----------------------------------------------------------------------------
VectorInspectorFrame::VectorInspectorFrame(QScrollArea *parent,
                                           Qt::WindowFlags flags)
    : QFrame(parent, flags), m_scrollArea(parent) {
  setObjectName("VectorInspectorFrame");
  setFrameStyle(QFrame::StyledPanel);

  setFocusPolicy(Qt::StrongFocus); /*-- Keyboard Tab Focus --*/

  setFixedHeight(parentWidget()->height());
  setMinimumWidth(std::max(parentWidget()->width(), 800));
}

//------------------------------------------------------------------------------

VectorInspectorPanel::VectorInspectorPanel(QWidget *parent,
                                           Qt::WindowFlags flags)
    : QWidget(parent) {

  initiatedByVectorInspector = false;
  initiatedBySelectTool = false;
  strokeOrderChangedInProgress = false;
  selectingRowsForStroke = false;

  m_field = new QLabel("I am a test value for the QLabel");

  m_field->setFixedHeight(16);

  std::vector<int> m_selectedStrokeIndexes = {};

  setFocusProxy(m_field);

  proxyModel = new MultiColumnSortProxyModel;

  proxyView = new QTreeView;
  proxyView->setRootIsDecorated(false);
  proxyView->setAlternatingRowColors(true);
  proxyView->setModel(proxyModel);
  proxyView->setSortingEnabled(true);

  proxyView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  proxyView->setSelectionBehavior(QAbstractItemView::SelectRows);

  proxyView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(proxyView, &QWidget::customContextMenuRequested, this,
          &VectorInspectorPanel::showContextMenu);

//  sortCaseSensitivityCheckBox   = new QCheckBox(tr("Case sensitive sorting"));
//  filterCaseSensitivityCheckBox = new QCheckBox(tr("Case sensitive filter"));

  filterPatternLineEdit = new QLineEdit;
  filterPatternLabel    = new QLabel(tr("&Filter pattern (example 0|1):"));
  filterPatternLabel->setBuddy(filterPatternLineEdit);

  filterSyntaxComboBox = new QComboBox;
  filterSyntaxComboBox->addItem(tr("Regular expression"), QRegExp::RegExp);
  filterSyntaxComboBox->addItem(tr("Wildcard"), QRegExp::Wildcard);
  filterSyntaxComboBox->addItem(tr("Fixed string"), QRegExp::FixedString);
  filterSyntaxLabel = new QLabel(tr("Filter &syntax:"));
  filterSyntaxLabel->setBuddy(filterSyntaxComboBox);

  filterColumnComboBox = new QComboBox;
  filterColumnComboBox->addItem(tr("#"));
  filterColumnComboBox->addItem(tr("Group Id"));
  filterColumnComboBox->addItem(tr("Id"));
  filterColumnComboBox->addItem(tr("StyleId"));
  filterColumnComboBox->addItem(tr("Self Loop"));
  filterColumnComboBox->addItem(tr("Quad"));
  filterColumnComboBox->addItem(tr("P"));
  filterColumnComboBox->addItem(tr("x"));
  filterColumnComboBox->addItem(tr("y"));
  filterColumnComboBox->addItem(tr("Thickness"));

  filterColumnLabel = new QLabel(tr("Filter &column:"));
  filterColumnLabel->setBuddy(filterColumnComboBox);

  connect(filterPatternLineEdit, &QLineEdit::textChanged, this,
          &VectorInspectorPanel::filterRegExpChanged);
  connect(filterSyntaxComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &VectorInspectorPanel::filterRegExpChanged);
  connect(filterColumnComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &VectorInspectorPanel::filterColumnChanged);
  //connect(filterCaseSensitivityCheckBox, &QAbstractButton::toggled, this,
  //        &VectorInspectorPanel::filterRegExpChanged);
  //connect(sortCaseSensitivityCheckBox, &QAbstractButton::toggled, this,
  //        &VectorInspectorPanel::sortChanged);

  connect(proxyView, &QTreeView::clicked, this, &VectorInspectorPanel::onSelectedStroke);

  connect(proxyView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &VectorInspectorPanel::onVectorInspectorSelectionChanged);

  proxyGroupBox = new QGroupBox();
  proxyGroupBox->setMinimumSize(600, 400);

  QGridLayout *proxyLayout = new QGridLayout;
  proxyLayout->addWidget(proxyView, 0, 0, 1, 3);
  proxyLayout->addWidget(filterPatternLabel, 1, 0);
  proxyLayout->addWidget(filterPatternLineEdit, 1, 1, 1, 2);
  proxyLayout->addWidget(filterSyntaxLabel, 2, 0);
  proxyLayout->addWidget(filterSyntaxComboBox, 2, 1, 1, 2);
  proxyLayout->addWidget(filterColumnLabel, 3, 0);
  proxyLayout->addWidget(filterColumnComboBox, 3, 1, 1, 2);
  //proxyLayout->addWidget(filterCaseSensitivityCheckBox, 4, 0, 1, 2);
  //proxyLayout->addWidget(sortCaseSensitivityCheckBox, 4, 2);
  proxyGroupBox->setLayout(proxyLayout);

  QVBoxLayout *mainLayout = new QVBoxLayout;

  mainLayout->addWidget(m_field);
  mainLayout->addWidget(proxyGroupBox);

  setLayout(mainLayout);

  setWindowTitle(tr("Basic Sort/Filter Model"));

  proxyView->sortByColumn(0, Qt::AscendingOrder);
  filterColumnComboBox->setCurrentIndex(
      6);  // position 6 for the 'P' column of the Vector Inspector QTreeView
  filterPatternLineEdit->setText(
      "2");  // set default filter here, like: "0|1" to filter on value 0 or 1
  //filterCaseSensitivityCheckBox->setChecked(true);
  //sortCaseSensitivityCheckBox->setChecked(true);
}

//-----------------------------------------------------------------------------

void VectorInspectorPanel::onLevelSwitched() {
  // Get the current UTC date and time
  QDateTime utcNow = QDateTime::currentDateTimeUtc();

  // Get timestamp in milliseconds
  qint64 timestampMilliseconds = utcNow.toMSecsSinceEpoch();

  std::string levelType = "";

  switch (TApp::instance()->getCurrentImageType()) {
  case TImage::MESH:
    levelType = "Mesh";
    break;
  case TImage::VECTOR:
    levelType = "Vector";
    break;
  case TImage::TOONZ_RASTER:
    levelType = "ToonzRaster";
    break;
  case TImage::RASTER:
    levelType = "Raster";
    break;
  default:
    levelType = "Unknown";
  }

  //std::cout << "---------VectorInspectorPanel onLevelSwitched.levelType:" << levelType << std::endl;

  if (levelType == "Vector") {
    // ----------------------- the new stuff ----------------------------------

    // to prevent multiple connections, try to disconnect first in case
    // already connected previously.

    QObject::disconnect(TApp::instance()->getCurrentSelection(), &TSelectionHandle::selectionChanged, this, &VectorInspectorPanel::onSelectionChanged);
    QObject::connect(TApp::instance()->getCurrentSelection(), &TSelectionHandle::selectionChanged, this, &VectorInspectorPanel::onSelectionChanged);

    QObject::disconnect(TApp::instance()->getCurrentTool(), &ToolHandle::toolSwitched,
               this, &VectorInspectorPanel::onToolSwitched); 
    QObject::connect(TApp::instance()->getCurrentTool(), &ToolHandle::toolSwitched, this,
            &VectorInspectorPanel::onToolSwitched);

    QObject::disconnect(TApp::instance()->getCurrentScene(), &TSceneHandle::sceneSwitched,
      this, &VectorInspectorPanel::onSceneChanged);
    QObject::connect(TApp::instance()->getCurrentScene(), &TSceneHandle::sceneSwitched,
            this, &VectorInspectorPanel::onSceneChanged);

    QObject::connect(TApp::instance()->getCurrentScene(), &TSceneHandle::sceneChanged,
      this, &VectorInspectorPanel::onSceneChanged);
    QObject::connect(TApp::instance()->getCurrentScene(), &TSceneHandle::sceneChanged,
            this, &VectorInspectorPanel::onSceneChanged);

    TApp *app = TApp::instance();

    TXshLevelHandle *currentLevel = app->getCurrentLevel();

    TFrameHandle *currentFrame = app->getCurrentFrame();

    TColumnHandle *currentColumn = app->getCurrentColumn();

    int frameIndex = currentFrame->getFrameIndex();

    TXshSimpleLevel *currentSimpleLevel = currentLevel->getSimpleLevel();

    int row = currentFrame->getFrame();
    int col = currentColumn->getColumnIndex();

    //std::cout << "row:" << row << ", column:" << col << std::endl;

    TXsheet *xsheet = app->getCurrentXsheet()->getXsheet();

    const TXshCell cell = xsheet->getCell(row, col);

    TVectorImageP vectorImage = cell.getImage(false);
    m_vectorImage             = vectorImage.getPointer();

    UINT strokeCount = vectorImage->getStrokeCount();

    if (vectorImage) {
      //std::cout << "+++ --- +++ onLevelSwitched() stroke count:" << strokeCount
      //          << " = " << m_vectorImage->getStrokeCount() << std::endl;

      QObject::disconnect(m_vectorImage, &TVectorImage::changedStrokes, this,
                          &VectorInspectorPanel::onChangedStrokes);

      QObject::connect(m_vectorImage, &TVectorImage::changedStrokes, this,
                       &VectorInspectorPanel::onChangedStrokes);

      //std::cout << "+++ --- +++ connected changedStrokes() event to vectorImage"
      //          << std::endl;

      QObject::disconnect(m_vectorImage, &TVectorImage::enteredGroup, this,
                          &VectorInspectorPanel::onEnteredGroup);

      QObject::connect(m_vectorImage, &TVectorImage::enteredGroup, this,
                       &VectorInspectorPanel::onEnteredGroup);

      //std::cout << "+++ --- +++ connected enteredGroup() event to vectorImage"
      //          << std::endl;

      QObject::disconnect(m_vectorImage, &TVectorImage::exitedGroup, this,
                          &VectorInspectorPanel::onExitedGroup);

      QObject::connect(m_vectorImage, &TVectorImage::exitedGroup, this,
                       &VectorInspectorPanel::onExitedGroup);

      //std::cout << "+++ --- +++ connected exitedGroup() event to vectorImage"
      //          << std::endl;

      QObject::disconnect(m_vectorImage, &TVectorImage::changedStrokeOrder,
                          this, &VectorInspectorPanel::onStrokeOrderChanged);

      QObject::connect(m_vectorImage, &TVectorImage::changedStrokeOrder, this,
                       &VectorInspectorPanel::onStrokeOrderChanged);

      QObject::disconnect(m_vectorImage, &TVectorImage::selectedAllStrokes,
                          this, &VectorInspectorPanel::onSelectedAllStrokes);

      QObject::connect(m_vectorImage, &TVectorImage::selectedAllStrokes, this,
                       &VectorInspectorPanel::onSelectedAllStrokes);

      //std::cout
      //    << "+++ --- +++ connected to vectorImage changedStrokeOrder event"
      //    << std::endl;

      //std::cout << "------------------ VectorInspectorPanel calling "
      //             "getStrokeListData for strokeCount:"
      //          << strokeCount << std::endl;

      QStandardItemModel *model =
          m_vectorImage->getStrokeListData(parentWidget());
      setSourceModel(model);
      proxyModel->invalidate();

      // Autosizing columns after the model is populated
      for (int column = 1; column < model->columnCount(); ++column) {
        proxyView->resizeColumnToContents(column);
      }
      proxyView->setColumnWidth(0, 30);  // Stroke column width

    }

  } else {

    // disconnect all the signals which only apply when the current level is Vector
    QObject::disconnect(TApp::instance()->getCurrentSelection(), &TSelectionHandle::selectionChanged, this, &VectorInspectorPanel::onSelectionChanged);

    QObject::disconnect(TApp::instance()->getCurrentTool(), &ToolHandle::toolSwitched,
      this, &VectorInspectorPanel::onToolSwitched);

    QObject::disconnect(TApp::instance()->getCurrentScene(), &TSceneHandle::sceneSwitched,
      this, &VectorInspectorPanel::onSceneChanged);

    QObject::disconnect(TApp::instance()->getCurrentScene(), &TSceneHandle::sceneChanged,
      this, &VectorInspectorPanel::onSceneChanged);
    
    setSourceModel(new MultiColumnSortProxyModel);

  }

  //qDebug() << "Level type: " << QString::fromStdString(levelType)
  //  << " at: " << utcNow.toString(Qt::ISODate);

  m_field->setText("Level type: " + QString::fromStdString(levelType)
    + " at: " + utcNow.toString(Qt::ISODate));

  m_field->adjustSize();

  //filterColumnComboBox->setWindowTitle(
  //    utcNow.toString(Qt::ISODate) +
  //    " Level type:" + QString::fromStdString(levelType));

  //std::cout << "11,";
  update();
  //std::cout << "12,";
}

//-------------------------------------------------------------------------------------------------
void VectorInspectorPanel::onSelectionSwitched(TSelection *selectionFrom,
                                               TSelection *selectionTo) {
  //std::cout
  //    << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString()
  //    << " ------- VectorInspectorPanel::onSelectionSwitched() -----"
  //    << std::endl;
}

//-------------------------------------------------------------------------------------------------
void VectorInspectorPanel::setRowHighlighting() {
  //std::cout << " ------- VectorInspectorPanel::setRowHighlighting()------- "
  //          << std::endl;
  disconnect(proxyView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &VectorInspectorPanel::onVectorInspectorSelectionChanged);
  QItemSelectionModel *selectionModel = proxyView->selectionModel();

//  if (!selectingRowsForStroke) {
    // Clear the previous selection
    selectionModel->clearSelection();
//  }

  m_selectedStrokeIndexes = {};

  QModelIndex index =
      proxyModel->index(0, 0);  // the first column of the first row

  ToolHandle *currentTool = TApp::instance()->getCurrentTool();

  // std::cout << "------- Tool Name:" << currentTool->getTool()->getName()
  //  << std::endl;  // Tool Name:T_Selection

  // std::cout << "------- Tool Type:" << currentTool->getTool()->getToolType()
  //  << std::endl;  // Tool Type:8

  if ("T_Selection" == currentTool->getTool()->getName()) {
    StrokeSelection *strokeSelection =
        dynamic_cast<StrokeSelection *>(currentTool->getTool()->getSelection());

    if (strokeSelection) {
      //std::cout << "strokeSelection.isEmpty():" << strokeSelection->isEmpty();

      if (strokeSelection) {
        // initialize the lookup value list.
        for (int i = 0; i < (int)proxyModel->rowCount(); i++) {
          if (strokeSelection->isSelected(i)) {
            //std::cout << "strokeSelection.isSelected stroke index:" << i
            //          << std::endl;
            index = proxyModel->index(i, 0);
            m_selectedStrokeIndexes.push_back(i);
          }
        }
        // now if selected rows then set them in the QTreeView
        if (m_selectedStrokeIndexes.size() > 0) {
          for (int i = 0; i < (int)proxyModel->rowCount(); i++) {
            if (isSelected(proxyModel->index(i, 0).data().toInt())) {
              index = proxyModel->index(i, 0);
              selectionModel->select(index, QItemSelectionModel::Select |
                                                QItemSelectionModel::Rows);
            }
          }
        } else {
          selectionModel->clearSelection();
        }
      }
    }
    //std::cout << "setRowHighlighting, done with selecting rows in the QTreeView"
    //          << std::endl;
    // scroll to show selected rows
    QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

    if (!selectedIndexes.isEmpty()) {
      QModelIndex firstSelectedIndex = selectedIndexes.first();
      proxyView->scrollTo(firstSelectedIndex, QAbstractItemView::PositionAtTop);
    }
  }
  connect(proxyView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &VectorInspectorPanel::onVectorInspectorSelectionChanged);
}

//-------------------------------------------------------------------------------------------------

bool VectorInspectorPanel::isSelected(int index) const {
  return (std::find(m_selectedStrokeIndexes.begin(), m_selectedStrokeIndexes.end(), index) !=
    m_selectedStrokeIndexes.end());
}

//-------------------------------------------------------------------------------------------------

void VectorInspectorPanel::onSelectionChanged() {
  //std::cout
  //    << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString()
  //    << " ------- VectorInspectorPanel::onSelectionChanged() -----"
  //    << std::endl;

  if (initiatedByVectorInspector) {
    //std::cout << "- - initiatedByVectorInspector so no need to update Vector Inspector selection - -" << std::endl;
  }else{
    //std::cout << "- - set initiatedBySelectTool to true - -" << std::endl;
    initiatedBySelectTool = true;

    setRowHighlighting();
    
    //std::cout << "- - set initiatedBySelectTool to false - -" << std::endl;
    initiatedBySelectTool = false;
  }
}

//-----------------------------------------------------------------------------
void VectorInspectorPanel::showEvent(QShowEvent *) {

  QObject::connect(TApp::instance()->getCurrentLevel(), &TXshLevelHandle::xshLevelSwitched, this, &VectorInspectorPanel::onLevelSwitched);

  QObject::connect(TApp::instance()->getCurrentFrame(), &TFrameHandle::frameSwitched, this, &VectorInspectorPanel::onLevelSwitched);

  QObject::connect(TApp::instance()->getCurrentSelection(), &TSelectionHandle::selectionSwitched, this, &VectorInspectorPanel::onSelectionSwitched);

  onLevelSwitched();
  onSelectionChanged();
}

//-----------------------------------------------------------------------------

void VectorInspectorPanel::hideEvent(QHideEvent *) {

  QObject::disconnect(TApp::instance()->getCurrentLevel(), &TXshLevelHandle::xshLevelSwitched, this, &VectorInspectorPanel::onLevelSwitched);

  QObject::disconnect(TApp::instance()->getCurrentFrame(), &TFrameHandle::frameSwitched, this, &VectorInspectorPanel::onLevelSwitched);

  QObject::disconnect(TApp::instance()->getCurrentSelection(), &TSelectionHandle::selectionSwitched, this, &VectorInspectorPanel::onSelectionSwitched);

}

void VectorInspectorPanel::setSourceModel(QAbstractItemModel *model) {
  proxyModel->setSourceModel(model);
}

void VectorInspectorPanel::filterRegExpChanged() {
  QRegExp::PatternSyntax syntax = QRegExp::PatternSyntax(
      filterSyntaxComboBox->itemData(filterSyntaxComboBox->currentIndex())
          .toInt());
  //Qt::CaseSensitivity caseSensitivity =
  //    filterCaseSensitivityCheckBox->isChecked() ? Qt::CaseSensitive
  //                                               : Qt::CaseInsensitive;

  QRegExp regExp(filterPatternLineEdit->text(), Qt::CaseInsensitive, syntax);
  proxyModel->setFilterRegExp(regExp);
  setRowHighlighting();
}

void VectorInspectorPanel::filterColumnChanged() {
  proxyModel->setFilterKeyColumn(filterColumnComboBox->currentIndex());
  //setRowHighlighting();
}

void VectorInspectorPanel::sortChanged() {
  //proxyModel->setSortCaseSensitivity(sortCaseSensitivityCheckBox->isChecked()
                                         //? Qt::CaseSensitive
                                         //: Qt::CaseInsensitive);
}

void VectorInspectorPanel::copySelectedItemsToClipboard() {
  QModelIndexList indexes = proxyView->selectionModel()->selectedIndexes();

  QString selectedText;
  QMap<int, QString> rows;  // Use a map to sort by row number automatically

  // Collect data from each selected cell, grouped by row
  for (const QModelIndex &index : qAsConst(indexes)) {
    rows[index.row()] +=
        index.data(Qt::DisplayRole).toString() + "\t";  // Tab-separated values
  }

  // Concatenate rows into a single string, each row separated by a newline
  for (const QString &row : qAsConst(rows)) {
    selectedText += row.trimmed() + "\n";  // Remove trailing tab
  }

  // Copy the collected data to the clipboard
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(selectedText);
}

void VectorInspectorPanel::contextMenuEvent(QContextMenuEvent *event) {
  QMenu menu(this);
  QAction *copyAction = menu.addAction(tr("Copy"));
  connect(copyAction, &QAction::triggered, this,
          &VectorInspectorPanel::copySelectedItemsToClipboard);

  // Show the menu at the cursor position
  menu.exec(event->globalPos());
}

void VectorInspectorPanel::showContextMenu(const QPoint &pos) {
  QPoint globalPos = proxyView->viewport()->mapToGlobal(pos);
  QMenu menu;
QAction* copyAction = menu.addAction(tr("Copy"));
connect(copyAction, &QAction::triggered, this,
  &VectorInspectorPanel::copySelectedItemsToClipboard);

menu.exec(globalPos);
}

void VectorInspectorPanel::onEnteredGroup() {
  //std::cout << " - - - - - - - - - VectorInspectorPanel::onEnteredGroup() - - - - - - - - - - - - "
  //  "- - - - "
  //  << std::endl;
  QStandardItemModel* model = m_vectorImage->getStrokeListData(parentWidget());
  setSourceModel(model);
  proxyView->selectionModel()->clearSelection();
  //onSelectionChanged();
  //setRowHighlighting();
}

void VectorInspectorPanel::onExitedGroup() {
  //std::cout
  //  << " - - - - - - - - - VectorInspectorPanel::onExitedGroup() - - - - - - - - - - - - - - - - "
  //  << std::endl;
  QStandardItemModel* model = m_vectorImage->getStrokeListData(parentWidget());
  setSourceModel(model);
  //proxyView->selectionModel()->clearSelection();
  //onSelectionChanged();
  setRowHighlighting();
}

void VectorInspectorPanel::onChangedStrokes() {
  //std::cout
  //  << " - - - - - - - - - VectorInspectorPanel::onChangedStrokes() - - - - - - - - - - - - - - - - "
  //  << std::endl;
  QStandardItemModel* model = m_vectorImage->getStrokeListData(parentWidget());
  setSourceModel(model);
  setRowHighlighting();
}

void VectorInspectorPanel::onToolEditingFinished() {
  //std::cout
  //  << " - - - - - - - - - VectorInspectorPanel::onToolEditingFinished() - - - - - - - - - - - - - - - - "
  //  << std::endl;
  //setRowHighlighting();
}

void VectorInspectorPanel::onSceneChanged() {
  //std::cout
  //  << " - - - - - - - - - VectorInspectorPanel::onSceneChanged() - - - - - - - - - - - - - - - - "
  //  << std::endl;
}

void VectorInspectorPanel::onStrokeOrderChanged(int fromIndex, int count,
  int moveBefore, bool regroup) {
  //std::cout << " - - - - - - - - - VectorInspectorPanel::onStrokeOrderChanged() start - "
  //  "- - - - - - - - - - - - - - - "
  //  << std::to_string(fromIndex) << ", " << std::to_string(count)
  //  << ", " << std::to_string(moveBefore) << ", "
  //  << std::to_string(regroup) << std::endl;

  strokeOrderChangedInProgress = true;
  bool isSortOrderAscending = true;

  ToolHandle* currentTool = TApp::instance()->getCurrentTool();
  StrokeSelection* strokeSelection =
    dynamic_cast<StrokeSelection*>(currentTool->getTool()->getSelection());

  strokeSelection->notifyView();

  QStandardItemModel* model = m_vectorImage->getStrokeListData(parentWidget());
  setSourceModel(model);

  std::vector<int> theIndexes = strokeSelection->getSelection();

  QItemSelectionModel* selectionModel = proxyView->selectionModel();

  selectionModel->clearSelection();

  Qt::SortOrder sortOrder = proxyView->header()->sortIndicatorOrder();

  if (sortOrder == Qt::AscendingOrder) {
    //std::cout << "Strokes are sorted in ascending order" << std::endl;
    isSortOrderAscending = true;
  }
  else if (sortOrder == Qt::DescendingOrder) {
    //std::cout << "Strokes are sorted in descending order" << std::endl;
    isSortOrderAscending = false;
  }

  QModelIndex index =
    proxyModel->index(0, 0);  // the first column of the first row

  int moveAmount = (fromIndex < moveBefore) ? moveBefore - 1 - fromIndex : moveBefore - fromIndex;

  //std::cout << "- - - - - moveAmount:" << moveAmount << ", theIndexes.size():" << theIndexes.size() << std::endl;

  int maxIndexValue = proxyModel->rowCount() - 1; //theIndexes.size() - 1;
  int minIndexValue = 0;
  int currentRowIndex = 0;

  // get the maximum stroke index value, assume sorted values, no gaps
  int lastRow = model->rowCount() - 1;
  QModelIndex firstIndex = model->index(0, 0);
  QModelIndex lastIndex = model->index(lastRow, 0);
  //if (lastIndex.isValid() && firstIndex.isValid()) {
  int maxStrokeIndex = std::max(model->data(firstIndex).toInt(), model->data(lastIndex).toInt());
  //std::cout << "first, last, max " << model->data(firstIndex).toInt() << ", " << model->data(lastIndex).toInt() << ", " << maxStrokeIndex;
  //}

  // iterate and create list of selected indexes
  m_selectedStrokeIndexes.clear();

  if (moveAmount >= 0){
    //std::cout << "-+-+-+-+ move toward FRONT, moveAmount:" << moveAmount
    //          << std::endl;
    for (auto it = theIndexes.rbegin(); it != theIndexes.rend(); ++it) {
      //std::cout << "it:" << *it << ", ";
      currentRowIndex = *it + moveAmount;
      
      // at the upper row limit?
      if (currentRowIndex > maxIndexValue) {
        currentRowIndex = maxIndexValue;
        maxIndexValue--;
      }
      m_selectedStrokeIndexes.push_back(currentRowIndex);
    }
  }
  else {
    //std::cout << "-+-+-+-+ move toward BACK, moveAmount:" << moveAmount
    //          << std::endl;
    for (auto it = theIndexes.begin(); it != theIndexes.end(); ++it) {
      //std::cout << "it:" << *it << ", ";

      currentRowIndex = *it + moveAmount;

      // at the lower row limit?
      if (currentRowIndex < minIndexValue) {
        currentRowIndex = minIndexValue;
        minIndexValue++;
      }
      m_selectedStrokeIndexes.push_back(currentRowIndex);
    }
  }
  
  // now set selected rows in the QTreeView
  for (int i = 0; i < (int)proxyModel->rowCount(); i++) {
    if (isSelected(proxyModel->index(i, 0).data().toInt())) {
      index = proxyModel->index(i, 0);
      selectionModel->select(
        index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
  }

  // scroll to show selected rows
  QModelIndexList selectedIndexes = selectionModel->selectedIndexes();

  if (!selectedIndexes.isEmpty()) {
    QModelIndex firstSelectedIndex = selectedIndexes.first();
    proxyView->scrollTo(firstSelectedIndex, QAbstractItemView::PositionAtTop);
  }

  strokeOrderChangedInProgress = false;

  //std::cout << " - - - - - - - - - VectorInspectorPanel::onStrokeOrderChanged() end - - - - - - - - -" << std::endl;
}

void VectorInspectorPanel::onToolSwitched() {
  //std::cout << " - - - - - - - - - VectorInspectorPanel::onToolSwitched() - - - - "
  //             "- - - - - - - - - - - - "
  //          << std::endl;

  QObject::disconnect(TApp::instance()->getCurrentTool(),
                      &ToolHandle::toolEditingFinished, this,
                      &VectorInspectorPanel::onToolEditingFinished);

  QObject::connect(TApp::instance()->getCurrentTool(),
                   &ToolHandle::toolEditingFinished, this,
                   &VectorInspectorPanel::onToolEditingFinished);

  setRowHighlighting();
}

void VectorInspectorPanel::onSelectedAllStrokes() {
  //std::cout << " - - - - - - - - - VectorInspectorPanel::onSelectedAllStrokes() "
  //             "Strokes signal - - - - - - - - - - - - - - - - "
  //          << std::endl;
  setRowHighlighting();
}

void VectorInspectorPanel::onSelectedStroke(const QModelIndex& index) {
  //std::cout << " - - - - - - - - - VectorInspectorPanel::onSelectedStroke() - - - - - - - - - - - - - - - - "
  //  << std::endl;
  //std::cout << "- - set selectingRowsForStroke to true - -" << std::endl;
  selectingRowsForStroke = true;
  setRowHighlighting();
  //std::cout << "- - set selectingRowsForStroke to false - -" << std::endl;
  selectingRowsForStroke = false;
}

void VectorInspectorPanel::onVectorInspectorSelectionChanged(
    const QItemSelection &selected, const QItemSelection &deselected) {
  //std::cout << " - - - - - - - - - VectorInspectorPanel::onVectorInspectorSelectionChanged() - - - - - - - - - - - - - - - - "
  //  << std::endl;
   // Handle the selection changed event
  if (strokeOrderChangedInProgress) {
    //std::cout << "- - strokeOrderChangedInProgress so no need to update select tool - -" << std::endl;
  }
  else if (initiatedBySelectTool) {
    //std::cout << "- - initiatedBySelectTool so no need to update select tool - -" << std::endl;
  }
  else if (selectingRowsForStroke) {
    //std::cout << "- - selectingRowsForStroke so no need to update select tool - -" << std::endl;
  }else{
    //std::cout << "- - set initiatedByVectorInspector to true - -" << std::endl;
    initiatedByVectorInspector = true;

    updateSelectToolSelectedRows(selected, deselected);

    //std::cout << "- - set initiatedByVectorInspector to false - -" << std::endl;
    initiatedByVectorInspector = false;
  }
}

void VectorInspectorPanel::updateSelectToolSelectedRows(const QItemSelection& selected, const QItemSelection& deselected) {
  //std::cout << " - - - - - - - - - VectorInspectorPanel::updateSelectToolSelectedRows() - - - - - - - - - - - - - - - - "
  //  << std::endl;
  ToolHandle* currentTool = TApp::instance()->getCurrentTool();
  VectorSelectionTool* vectorSelectionTool =
    dynamic_cast<VectorSelectionTool*>(currentTool->getTool());
  StrokeSelection* strokeSelection =
    dynamic_cast<StrokeSelection*>(currentTool->getTool()->getSelection());

  if (strokeSelection) {

    disconnect(proxyView, &QTreeView::clicked, this, &VectorInspectorPanel::onSelectedStroke);

    QModelIndexList selectedIndexes = selected.indexes();
    //qDebug() << "Selected items:";
    for (const QModelIndex& index : selectedIndexes) {
      if (index.column() == 0) {  // Check if the column is the first column
        int selectedStrokeIndex = index.data(Qt::DisplayRole).toInt();
        //qDebug() << selectedStrokeIndex;
        strokeSelection->select(selectedStrokeIndex, 1);
        strokeSelection->notifyView();
      }
    }

    QModelIndexList deselectedIndexes = deselected.indexes();
    //qDebug() << "Deselected items:";
    int priorRowStrokeIndexValue = -1;
    for (const QModelIndex &index : deselectedIndexes) {
      if (index.column() == 0) {  // Check if the column is the first column
        int deselectedStrokeIndex = index.data(Qt::DisplayRole).toInt();
        if (deselectedStrokeIndex == priorRowStrokeIndexValue) {
        } else {
          //qDebug() << deselectedStrokeIndex;
          strokeSelection->select(deselectedStrokeIndex, 0);
          strokeSelection->notifyView();
          priorRowStrokeIndexValue = deselectedStrokeIndex;
        }
      }
    }
    connect(proxyView, &QTreeView::clicked, this, &VectorInspectorPanel::onSelectedStroke);
  }
}