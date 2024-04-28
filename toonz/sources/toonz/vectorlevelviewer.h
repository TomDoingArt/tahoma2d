#pragma once

#ifndef VECTORLEVELVIEWER_H
#define VECTORLEVELVIEWER_H

#include <QScrollArea>
#include <QLabel>
#include "multicolumnsortproxymodel.h"

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

class VectorLevelViewerFrame final : public QFrame {
  Q_OBJECT

  QScrollArea *m_scrollArea;

public:
  VectorLevelViewerFrame(QScrollArea *parent   = 0,
                         Qt::WindowFlags flags = Qt::WindowFlags());
  ~VectorLevelViewerFrame(){};

   void setSourceModel(QAbstractItemModel *model);
   void showContextMenu(const QPoint &pos);
   void copySelectedItemsToClipboard();

private:

};

class VectorLevelViewPane final : public QWidget {
  Q_OBJECT

  QLabel *m_field;
  QLabel* m_tempField;
  QScrollArea *m_frameArea;

public:
  VectorLevelViewPane(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~VectorLevelViewPane(){};

  void setSourceModel(QAbstractItemModel *model);
  void showContextMenu(const QPoint &pos);
  void copySelectedItemsToClipboard();
  void contextMenuEvent(QContextMenuEvent *event);
  void getVectorLineData(std::ostream &os, int stroke, std::vector<int> groupIds) const {}

public slots:
  void onLevelChanged();

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

#endif  // VECTORLEVELVIEWER_H
