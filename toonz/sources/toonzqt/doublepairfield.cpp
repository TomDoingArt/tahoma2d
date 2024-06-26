

#include "toonzqt/doublepairfield.h"
#include "toonzqt/dvdialog.h"

#include "tcommon.h"

#include <cmath>

#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QSlider>
#include <QHBoxLayout>

using namespace DVGui;

#ifdef MACOSX
#define MARGIN_OFFSET 7
#else
#define MARGIN_OFFSET 2
#endif

//=============================================================================
// DoubleValuePairField
//-----------------------------------------------------------------------------

DoubleValuePairField::DoubleValuePairField(QWidget *parent,
                                           bool isMaxRangeLimited,
                                           DoubleValueLineEdit *leftLineEdit,
                                           DoubleValueLineEdit *rightLineEdit)
    : QWidget(parent)
    , m_values(0, 0)
    , m_minValue(0)
    , m_maxValue(100)
    , m_grabOffset(0)
    , m_grabIndex(-1)
    , m_leftMargin(72)
    , m_rightMargin(72)
    , m_isMaxRangeLimited(isMaxRangeLimited)
    , m_leftLineEdit(leftLineEdit)
    , m_rightLineEdit(rightLineEdit)
    , m_isLinear(true) {
  assert(m_leftLineEdit);
  assert(m_rightLineEdit);
  setObjectName("DoublePairField");
  setFixedHeight(WidgetHeight);

  m_leftLabel  = new QLabel("", this);
  m_rightLabel = new QLabel("", this);

  m_leftLineEdit->setFixedWidth(60);
  m_rightLineEdit->setFixedWidth(60);

  //---- layout
  QHBoxLayout *m_mainLayout = new QHBoxLayout;
  m_mainLayout->setContentsMargins(0, 0, 0, 0);
  m_mainLayout->setSpacing(3);
  {
    m_mainLayout->addWidget(m_leftLabel, 1);
    m_mainLayout->addWidget(m_leftLineEdit, 1, Qt::AlignLeft);
    m_mainLayout->addSpacing(35);
    m_mainLayout->addStretch(100);
    m_mainLayout->addWidget(m_rightLabel, 1);
    m_mainLayout->addWidget(m_rightLineEdit, 1, Qt::AlignRight);
  }
  setLayout(m_mainLayout);

  //---- signal-slot connections
  bool ret = connect(m_leftLineEdit, SIGNAL(editingFinished()),
                     SLOT(onLeftEditingFinished()));
  ret      = ret && connect(m_rightLineEdit, SIGNAL(editingFinished()),
                            SLOT(onRightEditingFinished()));
  assert(ret);
}

//-----------------------------------------------------------------------------

double DoubleValuePairField::pos2value(int x) const {
  int xMin = m_leftMargin, xMax = width() - m_rightMargin - 1;
  if (m_isLinear)
    return m_minValue + (m_maxValue - m_minValue) * (x - xMin) / (xMax - xMin);

  // nonlinear slider case
  double posRatio = (double)(x - xMin) / (double)(xMax - xMin);
  double t;
  if (posRatio <= 0.5)
    t = 0.04 * posRatio;
  else if (posRatio <= 0.75)
    t = -0.02 + 0.08 * posRatio;
  else if (posRatio <= 0.9)
    t = -0.26 + 0.4 * posRatio;
  else
    t = -8.0 + 9.0 * posRatio;
  return m_minValue + (m_maxValue - m_minValue) * t;
}

//-----------------------------------------------------------------------------

int DoubleValuePairField::value2pos(double v) const {
  int xMin = m_leftMargin, xMax = width() - m_rightMargin - 1;
  if (m_isLinear)
    return xMin + (int)(((xMax - xMin) * (v - m_minValue)) /
                        (m_maxValue - m_minValue));

  // nonlinear slider case
  double valueRatio =
      (v - (double)m_minValue) / (double)(m_maxValue - m_minValue);
  double t;
  if (valueRatio <= 0.02)
    t = valueRatio / 0.04;
  else if (valueRatio <= 0.04)
    t = (valueRatio + 0.02) / 0.08;
  else if (valueRatio <= 0.1)
    t = (valueRatio + 0.26) / 0.4;
  else
    t = (valueRatio + 8.0) / 9.0;
  return xMin + (int)(t * (double)(xMax - xMin));
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setBrush(Qt::NoBrush);

  int x0 = value2pos(m_minValue);
  int x1 = value2pos(m_maxValue);
  int y  = height() / 2;

  p.setPen(QPen(getDarkLineColor(), 4));
  p.drawLine(x0 - 1, y, x1, y);

  p.setPen(Qt::black);

  int y1 = height() - 1;
  int x;
  x                = value2pos(m_values.first);
  QRect sliderRect = QRect(x0, -5, x1 - x0 + 1, 10);

  if (sliderRect.contains(QPoint(x, 0)))
    p.drawPixmap(x - m_handleLeftPixmap.width() + 1, 2, m_handleLeftPixmap);
  else
    p.drawPixmap(sliderRect.right() - m_handleLeftPixmap.width() + 1, 2,
                 m_handleLeftGrayPixmap);

  x = value2pos(m_values.second);

  if (sliderRect.contains(QPoint(x, 0)))
    p.drawPixmap(x, 2, m_handleRightPixmap);
  else
    p.drawPixmap(sliderRect.right(), 2, m_handleRightGrayPixmap);
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::setLeftText(const QString &text) {
  QString oldText = m_leftLabel->text();

  int oldLabelSize = fontMetrics().horizontalAdvance(oldText);
  int newLabelSize = fontMetrics().horizontalAdvance(text);
  int labelSize    = newLabelSize - oldLabelSize;
  m_leftMargin += labelSize + MARGIN_OFFSET;

  m_leftLabel->setText(text);
  update();
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::setRightText(const QString &text) {
  QString oldText = m_rightLabel->text();

  int oldLabelSize = fontMetrics().horizontalAdvance(oldText);
  int newLabelSize = fontMetrics().horizontalAdvance(text);
  int labelSize    = newLabelSize - oldLabelSize;
  m_rightMargin += labelSize + MARGIN_OFFSET;

  m_rightLabel->setText(text);
  update();
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::setLabelsEnabled(bool enable) {
  if (!enable) {
    m_rightLabel->hide();
    m_leftLabel->hide();
  } else {
    m_rightLabel->show();
    m_leftLabel->show();
  }
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::setValue(double value) {
  int decimals = std::min(m_leftLineEdit->getDecimals(), 4);
  value = tround(value * std::pow(10, decimals)) * std::pow(0.1, decimals);
  value = tcrop(value, m_minValue, m_maxValue);
  if (m_grabIndex == 0)  // Left grab
  {
    m_values.first = value;
    m_leftLineEdit->setValue(m_values.first);

    if (value > m_values.second) {
      m_values.second = value;
      m_rightLineEdit->setValue(m_values.second);
    }
  } else  // Right grab
  {
    m_values.second = value;
    m_rightLineEdit->setValue(m_values.second);

    if (value < m_values.first) {
      m_values.first = value;
      m_leftLineEdit->setValue(m_values.first);
    }
  }
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::setValues(const std::pair<double, double> &values) {
  assert(values.first <= values.second);
  m_values.first = tcrop(values.first, m_minValue, m_maxValue);
  m_leftLineEdit->setValue(m_values.first);

  m_values.second = values.second;
  if (m_isMaxRangeLimited)
    m_values.second = tcrop(values.second, m_values.first, m_maxValue);
  m_rightLineEdit->setValue(m_values.second);

  update();
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::setRange(double minValue, double maxValue) {
  m_minValue = minValue;
  m_maxValue = maxValue;

  if (!m_isMaxRangeLimited) maxValue = (std::numeric_limits<int>::max)();

  m_leftLineEdit->setRange(minValue, maxValue);
  m_rightLineEdit->setRange(minValue, maxValue);
  update();
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::getRange(double &minValue, double &maxValue) {
  minValue = m_minValue;
  maxValue = m_maxValue;
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    int x = event->pos().x();
    int cur0, cur1;
    if (m_values.first > m_maxValue)
      cur0 = value2pos(m_maxValue) - 5;
    else
      cur0 = value2pos(m_values.first);
    if (m_values.second > m_maxValue)
      cur1 = value2pos(m_maxValue);
    else
      cur1 = value2pos(m_values.second);
    int d0 = abs(cur0 - x);
    int d1 = abs(cur1 - x);
    int d, cur;
    if (d0 < d1 || (d0 == d1 && x < cur0)) {
      d           = abs(d0);
      cur         = cur0;
      m_grabIndex = 0;
    } else {
      d           = abs(d1);
      cur         = cur1;
      m_grabIndex = 1;
    }
    if (d < 6)
      m_grabOffset = cur - x;
    else {
      m_grabOffset = 0;
      setValue(pos2value(x));
      emit valuesChanged(true);
      update();
    }
  }
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons()) {
    std::pair<double, double> oldValues = m_values;
    int x                               = event->pos().x() + m_grabOffset;
    setValue(pos2value(x));
    if (oldValues != m_values) {
      emit valuesChanged(true);
      update();
    }
  }
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::mouseReleaseEvent(QMouseEvent *event) {
  m_grabOffset = 0;
  m_grabIndex  = 0;
  emit valuesChanged(false);
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::onLeftEditingFinished() {
  double value = m_leftLineEdit->getValue();
  if (value == m_values.first) return;
  if (!m_isMaxRangeLimited && value < m_minValue)
    value = m_minValue;
  else if (m_isMaxRangeLimited)
    value = tcrop(value, m_minValue, m_maxValue);
  m_values.first = value;
  if (m_values.first > m_values.second) {
    m_values.second = m_values.first;
    m_rightLineEdit->setValue(m_values.second);
  }
  emit valuesChanged(false);
  update();
}

//-----------------------------------------------------------------------------

void DoubleValuePairField::onRightEditingFinished() {
  double value = m_rightLineEdit->getValue();
  if (value == m_values.second) return;
  if (m_isMaxRangeLimited) value = tcrop(value, m_minValue, m_maxValue);
  m_values.second = value;
  if (m_values.second < m_values.first) {
    m_values.first = m_values.second;
    m_leftLineEdit->setValue(m_values.first);
  }
  emit valuesChanged(false);
  update();
}

//=============================================================================
// DoublePairField
//-----------------------------------------------------------------------------

DoublePairField::DoublePairField(QWidget *parent, bool isMaxRangeLimited)
    : DoubleValuePairField(parent, isMaxRangeLimited, new DoubleLineEdit(0),
                           new DoubleLineEdit(0)) {
  DoubleLineEdit *leftLineEdit = dynamic_cast<DoubleLineEdit *>(m_leftLineEdit);
  leftLineEdit->setDecimals(2);
  DoubleLineEdit *rightLineEdit =
      dynamic_cast<DoubleLineEdit *>(m_rightLineEdit);
  rightLineEdit->setDecimals(2);
}

//=============================================================================
// MeasuredDoublePairField
//-----------------------------------------------------------------------------

MeasuredDoublePairField::MeasuredDoublePairField(QWidget *parent,
                                                 bool isMaxRangeLimited)
    : DoubleValuePairField(parent, isMaxRangeLimited,
                           new MeasuredDoubleLineEdit(0),
                           new MeasuredDoubleLineEdit(0)) {
  m_leftLineEdit->setFixedSize(63, WidgetHeight);
  m_rightLineEdit->setFixedSize(63, WidgetHeight);
}

//-----------------------------------------------------------------------------

void MeasuredDoublePairField::setMeasure(std::string measureName) {
  MeasuredDoubleLineEdit *leftLineEdit =
      dynamic_cast<MeasuredDoubleLineEdit *>(m_leftLineEdit);
  assert(leftLineEdit);
  leftLineEdit->setMeasure(measureName);
  MeasuredDoubleLineEdit *rightLineEdit =
      dynamic_cast<MeasuredDoubleLineEdit *>(m_rightLineEdit);
  assert(rightLineEdit);
  rightLineEdit->setMeasure(measureName);
}

//-----------------------------------------------------------------------------
/*--設定ファイルからインタフェースの精度を変える--*/
void MeasuredDoublePairField::setPrecision(int precision) {
  MeasuredDoubleLineEdit *leftLineEdit =
      dynamic_cast<MeasuredDoubleLineEdit *>(m_leftLineEdit);
  if (leftLineEdit) leftLineEdit->setDecimals(precision);
  MeasuredDoubleLineEdit *rightLineEdit =
      dynamic_cast<MeasuredDoubleLineEdit *>(m_rightLineEdit);
  if (rightLineEdit) rightLineEdit->setDecimals(precision);
}
