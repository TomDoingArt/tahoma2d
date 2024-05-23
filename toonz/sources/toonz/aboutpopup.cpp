#include "aboutpopup.h"
#include "tenv.h"
#include "tsystem.h"

#include <QPushButton>
#include <QLabel>
#include <QApplication>
#include <QScreen>
#include <QTextEdit>
#include <QDesktopServices>

AboutClickableLabel::AboutClickableLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent) {
  setStyleSheet("text-decoration: underline;");
}

AboutClickableLabel::~AboutClickableLabel() {}

void AboutClickableLabel::mousePressEvent(QMouseEvent* event) {
  emit clicked();
}

AboutPopup::AboutPopup(QWidget* parent)
    : DVGui::Dialog(parent, true, true, "About Tahoma2D") {
  setFixedWidth(360);
  setFixedHeight(350);

  setWindowTitle(tr("About Tahoma2D"));
  setTopMargin(0);

  TFilePath baseLicensePath   = TEnv::getStuffDir() + "doc/LICENSE";
  TFilePath tahomaLicensePath = baseLicensePath + "LICENSE.txt";

  QVBoxLayout* mainLayout = new QVBoxLayout();

  QLabel* logo = new QLabel(this);

  logo->setPixmap(QPixmap::fromImage(QImage(":Resources/tahoma2d_about.png")));
  mainLayout->addWidget(logo);

  QString name = QString::fromStdString(TEnv::getApplicationFullName());
  name += "\nBuilt: " __DATE__;
  mainLayout->addWidget(new QLabel(name));

  QLabel* blankLabel = new QLabel(this);
  blankLabel->setText(tr(" "));
  blankLabel->setWordWrap(true);

  mainLayout->addWidget(blankLabel);

  AboutClickableLabel* licenseLink = new AboutClickableLabel(this);
  licenseLink->setText(tr("Tahoma2D License"));

  connect(licenseLink, &AboutClickableLabel::clicked, [=]() {
    if (TSystem::isUNC(tahomaLicensePath))
      QDesktopServices::openUrl(
          QUrl(QString::fromStdWString(tahomaLicensePath.getWideString())));
    else
      QDesktopServices::openUrl(QUrl::fromLocalFile(
          QString::fromStdWString(tahomaLicensePath.getWideString())));
  });

  mainLayout->addWidget(licenseLink);

  AboutClickableLabel* thirdPartyLink = new AboutClickableLabel(this);
  thirdPartyLink->setText(tr("Third Party Licenses"));

  connect(thirdPartyLink, &AboutClickableLabel::clicked, [=]() {
    if (TSystem::isUNC(baseLicensePath))
      QDesktopServices::openUrl(
          QUrl(QString::fromStdWString(baseLicensePath.getWideString())));
    else
      QDesktopServices::openUrl(QUrl::fromLocalFile(
          QString::fromStdWString(baseLicensePath.getWideString())));
  });

  mainLayout->addWidget(thirdPartyLink);

  QLabel* ffmpegLabel = new QLabel(this);
  ffmpegLabel->setText(tr(
      "Tahoma2D ships with FFmpeg.  \nFFmpeg is licensed under the LGPLv2.1"));
  mainLayout->addWidget(ffmpegLabel);

  mainLayout->addSpacerItem(new QSpacerItem(this->width(), 10));

  mainLayout->addWidget(
      new QLabel(tr("Tahoma2D is made possible with the help of "
                    "patrons.\nSpecial thanks to:")));
  QLabel* patrons = new QLabel(
      "<i>Rodney Baker, Hans Jacob Wagner, Pierre Coffin, Adam Earle</i>");
  patrons->setTextFormat(Qt::TextFormat::RichText);
  mainLayout->addWidget(patrons);
  mainLayout->addWidget(new QLabel("  "));

  //  AboutClickableLabel* supportLink = new AboutClickableLabel(this);
  //  supportLink->setText(tr("Please consider supporting Tahoma2D on
  //  Patreon."));
  //  connect(supportLink, &AboutClickableLabel::clicked, [=]() {
  //    QDesktopServices::openUrl(QUrl("https://patreon.com/jeremybullock"));
  //    ;
  //  });
  //  supportLink->setToolTip("https://patreon.com/jeremybullock");
  //  mainLayout->addWidget(supportLink);
  mainLayout->addWidget(new QLabel(
      tr("Please consider sponsoring Tahoma2D developers on GitHub.")));
  mainLayout->addStretch();

  QFrame* mainFrame = new QFrame(this);
  mainFrame->setLayout(mainLayout);
  mainFrame->setFixedWidth(360);

  addWidget(mainFrame);

  QPushButton* button = new QPushButton(tr("Close"), this);
  button->setDefault(true);
  addButtonBarWidget(button);
  connect(button, SIGNAL(clicked()), this, SLOT(accept()));
}