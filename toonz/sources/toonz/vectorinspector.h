#pragma once

#ifndef VECTORINSPECTOR_H
#define VECTORINSPECTOR_H

#include <QScrollArea>
#include <QLabel>
#include "multicolumnsortproxymodel.h"
#include "toonzqt/tselectionhandle.h"

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
  QLabel* m_tempField;
  QScrollArea *m_frameArea;

public:
  VectorInspectorPanel(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~VectorInspectorPanel(){};

  void setSourceModel(QAbstractItemModel *model);
  void showContextMenu(const QPoint &pos);
  void copySelectedItemsToClipboard();
  void contextMenuEvent(QContextMenuEvent *event);
  void getVectorLineData(std::ostream &os, int stroke, std::vector<int> groupIds) const {}

public slots:
  void onLevelChanged();
  void onSelectionSwitched(TSelection* selectionFrom, TSelection* selectionTo);
  void onSelectionChanged(TSelection* selectionTo);

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
  QCheckBox *filterCaseSensitivityCheckBox;
  QCheckBox *sortCaseSensitivityCheckBox;
  QLabel *filterPatternLabel;
  QLabel *filterSyntaxLabel;
  QLabel *filterColumnLabel;
  QLineEdit *filterPatternLineEdit;
  QComboBox *filterSyntaxComboBox;
  QComboBox *filterColumnComboBox;
};

#endif  // VECTORINSPECTOR_H
