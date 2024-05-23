#pragma once

#ifndef VECTORINSPECTOR_H
#define VECTORINSPECTOR_H

#include <QScrollArea>
#include <QLabel>
#include "multicolumnsortproxymodel.h"
#include "toonzqt/tselectionhandle.h"
#include "tvectorimage.h"
#include "toonz/tscenehandle.h"

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

class VectorInspectorFrame final : public QFrame {
  Q_OBJECT

  QScrollArea *m_scrollArea;

public:
  VectorInspectorFrame(QScrollArea *parent   = 0,
                         Qt::WindowFlags flags = Qt::WindowFlags());
  ~VectorInspectorFrame(){};

   void setSourceModel(QAbstractItemModel *model);
   void showContextMenu(const QPoint &pos);
   void copySelectedItemsToClipboard();

private:

};

class VectorInspectorPanel final : public QWidget {
  Q_OBJECT

  QLabel *m_field;
  QScrollArea *m_frameArea;
  TVectorImage *m_vectorImage;
  std::vector<int> m_selectedStrokeIndexes;

public:
  VectorInspectorPanel(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~VectorInspectorPanel(){};

  void setSourceModel(QAbstractItemModel *model);
  void showContextMenu(const QPoint &pos);
  void copySelectedItemsToClipboard();
  void contextMenuEvent(QContextMenuEvent *event);
  void getVectorLineData(std::ostream &os, int stroke, std::vector<int> groupIds) const {}

public slots:
  void onLevelSwitched();
  void onSelectionSwitched(TSelection* selectionFrom, TSelection* selectionTo);
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
  void onSelectedStroke(const QModelIndex&);
  void onVectorInspectorSelectionChanged(const QItemSelection&, const QItemSelection&);
  void updateSelectToolSelectedRows(const QItemSelection&, const QItemSelection&);

protected:
  void showEvent(QShowEvent *) override;
  void hideEvent(QHideEvent *) override;

private slots:
  void filterRegExpChanged();
  void filterColumnChanged();
  void sortChanged();

private:
  MultiColumnSortProxyModel *proxyModel;

  QGroupBox *proxyGroupBox;
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
};

#endif  // VECTORINSPECTOR_H
