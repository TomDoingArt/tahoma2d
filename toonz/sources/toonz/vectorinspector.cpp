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
#include "toonz\tvectorimageutils.h"
#include "tstroke.h"
#include "toonz/tobjecthandle.h"
#include "toonzqt/selection.h"
#include "toonzqt/tselectionhandle.h"
#include "tools/tool.h"
#include "tools/toolhandle.h"
#include "tools/strokeselection.h"

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


//QWidget m_panel;

//void addVectorDataRow(QAbstractItemModel *model, const QString &stroke, const QString &groupid,
//            const QString &id, const QString &styleid, const QString &quad,
//            const QString &p, const QString &x, const QString &y, const QString &thickness)
//{
//   model->insertRow(0);
//   model->setData(model->index(0, 0), stroke);
//   model->setData(model->index(0, 1), groupid);
//   model->setData(model->index(0, 2), id);
//   model->setData(model->index(0, 3), styleid);
//   model->setData(model->index(0, 4), quad);
//   model->setData(model->index(0, 5), p);
//   model->setData(model->index(0, 6), x);
//   model->setData(model->index(0, 7), y);
//   model->setData(model->index(0, 8), thickness);
//
//}

//static void printStrokes(std::vector<VIStroke*>& v, int size){
//
//   qDebug().noquote() << QDateTime::currentDateTimeUtc().toString(Qt::ISODate) << "printStrokes() called";
//
//   std::ofstream file("C:\\temp\\VectorInspectorStrokes.txt");
//
//   //Header
//   file << "Stroke,Group Id,Id,StyleId,Quad,P,x,y,Thickness," << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString() << endl;
//
//   // pass in a Vector of strokes, and the size.
//
//   for (int i = 0; i < (UINT)size; i++) {
//       //file << i << "," << std::to_string(*TVectorImage::Imp::m_strokes[i]->m_groupId.m_id.data());
//       v[i]->m_s->print(file, i, v[i]->m_groupId.m_id);
//       //VectorInspectorPanel::getVectorLineData(file, i, v[i]->m_groupId.m_id);
//   }
//   //printStrokes(file);
//   file.close();
//
//}

//QStandardItemModel *createVectorDataModel(QObject *parent)
//{
//   QStandardItemModel *model = new QStandardItemModel(0, 9, parent);
//
//   model->setHeaderData(0, Qt::Horizontal, QObject::tr("Stroke"));
//   model->setHeaderData(1, Qt::Horizontal, QObject::tr("Group Id"));
//   model->setHeaderData(2, Qt::Horizontal, QObject::tr("Id"));
//   model->setHeaderData(3, Qt::Horizontal, QObject::tr("StyleId"));
//   model->setHeaderData(4, Qt::Horizontal, QObject::tr("Quad"));
//   model->setHeaderData(5, Qt::Horizontal, QObject::tr("P"));
//   model->setHeaderData(6, Qt::Horizontal, QObject::tr("x"));
//   model->setHeaderData(7, Qt::Horizontal, QObject::tr("y"));
//   model->setHeaderData(8, Qt::Horizontal, QObject::tr("Thickness"));
//
//   // addVectorDataRow(model,"Stroke","Group Id","Id","StyleId","Quad","P","x","y","Thickness");
//
//   addVectorDataRow(model,"0","-2","7","4","0","0","84.0312","81.2656","0.505882");
//   addVectorDataRow(model,"0","-2","7","4","0","1","92.6406","55.6172","1.29412");
//   addVectorDataRow(model,"0","-2","7","4","1","0","86.6953","34.3828","1.84706");
//   addVectorDataRow(model,"0","-2","7","4","1","1","77.8672","2.86719","2.67059");
//   addVectorDataRow(model,"0","-2","7","4","2","0","49.2109","-14.6641","3");
//   addVectorDataRow(model,"0","-2","7","4","2","1","28.2969","-26.5469","3");
//   addVectorDataRow(model,"0","-2","7","4","3","0","4.83594","-33.25","3");
//   addVectorDataRow(model,"0","-2","7","4","3","1","-18.6094","-39.9531","3");
//   addVectorDataRow(model,"0","-2","7","4","4","0","-40.5625","-49.9453","3");
//   addVectorDataRow(model,"0","-2","7","4","4","1","-69.7109","-64.3672","3");
//   addVectorDataRow(model,"0","-2","7","4","4","2","-88.6875","-93.8125","3");
//   addVectorDataRow(model,"1","-2","8","7","0","0","2.79688","51.4766","0.682353");
//   addVectorDataRow(model,"1","-2","8","7","0","1","11.2812","52.1328","1.61176");
//   addVectorDataRow(model,"1","-2","8","7","1","0","19.4219","48.25","1.77647");
//   addVectorDataRow(model,"1","-2","8","7","1","1","27.5547","44.3672","1.95294");
//   addVectorDataRow(model,"1","-2","8","7","2","0","35.3438","40.3438","2.05882");
//   addVectorDataRow(model,"1","-2","8","7","2","1","79","19.8359","2.81176");
//   addVectorDataRow(model,"1","-2","8","7","3","0","91.6953","-17.5703","3");
//   addVectorDataRow(model,"1","-2","8","7","3","1","98.1328","-36.5312","3");
//   addVectorDataRow(model,"1","-2","8","7","4","0","97.6484","-52.1875","3");
//   addVectorDataRow(model,"1","-2","8","7","4","1","97.2344","-65.5781","3");
//   addVectorDataRow(model,"1","-2","8","7","5","0","86.3125","-89.3516","3");
//   addVectorDataRow(model,"1","-2","8","7","5","1","78.3047","-106.789","3");
//   addVectorDataRow(model,"1","-2","8","7","5","2","68.0156","-111.047","3");
//   addVectorDataRow(model,"2","4","9","5","0","0","21.7266","84.7578","0.505882");
//   addVectorDataRow(model,"2","4","9","5","0","1","27.0781","63.4922","1.36471");
//   addVectorDataRow(model,"2","4","9","5","1","0","26.5078","41.25","2.15294");
//   addVectorDataRow(model,"2","4","9","5","1","1","25.9297","19.0078","2.94118");
//   addVectorDataRow(model,"2","4","9","5","2","0","25.9688","-3.0625","3");
//   addVectorDataRow(model,"2","4","9","5","2","1","27.1953","-31.3281","3");
//   addVectorDataRow(model,"2","4","9","5","3","0","29.5234","-59.625","3");
//   addVectorDataRow(model,"2","4","9","5","3","1","31.8516","-87.9141","3");
//   addVectorDataRow(model,"2","4","9","5","3","2","29.7109","-116.07","3");
//   addVectorDataRow(model,"3","4.3","10","6","0","0","133.359","-25.3047","0.505882");
//   addVectorDataRow(model,"3","4.3","10","6","0","1","120.211","-24","0.541176");
//   addVectorDataRow(model,"3","4.3","10","6","1","0","106.992","-23.8672","0.658824");
//   addVectorDataRow(model,"3","4.3","10","6","1","1","93.7656","-23.7344","0.776471");
//   addVectorDataRow(model,"3","4.3","10","6","2","0","80.5469","-23.6016","0.917647");
//   addVectorDataRow(model,"3","4.3","10","6","2","1","57.4688","-23.1484","1.14118");
//   addVectorDataRow(model,"3","4.3","10","6","3","0","34.3906","-22.5938","1.27059");
//   addVectorDataRow(model,"3","4.3","10","6","3","1","11.3125","-22.0312","1.4");
//   addVectorDataRow(model,"3","4.3","10","6","4","0","-11.7578","-22.1875","1.85882");
//   addVectorDataRow(model,"3","4.3","10","6","4","1","-38.9688","-22.4531","2.43529");
//   addVectorDataRow(model,"3","4.3","10","6","5","0","-59.75","-22","2.81176");
//   addVectorDataRow(model,"3","4.3","10","6","5","1","-66.8906","-21.6328","2.88235");
//   addVectorDataRow(model,"3","4.3","10","6","6","0","-74.0938","-21.0312","2.94118");
//   addVectorDataRow(model,"3","4.3","10","6","6","1","-81.2969","-20.4219","3");
//   addVectorDataRow(model,"3","4.3","10","6","6","2","-88.1797","-22.0938","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","0","0","-81.6562","90.9844","0.529412");
//   addVectorDataRow(model,"4","4.3.2","11","3","0","1","-78.3906","71.5312","0.847059");
//   addVectorDataRow(model,"4","4.3.2","11","3","1","0","-71.7188","61.2656","1.25882");
//   addVectorDataRow(model,"4","4.3.2","11","3","1","1","-67.3672","54.8125","1.41176");
//   addVectorDataRow(model,"4","4.3.2","11","3","2","0","-61.7109","49.375","1.56471");
//   addVectorDataRow(model,"4","4.3.2","11","3","2","1","-56.0469","43.9375","1.70588");
//   addVectorDataRow(model,"4","4.3.2","11","3","3","0","-50.3203","38.625","1.78824");
//   addVectorDataRow(model,"4","4.3.2","11","3","3","1","-23.3203","12.7891","2.54118");
//   addVectorDataRow(model,"4","4.3.2","11","3","4","0","21.1562","-14.5703","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","4","1","56.7422","-36.4609","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","5","0","79.6328","-58.0156","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","5","1","102.523","-79.5703","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","6","0","112.711","-100.789","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","6","1","116.82","-108.836","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","7","0","120.836","-117.031","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","7","1","124.852","-125.219","3");
//   addVectorDataRow(model,"4","4.3.2","11","3","7","2","126.141","-134.086","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","0","0","54.6562","51.2891","0.505882");
//   addVectorDataRow(model,"5","4.3.2","12","8","0","1","49.8203","46.875","1.10588");
//   addVectorDataRow(model,"5","4.3.2","12","8","1","0","43.6875","44.0781","1.14118");
//   addVectorDataRow(model,"5","4.3.2","12","8","1","1","37.5469","41.2812","1.17647");
//   addVectorDataRow(model,"5","4.3.2","12","8","2","0","32.1094","37.5156","1.25882");
//   addVectorDataRow(model,"5","4.3.2","12","8","2","1","26.9531","33.3984","1.4");
//   addVectorDataRow(model,"5","4.3.2","12","8","3","0","23.1406","27.875","1.64706");
//   addVectorDataRow(model,"5","4.3.2","12","8","3","1","19.3281","22.3594","1.88235");
//   addVectorDataRow(model,"5","4.3.2","12","8","4","0","15.4609","16.9531","2.16471");
//   addVectorDataRow(model,"5","4.3.2","12","8","4","1","0.265625","-3.16406","2.83529");
//   addVectorDataRow(model,"5","4.3.2","12","8","5","0","-3.65625","-16.25","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","5","1","-8.15625","-31.25","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","6","0","-7.75781","-44.0547","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","6","1","-7.35938","-56.8594","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","7","0","-2.07031","-67.4688","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","7","1","6.29688","-84.2578","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","8","0","20.5859","-87.1875","3");
//   addVectorDataRow(model,"5","4.3.2","12","8","8","1","23.1172","-87.6641","2.52941");
//   addVectorDataRow(model,"5","4.3.2","12","8","9","0","33.2188","-91.3125","1.85882");
//   addVectorDataRow(model,"5","4.3.2","12","8","9","1","43.4375","-95.0156","1.2");
//   addVectorDataRow(model,"5","4.3.2","12","8","9","2","45.9766","-95.1797","0.564706");
//
//   return model;
//}


//-----------------------------------------------------------------------------
VectorInspectorFrame::VectorInspectorFrame(QScrollArea *parent,
                                              Qt::WindowFlags flags)
   : QFrame(parent, flags), m_scrollArea(parent) {
 setObjectName("VectorInspectorFrame");
 setFrameStyle(QFrame::StyledPanel);

 setFocusPolicy(Qt::StrongFocus); /*-- Keyboard Tab Focus --*/

 setFixedHeight(parentWidget()->height());
 setMinimumWidth(std::max(parentWidget()->width(),800));
}

//------------------------------------------------------------------------------

VectorInspectorPanel::VectorInspectorPanel(QWidget *parent, Qt::WindowFlags flags)
   : QWidget(parent) {

  m_field =
     new QLabel("I am a test value for the QLabel");

 m_field->setFixedHeight(16);

 m_tempField = new QLabel("I am m_tempField");

 setFocusProxy(m_field);

 proxyModel = new MultiColumnSortProxyModel;

 proxyView = new QTreeView;
 proxyView->setRootIsDecorated(false);
 proxyView->setAlternatingRowColors(true);
 proxyView->setModel(proxyModel);
 proxyView->setSortingEnabled(true);

 proxyView->setSelectionMode(QAbstractItemView::ExtendedSelection);
 proxyView->setSelectionBehavior(QAbstractItemView::SelectItems);

 proxyView->setContextMenuPolicy(Qt::CustomContextMenu);
 connect(proxyView, &QWidget::customContextMenuRequested, this, &VectorInspectorPanel::showContextMenu);

 sortCaseSensitivityCheckBox = new QCheckBox(tr("Case sensitive sorting"));
 filterCaseSensitivityCheckBox = new QCheckBox(tr("Case sensitive filter"));

 filterPatternLineEdit = new QLineEdit;
 filterPatternLabel = new QLabel(tr("&Filter pattern (example 0|1):"));
 filterPatternLabel->setBuddy(filterPatternLineEdit);

 filterSyntaxComboBox = new QComboBox;
 filterSyntaxComboBox->addItem(tr("Regular expression"), QRegExp::RegExp);
 filterSyntaxComboBox->addItem(tr("Wildcard"), QRegExp::Wildcard);
 filterSyntaxComboBox->addItem(tr("Fixed string"), QRegExp::FixedString);
 filterSyntaxLabel = new QLabel(tr("Filter &syntax:"));
 filterSyntaxLabel->setBuddy(filterSyntaxComboBox);

 filterColumnComboBox = new QComboBox;
 filterColumnComboBox->addItem(tr("Stroke"));
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

 connect(filterPatternLineEdit, &QLineEdit::textChanged,
         this, &VectorInspectorPanel::filterRegExpChanged);
 connect(filterSyntaxComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
         this, &VectorInspectorPanel::filterRegExpChanged);
 connect(filterColumnComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
         this, &VectorInspectorPanel::filterColumnChanged);
 connect(filterCaseSensitivityCheckBox, &QAbstractButton::toggled,
         this, &VectorInspectorPanel::filterRegExpChanged);
 connect(sortCaseSensitivityCheckBox, &QAbstractButton::toggled,
         this, &VectorInspectorPanel::sortChanged);

 proxyGroupBox = new QGroupBox();

 QGridLayout *proxyLayout = new QGridLayout;
 proxyLayout->addWidget(proxyView, 0, 0, 1, 3);
 proxyLayout->addWidget(filterPatternLabel, 1, 0);
 proxyLayout->addWidget(filterPatternLineEdit, 1, 1, 1, 2);
 proxyLayout->addWidget(filterSyntaxLabel, 2, 0);
 proxyLayout->addWidget(filterSyntaxComboBox, 2, 1, 1, 2);
 proxyLayout->addWidget(filterColumnLabel, 3, 0);
 proxyLayout->addWidget(filterColumnComboBox, 3, 1, 1, 2);
 proxyLayout->addWidget(filterCaseSensitivityCheckBox, 4, 0, 1, 2);
 proxyLayout->addWidget(sortCaseSensitivityCheckBox, 4, 2);
 proxyGroupBox->setLayout(proxyLayout);

 QVBoxLayout *mainLayout = new QVBoxLayout;

 //mainLayout->addWidget(m_frameArea, 800);
 mainLayout->addWidget(m_field);
 mainLayout->addWidget(m_tempField);
 mainLayout->addWidget(proxyGroupBox);

 setLayout(mainLayout);

 setWindowTitle(tr("Basic Sort/Filter Model"));
 resize(500, 450);

 proxyView->sortByColumn(0, Qt::AscendingOrder);
 filterColumnComboBox->setCurrentIndex(6); // position 6 for the 'P' column of the Vector Inspector QTreeView
 filterPatternLineEdit->setText("2"); //set default filter here, like: "0|1" to filter on value 0 or 1
 filterCaseSensitivityCheckBox->setChecked(true);
 sortCaseSensitivityCheckBox->setChecked(true);

}

//-----------------------------------------------------------------------------

void VectorInspectorPanel::onLevelChanged() {
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

  // TXshLevel currentLevel = TApp::instance()->getCurrentLevel()->getLevel();

 std::cout << "---------onLevelChanged.levelType:" << levelType << std::endl;
 
 if (levelType == "Vector") {
   // ----------------------- the new stuff ----------------------------------

   TApp *app = TApp::instance();

   TXshLevelHandle *currentLevel = app->getCurrentLevel();

   TFrameHandle *currentFrame = app->getCurrentFrame();

   TColumnHandle *currentColumn = app->getCurrentColumn();

   int frameIndex = currentFrame->getFrameIndex();

   // ToDo: Add a signal and slot for when the frame changes in order to update
   // this info.

   TXshSimpleLevel *currentSimpleLevel = currentLevel->getSimpleLevel();

   // ToDo - get current cell from xsheet.

   int row = currentFrame->getFrame();
   int col = currentColumn->getColumnIndex();

   std::cout << "row:" << row << ", column:" << col << std::endl;

   TXsheet *xsheet = app->getCurrentXsheet()->getXsheet();

   const TXshCell cell = xsheet->getCell(row, col);

   // ToDo - get the image eposed at that cell.
   TVectorImageP vectorImage = cell.getImage(false);

   //std::cout << "imp:" << vectorImage.m_imp.get() << std::endl; vectorImage.m_imp;

   // TVectorImageP* vectorImage = currentLevel->getFrame(frameIndex);
   // TVectorImageP vectorImage = cell.getImage(false);
   
   //TVectorImageP vectorImage = cell.getImage(false);

   // TVectorImageP *vectorImage = currentSimpleLevel->getFrame(frameIndex);

   // UINT fillStyle = vectorImage->getFillData(intersectionBranch);

   // Cannot access <TVectorImage::Imp> outside of the class. Consider adding methods to TVectorImage if access to information is needed.
   //std::unique_ptr<TVectorImage::Imp>
   //std::unique_ptr<TVectorImage::Imp::Imp> m_impintersectionData = <TVectorImage> vectorImage->m_imp  m_intersectionData;

   // std::vector<VIStroke*> m_strokes = vectorImage->getStrokeCount();
   //UINT strokeCount = vectorImage.getStrokeCount();
   UINT strokeCount = vectorImage->getStrokeCount();

   std::cout << "TVectorImageP Stroke count:" << strokeCount << ", getType():" << vectorImage->getType() << std::endl;

   //std::vector <VIStroke*> strokes = vectorImage;

   //VIStroke m_strokes = vectorImage.getVIStroke(0);

   // printStrokes(m_strokes, m_strokes.size());
   //vectorImage.getStrokeListData();

   std::cout << "------------------ calling getStrokeListData for strokeCount:" << strokeCount << std::endl;
   QStandardItemModel* model = vectorImage->getStrokeListData(parentWidget());
   setSourceModel(model);
   proxyModel->invalidate();

   m_tempField->setText(QString(" app exists: ") + (app != nullptr ? "true" : "false") +
     QString("; currentLevel exists: ") + (currentLevel != nullptr ? "true" : "false") +
     QString("; currentFrame exists: ") + (currentFrame != nullptr ? "true" : "false") +
     QString("; frameIndex: ") + QString::fromStdString(std::to_string(frameIndex)) +
     QString("; currentSimpleLevel exists: ") + (currentSimpleLevel != nullptr ? "true" : "false") +
     QString("; strokeCount: ") + QString::fromStdString(std::to_string(strokeCount)) //+
     //QString("; m_imp exists: ") + (cell.getImage(false).m_imp != nullptr ? "true" : "false")
   );

   m_tempField->adjustSize();

 }

 qDebug() << "ISODate enum value:" << utcNow.toString(Qt::ISODate)
          << "Level type:" << QString::fromStdString(levelType);

 m_field->setText(utcNow.toString(Qt::ISODate) +
                  " Level type:" + QString::fromStdString(levelType));

 m_field->adjustSize();

 filterColumnComboBox->setWindowTitle(utcNow.toString(Qt::ISODate) +
               " Level type:" + QString::fromStdString(levelType));

 update();
}

//-------------------------------------------------------------------------------------------------

//void VectorInspectorPanel::onObjectSwitched(QShowEvent*) {}
void VectorInspectorPanel::onSelectionSwitched(TSelection* selectionFrom, TSelection* selectionTo) {
  std::cout << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString() << " ------- VectorInspectorPanel::onSelectionSwitched(), noticed the selection switched -----" << std::endl;
}

//-------------------------------------------------------------------------------------------------

void VectorInspectorPanel::onSelectionChanged(TSelection* selectionTo) {
  std::cout << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString() << " ------- VectorInspectorPanel::onSelectionChanged(), noticed the selection changed -----" << std::endl;
  TSelection* currentSelection = selectionTo->getCurrent();
    
  // Get selected strokes
  ToolHandle* currentTool = TApp::instance()->getCurrentTool();
  std::cout << "------- Tool Name:" << currentTool->getTool()->getName() << std::endl; // Tool Name:T_Selection

  std::cout << "------- Tool Type:" << currentTool->getTool()->getToolType() << std::endl; // Tool Type:8

  if ("T_Selection" == currentTool->getTool()->getName()){
    //TImage* currentImage = currentTool->getTool()->getImage(false);

    //std::cout << "------- Image Type:" << currentImage->getType() << std::endl;

    //TSelection* currentSelection = currentTool->getTool()->getSelection();

    //TVectorImageP vectorImage = currentTool->getTool()->getImage(false);

    StrokeSelection* strokeSelection = dynamic_cast<StrokeSelection*>(currentTool->getTool()->getSelection());

    //for (int i = 0; i < (int)vectorImage->getStrokeCount(); i++) {
    //  std::cout << "------- stroke " << i << " is selected:" << strokeSelection->isSelected(i) << std::endl;
    //}

    // Loop through the rows and set highlighting.
    //std::cout << "--------- proxyView size():" << std::to_string(proxyModel->rowCount()) << std::endl;

    // 'proxyView' is a pointer to the QTreeView and 'selectionModel' is the model used
    QItemSelectionModel* selectionModel = proxyView->selectionModel();

    // Clear the previous selection
    selectionModel->clearSelection();

    for (int i = 0; i < (int)proxyModel->rowCount(); i++) {

      QModelIndex index = proxyModel->index(0, 0); // the first column of the first row

      // select all rows whose Stroke value matches the selected strokes.
      int strokeValue = proxyModel->index(i, 0).data().toInt();
      //std::cout << "--------------- strokeValue for row:" << i << " is:" << strokeValue << std::endl;
      if (strokeSelection->isSelected(strokeValue)){
        index = proxyModel->index(i, 0);
        selectionModel->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
      }
    }
  }
  else {
    //selectionModel->clearSelection();
  }
}

//-----------------------------------------------------------------------------
void VectorInspectorPanel::showEvent(QShowEvent *) {
  connect(TApp::instance()->getCurrentLevel(),
          SIGNAL(xshLevelSwitched(TXshLevel *)), this, SLOT(onLevelChanged()));

  connect(TApp::instance()->getCurrentFrame(), SIGNAL(frameSwitched()), this,
          SLOT(onLevelChanged()));

  connect(TApp::instance()->getCurrentSelection(),
          SIGNAL(selectionSwitched(TSelection *, TSelection *)), this,
          SLOT(onSelectionSwitched(TSelection *, TSelection *)));

  connect(TApp::instance()->getCurrentSelection(),
          SIGNAL(selectionChanged(TSelection *)), this,
          SLOT(onSelectionChanged(TSelection *)));

  onLevelChanged();
}

//-----------------------------------------------------------------------------

void VectorInspectorPanel::hideEvent(QHideEvent *) {
  disconnect(TApp::instance()->getCurrentLevel(),
             SIGNAL(xshLevelSwitched(TXshLevel *)), this,
             SLOT(onLevelChanged()));

  disconnect(TApp::instance()->getCurrentFrame(), SIGNAL(frameSwitched()), this,
             SLOT(onLevelChanged()));

  disconnect(TApp::instance()->getCurrentSelection(),
             SIGNAL(selectionSwitched(TSelection *)), this,
             SLOT(onSelectionSwitched(TSelection *)));

  disconnect(TApp::instance()->getCurrentSelection(),
             SIGNAL(selectionChanged(TSelection *)), this,
             SLOT(onSelectionChanged(TSelection *)));
}

void VectorInspectorPanel::setSourceModel(QAbstractItemModel *model)
{
   proxyModel->setSourceModel(model);
}

void VectorInspectorPanel::filterRegExpChanged()
{
   QRegExp::PatternSyntax syntax =
       QRegExp::PatternSyntax(filterSyntaxComboBox->itemData(
                                                      filterSyntaxComboBox->currentIndex()).toInt());
   Qt::CaseSensitivity caseSensitivity =
       filterCaseSensitivityCheckBox->isChecked() ? Qt::CaseSensitive
                                                  : Qt::CaseInsensitive;

   QRegExp regExp(filterPatternLineEdit->text(), caseSensitivity, syntax);
   proxyModel->setFilterRegExp(regExp);
}

void VectorInspectorPanel::filterColumnChanged()
{
   proxyModel->setFilterKeyColumn(filterColumnComboBox->currentIndex());
}

void VectorInspectorPanel::sortChanged()
{
   proxyModel->setSortCaseSensitivity(
       sortCaseSensitivityCheckBox->isChecked() ? Qt::CaseSensitive
                                                : Qt::CaseInsensitive);
}

void VectorInspectorPanel::copySelectedItemsToClipboard() {
   QModelIndexList indexes = proxyView->selectionModel()->selectedIndexes();

   QString selectedText;
   QMap<int, QString> rows;  // Use a map to sort by row number automatically

   // Collect data from each selected cell, grouped by row
   for (const QModelIndex &index : qAsConst(indexes)) {
       rows[index.row()] += index.data(Qt::DisplayRole).toString() + "\t";  // Tab-separated values
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
   connect(copyAction, &QAction::triggered, this, &VectorInspectorPanel::copySelectedItemsToClipboard);

   // Show the menu at the cursor position
   menu.exec(event->globalPos());
}

void VectorInspectorPanel::showContextMenu(const QPoint &pos) {
   QPoint globalPos = proxyView->viewport()->mapToGlobal(pos);
   QMenu menu;
   QAction *copyAction = menu.addAction(tr("Copy"));
   connect(copyAction, &QAction::triggered, this, &VectorInspectorPanel::copySelectedItemsToClipboard);

   menu.exec(globalPos);
}
