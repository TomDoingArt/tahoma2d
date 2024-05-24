#pragma once

#ifndef VECTORINSPECTOR_H
#define VECTORINSPECTOR_H

#include <QScrollArea>
#include <QLabel>
#include "multicolumnsortproxymodel.h"
#include "toonzqt/tselectionhandle.h"
#include "tvectorimage.h"
#include "toonz/tscenehandle.h"

// added for tooltips - begin
#include <QHeaderView>
#include <QStandardItemModel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTreeView>
// added for tooltips - end

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QSortFilterProxyModel;
class QTreeView;
QT_END_NAMESPACE

class VectorInspectorPanel final : public QWidget {
  Q_OBJECT

  QScrollArea *m_frameArea;

  TVectorImage *m_vectorImage;
  std::vector<int> m_selectedStrokeIndexes;

public:
  VectorInspectorPanel(QWidget *parent       = 0,
                       Qt::WindowFlags flags = Qt::WindowFlags());
  ~VectorInspectorPanel(){};

  void setHeaderToolTips();
  void setSourceModel(QAbstractItemModel *model);
  void showContextMenu(const QPoint &pos);
  void copySelectedItemsToClipboard();
  void contextMenuEvent(QContextMenuEvent *event);
  void getVectorLineData(std::ostream &os, int stroke,
                         std::vector<int> groupIds) const {}

public slots:
  void onLevelSwitched();
  void onSelectionSwitched(TSelection *selectionFrom, TSelection *selectionTo);
  void setRowHighlighting();
  void onSelectionChanged();
  void onEnteredGroup();
  void onExitedGroup();
  void onChangedStrokes();
  void onToolEditingFinished();
  void onSceneChanged();
  void onStrokeOrderChanged(int, int, int, bool);
  void onToolSwitched();
  void onSelectedAllStrokes();
  void onSelectedStroke(const QModelIndex &);
  void onVectorInspectorSelectionChanged(const QItemSelection &,
                                         const QItemSelection &);
  void updateSelectToolSelectedRows(const QItemSelection &,
                                    const QItemSelection &);

protected:
  void showEvent(QShowEvent *) override;
  void hideEvent(QHideEvent *) override;

private slots:
  void filterRegExpChanged();
  void filterColumnChanged();
  void sortChanged();

private:
  MultiColumnSortProxyModel *proxyModel;

  QFrame *proxyGroupBox;
  QTreeView *proxyView;
  QLabel *filterPatternLabel;
  QLabel *filterSyntaxLabel;
  QLabel *filterColumnLabel;
  QLineEdit *filterPatternLineEdit;
  QComboBox *filterSyntaxComboBox;
  QComboBox *filterColumnComboBox;
  bool initiatedByVectorInspector;
  bool initiatedBySelectTool;
  bool strokeOrderChangedInProgress;
  bool isSelected(int) const;
  bool selectingRowsForStroke;
  void changeWindowTitle();
};

#endif  // VECTORINSPECTOR_H
