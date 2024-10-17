

// TnzCore includes
#include "tpixelutils.h"
#include "tstroke.h"
#include "tofflinegl.h"
#include "tstencilcontrol.h"
#include "tvectorgl.h"
#include "tvectorrenderdata.h"
#include "tcolorfunctions.h"
#include "tpalette.h"
#include "tropcm.h"
#include "trasterimage.h"
#include "tvectorimage.h"
#include "../../sources/common/tvectorimage/tvectorimageP.h"
#include "tmeshimage.h"
#include "tcolorstyles.h"
#include "timage_io.h"
#include "tregion.h"
#include "toonz/toonzscene.h"

#include "tcurves.h"

// TnzBase includes
#include "tenv.h"

// TnzExt includes
#include "ext/meshutils.h"
#include "ext/plasticskeleton.h"
#include "ext/plasticskeletondeformation.h"
#include "ext/plasticdeformerstorage.h"

// TnzLib includes
#include "toonz/stageplayer.h"
#include "toonz/stage.h"
#include "toonz/stage2.h"
#include "toonz/tcolumnfx.h"
#include "toonz/txsheet.h"
#include "toonz/txshsimplelevel.h"
#include "toonz/txshchildlevel.h"
#include "toonz/txshcolumn.h"
#include "toonz/txshcell.h"
#include "toonz/onionskinmask.h"
#include "toonz/dpiscale.h"
#include "toonz/imagemanager.h"
#include "toonz/tstageobjecttree.h"
#include "toonz/glrasterpainter.h"
#include "toonz/preferences.h"
#include "toonz/fill.h"
#include "toonz/levelproperties.h"
#include "toonz/autoclose.h"
#include "toonz/txshleveltypes.h"
#include "imagebuilders.h"
#include "toonz/tframehandle.h"
#include "toonz/preferences.h"

// Qt includes
#include <QImage>
#include <QPainter>
#include <QPolygon>
#include <QThreadStorage>
#include <QMatrix>
#include <QThread>
#include <QGuiApplication>

#include "toonz/stagevisitor.h"
#include "../common/tvectorimage/tvectorimageP.h"

bool debug_mode = false;  // Set to false to disable debug output
#define DEBUG_LOG(x) if (debug_mode) std::cout << x // << std::endl


//**********************************************************************************************
//    Stage namespace
//**********************************************************************************************

/*! \namespace Stage
    \brief The Stage namespace provides objects, classes and methods useful to
   view or display images.
*/

using namespace Stage;

/*! \var Stage::inch
                For historical reasons camera stand is defined in a coordinate
   system in which
                an inch is equal to 'Stage::inch' unit.
                Pay attention: modify this value condition apparent line
   thickness of
                images .pli.
*/
// const double Stage::inch = 53.33333;

//**********************************************************************************************
//    Local namespace
//**********************************************************************************************

namespace {

QImage rasterToQImage(const TRaster32P &ras) {
  QImage image(ras->getRawData(), ras->getLx(), ras->getLy(),
               QImage::Format_ARGB32_Premultiplied);
  return image;
}

//----------------------------------------------------------------

QImage rasterToQImage(const TRasterGR8P &ras) {
  QImage image(ras->getLx(), ras->getLy(), QImage::Format_ARGB32_Premultiplied);
  int lx = ras->getLx(), ly = ras->getLy();

  for (int y = 0; y < ly; y++) {
    TPixelGR8 *pix    = ras->pixels(y);
    TPixelGR8 *endPix = pix + lx;
    QRgb *outPix      = (QRgb *)image.scanLine(y);
    for (; pix < endPix; ++pix) {
      int value = pix->value;
      *outPix++ = qRgba(value, value, value, 255);
    }
  }
  return image;
}

//----------------------------------------------------------------

QImage rasterToQImage(const TRasterP &ras) {
  if (TRaster32P src32 = ras)
    return rasterToQImage(src32);
  else if (TRasterGR8P srcGr8 = ras)
    return rasterToQImage(srcGr8);

  // assert(!"Cannot use drawImage with this image!");
  return QImage();
}

//----------------------------------------------------------------

//! Draw orthogonal projection of \b bbox onto x-axis and y-axis.
void draw3DShadow(const TRectD &bbox, double z, double phi) {
  // bruttino assai, ammetto

  double a = bigBoxSize[0];
  double b = bigBoxSize[1];

  glColor3d(0.9, 0.9, 0.86);
  glBegin(GL_LINE_STRIP);
  glVertex3d(bbox.x0, bbox.y0, z);
  glVertex3d(bbox.x0, bbox.y1, z);
  glVertex3d(bbox.x1, bbox.y1, z);
  glVertex3d(bbox.x1, bbox.y0, z);
  glVertex3d(bbox.x0, bbox.y0, z);
  glEnd();

  double y = -b;
  double x = phi >= 0 ? a : -a;

  double xm = 0.5 * (bbox.x0 + bbox.x1);
  double ym = 0.5 * (bbox.y0 + bbox.y1);

  if (bbox.y0 > y) {
    glBegin(GL_LINE_STRIP);
    glVertex3d(xm, y, z);
    glVertex3d(xm, bbox.y0, z);
    glEnd();
  } else if (bbox.y1 < y) {
    glBegin(GL_LINE_STRIP);
    glVertex3d(xm, y, z);
    glVertex3d(xm, bbox.y1, z);
    glEnd();
  }

  if (bbox.x0 > x) {
    glBegin(GL_LINE_STRIP);
    glVertex3d(x, ym, z);
    glVertex3d(bbox.x0, ym, z);
    glEnd();
  } else if (bbox.x1 < x) {
    glBegin(GL_LINE_STRIP);
    glVertex3d(x, ym, z);
    glVertex3d(bbox.x1, ym, z);
    glEnd();
  }

  glColor3d(0.0, 0.0, 0.0);

  glBegin(GL_LINE_STRIP);
  glVertex3d(bbox.x0, -b, z);
  glVertex3d(bbox.x1, -b, z);
  glEnd();

  glBegin(GL_LINE_STRIP);
  glVertex3d(x, bbox.y0, z);
  glVertex3d(x, bbox.y1, z);
  glEnd();
}

//=====================================================================

//  Plastic function declarations

/*!
  Returns from the specified player the stage object to be plastic
  deformed - or 0 if current Toonz rules prevent it from being deformed.
*/
TStageObject *plasticDeformedObj(const Stage::Player &player,
                                 const PlasticVisualSettings &pvs);

//! Draws the specified mesh image
void onMeshImage(TMeshImage *mi, const Stage::Player &player,
                 const ImagePainter::VisualSettings &vs,
                 const TAffine &viewAff);

//! Applies Plastic deformation of the specified player's stage object.
void onPlasticDeformedImage(TStageObject *playerObj,
                            const Stage::Player &player,
                            const ImagePainter::VisualSettings &vs,
                            const TAffine &viewAff);

}  // namespace

//**********************************************************************************************
//    Picker  implementation
//**********************************************************************************************

Picker::Picker(const TAffine &viewAff, const TPointD &point,
               const ImagePainter::VisualSettings &vs, int devPixRatio)
    : Visitor(vs)
    , m_viewAff(viewAff)
    , m_point(point)
    , m_columnIndexes()
    , m_minDist2(25.0)
    , m_devPixRatio(devPixRatio) {}

//-----------------------------------------------------------------------------

void Picker::setMinimumDistance(double d) {
  m_minDist2 = (double)(m_devPixRatio * m_devPixRatio) * d * d;

}

//-----------------------------------------------------------------------------

void Picker::onImage(const Stage::Player &player) {
  // if m_currentColumnIndex is other than the default value (-1),
  // then pick only the current column.
  if (m_currentColumnIndex != -1 &&
      m_currentColumnIndex != player.m_ancestorColumnIndex)
    return;

  bool picked   = false;
  TAffine aff   = m_viewAff * player.m_placement;
  TPointD point = aff.inv() * m_point;

  const TImageP &img = player.image();

  if (TVectorImageP vi = img) {
    double w         = 0;
    UINT strokeIndex = 0;
    double dist2     = 0;
    TRegion *r       = vi->getRegion(point);
    int styleId      = 0;
    if (r) styleId   = r->getStyle();
    if (styleId != 0)
      picked = true;
    else if (vi->getNearestStroke(point, w, strokeIndex, dist2)) {
      dist2 *= aff.det();

      TStroke *stroke        = vi->getStroke(strokeIndex);
      TThickPoint thickPoint = stroke->getThickPoint(w);
      double len2 = thickPoint.thick * thickPoint.thick * aff.det();
      double checkDist = std::max(m_minDist2, len2);
      if (dist2 < checkDist) picked = true;
    }
  } else if (TRasterImageP ri = img) {
    TRaster32P ras = ri->getRaster();
    if (!ras) return;

    ras->lock();
    TPointD pp = player.m_dpiAff.inv() * point + ras->getCenterD();
    TPoint p(tround(pp.x), tround(pp.y));
    if (!ras->getBounds().contains(p)) return;

    TPixel32 *pix               = ras->pixels(p.y);
    if (pix[p.x].m != 0) picked = true;

    TAffine aff2 = (aff * player.m_dpiAff).inv();

    TPointD pa(p.x, p.y);
    TPointD dx = aff2 * (m_point + TPointD(3, 0)) - aff2 * m_point;
    TPointD dy = aff2 * (m_point + TPointD(0, 3)) - aff2 * m_point;
    double rx  = dx.x * dx.x + dx.y * dx.y;
    double ry  = dy.x * dy.x + dy.y * dy.y;
    int radius = tround(sqrt(rx > ry ? rx : ry));
    TRect rect = TRect(p.x - radius, p.y - radius, p.x + radius, p.y + radius) *
                 ras->getBounds();
    for (int y = rect.y0; !picked && y <= rect.y1; y++) {
      pix = ras->pixels(y);
      for (int x                  = rect.x0; !picked && x <= rect.x1; x++)
        if (pix[x].m != 0) picked = true;
    }

    ras->unlock();
  } else if (TToonzImageP ti = img) {
    TRasterCM32P ras = ti->getRaster();
    if (!ras) return;

    ras->lock();
    TPointD pp = player.m_dpiAff.inv() * point + ras->getCenterD();
    TPoint p(tround(pp.x), tround(pp.y));
    if (!ras->getBounds().contains(p)) return;

    TPixelCM32 *pix = ras->pixels(p.y) + p.x;
    if (!pix->isPurePaint() || pix->getPaint() != 0) picked = true;

    ras->unlock();
  }

  if (picked) {
    int columnIndex = player.m_ancestorColumnIndex;
    if (m_columnIndexes.empty() || m_columnIndexes.back() != columnIndex)
      m_columnIndexes.push_back(columnIndex);

    int row = player.m_frame;
    if (m_rows.empty() || m_rows.back() != row) m_rows.push_back(row);
  }
}

//-----------------------------------------------------------------------------

void Picker::beginMask() {}

//-----------------------------------------------------------------------------

void Picker::endMask() {}

//-----------------------------------------------------------------------------

void Picker::enableMask(TStencilControl::MaskType maskType) {}

//-----------------------------------------------------------------------------

void Picker::disableMask() {}

//-----------------------------------------------------------------------------

int Picker::getColumnIndex() const {
  if (m_columnIndexes.empty())
    return -1;
  else
    return m_columnIndexes.back();
}

//-----------------------------------------------------------------------------

void Picker::getColumnIndexes(std::vector<int> &indexes) const {
  indexes = m_columnIndexes;
}
//-----------------------------------------------------------------------------

int Picker::getRow() const {
  if (m_rows.empty())
    return -1;
  else
    return m_rows.back();
}

//**********************************************************************************************
//    RasterPainter  implementation
//**********************************************************************************************

RasterPainter::RasterPainter(const TDimension &dim, const TAffine &viewAff,
                             const TRect &rect,
                             const ImagePainter::VisualSettings &vs,
                             bool checkFlags)
    : Visitor(vs)
    , m_dim(dim)
    , m_viewAff(viewAff)
    , m_clipRect(rect)
    , m_maskLevel(0)
    , m_singleColumnEnabled(false)
    , m_checkFlags(checkFlags)
    , m_doRasterDarkenBlendedView(false) {}

//-----------------------------------------------------------------------------

//! Utilizzato solo per TAB Pro
void RasterPainter::beginMask() {
  flushRasterImages();  // per evitare che venga fatto dopo il beginMask
  ++m_maskLevel;
  TStencilControl::instance()->beginMask();
}
//! Utilizzato solo per TAB Pro
void RasterPainter::endMask() {
  flushRasterImages();  // se ci sono delle immagini raster nella maschera
                        // devono uscire ora
  --m_maskLevel;
  TStencilControl::instance()->endMask();
}
//! Utilizzato solo per TAB Pro
void RasterPainter::enableMask(TStencilControl::MaskType maskType) {
  TStencilControl::instance()->enableMask(maskType);
}
//! Utilizzato solo per TAB Pro
void RasterPainter::disableMask() {
  flushRasterImages();  // se ci sono delle immagini raster mascherate devono
                        // uscire ora
  TStencilControl::instance()->disableMask();
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

TEnv::DoubleVar AutocloseDistance("InknpaintAutocloseDistance", 10.0);
TEnv::DoubleVar AutocloseAngle("InknpaintAutocloseAngle", 60.0);
TEnv::IntVar AutocloseInk("InknpaintAutocloseInk", 1);
TEnv::IntVar AutocloseOpacity("InknpaintAutocloseOpacity", 255);

//-----------------------------------------------------------------------------

int RasterPainter::getNodesCount() { return m_nodes.size(); }

//-----------------------------------------------------------------------------

void RasterPainter::clearNodes() { m_nodes.clear(); }

//-----------------------------------------------------------------------------

TRasterP RasterPainter::getRaster(int index, QMatrix &matrix) {
  if ((int)m_nodes.size() <= index) return TRasterP();

  if (m_nodes[index].m_onionMode != Node::eOnionSkinNone) return TRasterP();

  if (m_nodes.empty()) return TRasterP();

  double delta = sqrt(fabs(m_nodes[0].m_aff.det()));
  TRectD bbox  = m_nodes[0].m_bbox.enlarge(delta);

  int i;
  for (i = 1; i < (int)m_nodes.size(); i++) {
    delta = sqrt(fabs(m_nodes[i].m_aff.det()));
    bbox += m_nodes[i].m_bbox.enlarge(delta);
  }
  TRect rect(tfloor(bbox.x0), tfloor(bbox.y0), tceil(bbox.x1), tceil(bbox.y1));
  rect = rect * TRect(0, 0, m_dim.lx - 1, m_dim.ly - 1);

  TAffine aff = TTranslation(-rect.x0, -rect.y0) * m_nodes[index].m_aff;
  matrix      = QMatrix(aff.a11, aff.a21, aff.a12, aff.a22, aff.a13, aff.a23);

  return m_nodes[index].m_raster;
}

//-----------------------------------------------------------------------------

/*! Make frame visualization.
\n	If onon-skin is active, create a new raster with dimension containing
all
                frame with onion-skin; recall \b TRop::quickPut with argument
each frame
                with onion-skin and new raster. If onion-skin is not active
recall
                \b TRop::quickPut with argument current frame and new raster.
*/

namespace {
QThreadStorage<std::vector<char> *> threadBuffers;
}

void RasterPainter::flushRasterImages() {
  if (m_nodes.empty()) return;

  // Build nodes bbox union
  double delta = sqrt(fabs(m_nodes[0].m_aff.det()));
  TRectD bbox  = m_nodes[0].m_bbox.enlarge(delta);

  int i, nodesCount = m_nodes.size();
  for (i = 1; i < nodesCount; ++i) {
    delta = sqrt(fabs(m_nodes[i].m_aff.det()));
    bbox += m_nodes[i].m_bbox.enlarge(delta);
  }

  TRect rect(tfloor(bbox.x0), tfloor(bbox.y0), tceil(bbox.x1), tceil(bbox.y1));
  rect = rect * TRect(0, 0, m_dim.lx - 1, m_dim.ly - 1);

  int lx = rect.getLx(), ly = rect.getLy();
  TDimension dim(lx, ly);

  // this is needed since a stop motion live view
  // doesn't register as a node correctly
  // there is probably a better way to do this.
  if (rect.getLx() == 0 && lx == 0) {
    rect = m_clipRect;
    dim  = m_dim;
  }

  // Build a raster buffer of sufficient size to hold said union.
  // The buffer is per-thread cached in order to improve the rendering speed.
  if (!threadBuffers.hasLocalData())
    threadBuffers.setLocalData(new std::vector<char>());

  int size = dim.lx * dim.ly * sizeof(TPixel32);

  std::vector<char> *vbuff = (std::vector<char> *)threadBuffers.localData();
  if (size > (int)vbuff->size()) vbuff->resize(size);

  TRaster32P ras(dim.lx, dim.ly, dim.lx, (TPixel32 *)&(*vbuff)[0]);
  TRaster32P ras2;

  if (m_vs.m_colorMask != 0) {
    ras2 = TRaster32P(ras->getSize());
    ras->clear();
  } else
    ras2 = ras;

  // Clear the buffer - it will hold all the stacked nodes content to be overed
  // on top of the OpenGL buffer through a glDrawPixel()
  ras->lock();

  ras->clear();  // ras is typically reused - and we need it transparent first

  TRect r                 = rect - rect.getP00();
  TRaster32P viewedRaster = ras->extract(r);

  int current = -1;

  // Retrieve preferences-related data
  int tc    = m_checkFlags ? ToonzCheck::instance()->getChecks() : 0;
  int index = ToonzCheck::instance()->getColorIndex();

  TPixel32 frontOnionColor, backOnionColor;
  bool onionInksOnly;

  Preferences::instance()->getOnionData(frontOnionColor, backOnionColor,
                                        onionInksOnly);

  // Stack every node on top of the raster buffer
  for (i = 0; i < nodesCount; ++i) {
    if (m_nodes[i].m_isCurrentColumn) current = i;

    TAffine aff         = TTranslation(-rect.x0, -rect.y0) * m_nodes[i].m_aff;
    TDimension imageDim = m_nodes[i].m_raster->getSize();
    TPointD offset(0.5, 0.5);
    aff *= TTranslation(offset);  // very quick and very dirty fix: in
                                  // camerastand the images seems shifted of an
                                  // half pixel...it's a quickput approximation?

    TPixel32 colorscale = TPixel32(0, 0, 0, m_nodes[i].m_alpha);
    int inksOnly;

    if (m_nodes[i].m_onionMode != Node::eOnionSkinNone) {
      inksOnly = onionInksOnly;

      if (m_nodes[i].m_onionMode == Node::eOnionSkinFront)
        colorscale = TPixel32(frontOnionColor.r, frontOnionColor.g,
                              frontOnionColor.b, m_nodes[i].m_alpha);
      else if (m_nodes[i].m_onionMode == Node::eOnionSkinBack)
        colorscale = TPixel32(backOnionColor.r, backOnionColor.g,
                              backOnionColor.b, m_nodes[i].m_alpha);
    } else {
      if (m_nodes[i].m_filterColor != TPixel32::Black) {
        colorscale   = m_nodes[i].m_filterColor;
        colorscale.m = (typename TPixel32::Channel)((int)colorscale.m *
                                                    (int)m_nodes[i].m_alpha /
                                                    TPixel32::maxChannelValue);
      }
      inksOnly = tc & ToonzCheck::eInksOnly;
    }

    if (TRaster32P src32 = m_nodes[i].m_raster)
      TRop::quickPut(viewedRaster, src32, aff, colorscale,
                     m_nodes[i].m_doPremultiply, m_nodes[i].m_whiteTransp,
                     m_nodes[i].m_isFirstColumn, m_doRasterDarkenBlendedView);
    else if (TRasterGR8P srcGr8 = m_nodes[i].m_raster)
      TRop::quickPut(viewedRaster, srcGr8, aff, colorscale);
    else if (TRasterCM32P srcCm = m_nodes[i].m_raster) {
      assert(m_nodes[i].m_palette);
      int oldframe = m_nodes[i].m_palette->getFrame();
      m_nodes[i].m_palette->setFrame(m_nodes[i].m_frame);

      TPaletteP plt;
      int styleIndex = -1;
      if ((tc & ToonzCheck::eGap || tc & ToonzCheck::eAutoclose) &&
          m_nodes[i].m_isCurrentColumn) {
        srcCm      = srcCm->clone();
        plt        = m_nodes[i].m_palette->clone();
        styleIndex = plt->addStyle(TPixel::Magenta);
        if (tc & ToonzCheck::eAutoclose)
          TAutocloser(srcCm, AutocloseDistance, AutocloseAngle, styleIndex,
                      AutocloseOpacity)
              .exec();
        if (tc & ToonzCheck::eGap)
          AreaFiller(srcCm).rectFill(m_nodes[i].m_savebox, 1, true, true,
                                     false);
      } else
        plt = m_nodes[i].m_palette;

      if (tc == 0 || tc == ToonzCheck::eBlackBg ||
          !m_nodes[i].m_isCurrentColumn)
        TRop::quickPut(viewedRaster, srcCm, plt, aff, colorscale, inksOnly);
      else {
        TRop::CmappedQuickputSettings settings;

        settings.m_globalColorScale = colorscale;
        settings.m_inksOnly         = inksOnly;
        settings.m_transparencyCheck =
            tc & (ToonzCheck::eTransparency | ToonzCheck::eGap);
        settings.m_blackBgCheck = tc & ToonzCheck::eBlackBg;
        /*-- InkCheck, Ink#1Check, PaintCheckはカレントカラムにのみ有効 --*/
        settings.m_inkIndex =
            m_nodes[i].m_isCurrentColumn
                ? (tc & ToonzCheck::eInk ? index
                                         : (tc & ToonzCheck::eInk1 ? 1 : -1))
                : -1;
        settings.m_paintIndex = m_nodes[i].m_isCurrentColumn
                                    ? (tc & ToonzCheck::ePaint ? index : -1)
                                    : -1;

        Preferences::instance()->getTranspCheckData(
            settings.m_transpCheckBg, settings.m_transpCheckInk,
            settings.m_transpCheckPaint);

        settings.m_isOnionSkin = m_nodes[i].m_onionMode != Node::eOnionSkinNone;
        settings.m_gapCheckIndex = styleIndex;

        TRop::quickPut(viewedRaster, srcCm, plt, aff, settings);
      }

      srcCm = TRasterCM32P();
      plt   = TPaletteP();

      m_nodes[i].m_palette->setFrame(oldframe);
    } else
      assert(!"Cannot use quickput with this raster combination!");
  }

  if (m_vs.m_colorMask != 0) {
    TRop::setChannel(ras, ras, m_vs.m_colorMask, false);
    TRop::quickPut(ras2, ras, TAffine());
  }

  // Now, output the raster buffer on top of the OpenGL buffer
  glPushAttrib(GL_COLOR_BUFFER_BIT);  // Preserve blending and stuff

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE,
              GL_ONE_MINUS_SRC_ALPHA);  // The raster buffer is intended in
  // premultiplied form - thus the GL_ONE on src
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_DITHER);
  glDisable(GL_LOGIC_OP);

/* disable, since these features are never enabled, and cause OpenGL to assert
 * on systems that don't support them: see #591 */
#if 0
#ifdef GL_EXT_convolution
  if( GLEW_EXT_convolution ) {
    glDisable(GL_CONVOLUTION_1D_EXT);
    glDisable(GL_CONVOLUTION_2D_EXT);
    glDisable(GL_SEPARABLE_2D_EXT);
  }
#endif

#ifdef GL_EXT_histogram
  if( GLEW_EXT_histogram ) {
    glDisable(GL_HISTOGRAM_EXT);
    glDisable(GL_MINMAX_EXT);
  }
#endif
#endif

#ifdef GL_EXT_texture3D
  if (GL_EXT_texture3D) {
    glDisable(GL_TEXTURE_3D_EXT);
  }
#endif

  glPushMatrix();
  glLoadIdentity();

  glRasterPos2d(rect.x0, rect.y0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glDrawPixels(ras2->getLx(), ras2->getLy(),            // Perform the over
               TGL_FMT, TGL_TYPE, ras2->getRawData());  //

  ras->unlock();
  glPopMatrix();

  glPopAttrib();  // Restore blending status

  if (m_vs.m_showBBox && current > -1) {
    glPushMatrix();
    glLoadIdentity();
    tglColor(TPixel(200, 200, 200));
    tglMultMatrix(m_nodes[current].m_aff);
    tglDrawRect(m_nodes[current].m_raster->getBounds());
    glPopMatrix();
  }

  m_nodes.clear();
}

//-----------------------------------------------------------------------------
/*! Make frame visualization in QPainter.
\n	Draw in painter mode just raster image in m_nodes.
\n  Onon-skin or channel mode are not considered.
*/
void RasterPainter::drawRasterImages(QPainter &p, QPolygon cameraPol) {
  if (m_nodes.empty()) return;

  double delta = sqrt(fabs(m_nodes[0].m_aff.det()));
  TRectD bbox  = m_nodes[0].m_bbox.enlarge(delta);

  int i;
  for (i = 1; i < (int)m_nodes.size(); i++) {
    delta = sqrt(fabs(m_nodes[i].m_aff.det()));
    bbox += m_nodes[i].m_bbox.enlarge(delta);
  }
  TRect rect(tfloor(bbox.x0), tfloor(bbox.y0), tceil(bbox.x1), tceil(bbox.y1));
  rect = rect * TRect(0, 0, m_dim.lx - 1, m_dim.ly - 1);

  TRect r = rect - rect.getP00();
  TAffine flipY(1, 0, 0, 0, -1, m_dim.ly);
  p.setClipRegion(QRegion(cameraPol));
  for (i = 0; i < (int)m_nodes.size(); i++) {
    if (m_nodes[i].m_onionMode != Node::eOnionSkinNone) continue;
    p.resetTransform();
    TRasterP ras = m_nodes[i].m_raster;
    TAffine aff  = TTranslation(-rect.x0, -rect.y0) * flipY * m_nodes[i].m_aff;
    QMatrix matrix(aff.a11, aff.a21, aff.a12, aff.a22, aff.a13, aff.a23);
    QImage image = rasterToQImage(ras);
    if (image.isNull()) continue;
    p.setMatrix(matrix);
    p.drawImage(rect.getP00().x, rect.getP00().y, image);
  }

  p.resetTransform();
  m_nodes.clear();
}

static void buildAutocloseImage(
    TVectorImage *vaux, TVectorImage *vi,
    const std::vector<std::pair<int, double>> &startPoints,
    const std::vector<std::pair<int, double>> &endPoints) {
  DEBUG_LOG("buildAutocloseImage\n");
  //std::cout << "buildAutocloseImage\n";
  for (UINT i = 0; i < startPoints.size(); i++) {
    TThickPoint p1 = vi->getStroke(startPoints[i].first)
                         ->getThickPoint(startPoints[i].second);
    TThickPoint p2 =
        vi->getStroke(endPoints[i].first)->getThickPoint(endPoints[i].second);
    std::vector<TThickPoint> points(3);
    points[0]       = p1;
    points[1]       = 0.5 * (p1 + p2);
    points[2]       = p2;
    points[0].thick = points[1].thick = points[2].thick = 0.0;
    TStroke *auxStroke                                  = new TStroke(points);
    auxStroke->setStyle(2);
    vaux->addStroke(auxStroke);
  }
}

TEnv::DoubleVar AutocloseFactor("InknpaintAutocloseFactor", 4.0);

static void drawAutocloses(TVectorImage *vi, TVectorRenderData &rd) {
  static TPalette *plt = 0;
  if (!plt) {
    plt = new TPalette();
    plt->addStyle(TPixel::Magenta);
  }

  std::vector<std::pair<int, double>> startPoints, endPoints;
  getClosingPoints(vi->getBBox(), AutocloseFactor, vi, startPoints, endPoints);
  TVectorImage *vaux = new TVectorImage();

  rd.m_palette = plt;
  buildAutocloseImage(vaux, vi, startPoints, endPoints);
  // temporarily disable fill check, to preserve the gap indicator color
  bool tCheckEnabledOriginal = rd.m_tcheckEnabled;
  rd.m_tcheckEnabled         = false;
  // draw
  tglDraw(rd, vaux);
  // restore original value
  rd.m_tcheckEnabled = tCheckEnabledOriginal;
  delete vaux;
}

//-----------------------------------------------------------------------------

  std::pair<double, double> extendQuadraticBezier(std::pair<double, double> P0,
                                                std::pair<double, double> P1,
                                                std::pair<double, double> P2,
                                                double L,
                                                bool reverse = false) {
  double dx, dy, length, unit_dx, unit_dy, x3, y3;

  if (reverse) {
    // Calculate the tangent vector at the start point P0
    dx = P1.first - P0.first;
    dy = P1.second - P0.second;
  } else {
    // Calculate the tangent vector at the endpoint P2
    dx = P2.first - P1.first;
    dy = P2.second - P1.second;
  }

  // Calculate the magnitude of the tangent vector
  length = sqrt(dx * dx + dy * dy);

  // Normalize the tangent vector
  unit_dx = dx / length;
  unit_dy = dy / length;

  if (reverse) {
    // Calculate the new start point P3 by extending in the reverse direction
    x3 = P0.first - L * unit_dx;
    y3 = P0.second - L * unit_dy;
  } else {
    // Calculate the new endpoint P3 by extending in the forward direction
    x3 = P2.first + L * unit_dx;
    y3 = P2.second + L * unit_dy;
  }

  return std::make_pair(x3, y3);
}

//-----------------------------------------------------------------------------

  // Define a struct to hold the intersection data
  struct Intersection {
    UINT s1Index;
    bool s2IsExtension;
    UINT s2Index;
    double w;
    double x;
    double y;
    int survives;
  };


  void determineSurvivingIntersections(std::vector<Intersection>& intersections, std::vector<Intersection>& uniqueIntersections) {

    //Preparation: sort the list in s1 and W order.
    //All rows should have survivor value of 0 as default, meaning unknown.

    //Store a value for currentS1Index to keep track of when the s1Index changes.
    UINT currentS1Index;
    bool markRemainingNonSurviving = false;

    //First Pass =================================================================================================================

    // Resolve all intersections that do not have dependency on other intersections.
    for (auto& inter : intersections) {
      //	Is this the first row for this s1Index? 
      if (inter.s1Index != currentS1Index) {
        // Yes, this is the first row
        // Update currentS1Index.
        currentS1Index = inter.s1Index;
        // set markRemainingNonSurviving flag off.
        markRemainingNonSurviving = false;

        //The first record processed is the lowest W value for the first s1Index.
        //			Is s2 a line?
        if (!inter.s2IsExtension) {
          //	Yes
          //		s1 is lowest W value, s2 is a line - unambiguous survivor.
          //		mark as survivor = 1
          inter.survives = 1;
          //		mark remaining rows for this s1 as survivor = -1
          markRemainingNonSurviving = true;
        }
        else { // No, s2 is not a line
         // Is s2 the lowest W value for that index?
          auto it =
            std::find_if(uniqueIntersections.begin(), uniqueIntersections.end(),
              [&inter](const Intersection& uniqueInter) {
                return uniqueInter.s1Index == inter.s2Index && uniqueInter.s2Index == inter.s1Index;
              });

          if (it != uniqueIntersections.end()) {
            //Yes, lowest W for s2
            //	s1 is lowest W value, s2 is lowest W value - unambiguous survivor.
            // ====================================== check the length of the final gap close line that it is within gap distance ==========================
            //	mark as survivor = 1
            inter.survives = 1;
            //	mark remaining rows for this s1 as survivor = -1
            markRemainingNonSurviving = true;
          }
          else {
            //No, not lowest W for s2
            //indeterminate outcome in First Pass, wait for Second Pass
          }
        }
      }
      else { // No, not the first row for this s1Index.
        // Is the markRemainingNonSurviving flag set?
        if (markRemainingNonSurviving) {
          //	Yes
          //	cannot be a survivor, mark as survivor = -1.
          inter.survives = -1;

          // mark the corresponding s2 row as well
          auto it =
            std::find_if(intersections.begin(), intersections.end(),
              [&inter](Intersection& inter2) {
                return inter2.s1Index == inter.s2Index && inter2.s2Index == inter.s1Index;
              });

          if (it != intersections.end()) {

            it->survives = -1;
          }
          /*
          else {
            //				No, the markRemainingNonSurviving flag is not set
            //					Cannot prove unambiguous survivor, see if it is a survivor anyway…
                // Is s2 a line?
            if (!inter.s2IsExtension) {
              //						Yes
              //							Cannot be a survivor, mark as survivor = -1
              inter.survives = -1;
            }
            else {
              //	No, s2 is not a line
              //  	Is s2 its lowest W value?
              auto it =
                std::find_if(uniqueIntersections.begin(), uniqueIntersections.end(),
                  [&inter](const Intersection& uniqueInter) {
                    return uniqueInter.s1Index == inter.s2Index && uniqueInter.s2Index == inter.s1Index;
                  });

              if (it != uniqueIntersections.end()) {

                //Yes, s2 is its lowest W value
                //	s1 not lowest W value, s2 is lowest W value - survivor.
                //	mark as survivor = 1
                inter.survives = 1;
                //	mark remaining rows for this s1 as survivor = -1
                //	set markRemainingNonSurviving flag on.
                markRemainingNonSurviving = true;
              }
              else {
                //
                //	No, neither s1 nor s2 are at their lowest W value
                //	cannot be a survivor, mark as survivor = -1.
                inter.survives = -1;
              }
            }
          }
          */
        }
      }
    }

      //std::cout << "Pass 1 Results:" << "\n";
      DEBUG_LOG("Pass 1 Results:" << "\n");
      for (const auto& currIntersection : intersections) {
        //std::cout << "Intersection s1:" << currIntersection.s1Index << " isExt:" << currIntersection.s2IsExtension << " s2:" << currIntersection.s2Index << " w:" << currIntersection.w << " x:" << currIntersection.x << " y:" << currIntersection.y << " survives:" << currIntersection.survives << "\n";
        DEBUG_LOG("Intersection s1:" << currIntersection.s1Index << " isExt:" << currIntersection.s2IsExtension << " s2:" << currIntersection.s2Index << " s2W:" << currIntersection.w << " x:" << currIntersection.x << " y:" << currIntersection.y << " survives:" << currIntersection.survives << "\n");
      }


      currentS1Index = -1;
      markRemainingNonSurviving = false;

      //Second Pass - resolve all unresolved intersections.  ======================================================================================
      for (auto& inter : intersections) {
        //std::cout << inter.s1Index << " - " << inter.s2Index << ", s2IsExtension:" << inter.s2IsExtension << "\n";
        DEBUG_LOG(inter.s1Index << " - " << inter.s2Index << ", s2IsExtension:" << inter.s2IsExtension << "\n");
        //	Is this the first row for this s1Index? 
        if (inter.s1Index != currentS1Index) {
          // Yes, this is the first row
          // Update currentS1Index.
          currentS1Index = inter.s1Index;
          // set markRemainingNonSurviving flag off.
          markRemainingNonSurviving = false;


          if(inter.survives == 0){

          //The first record processed is the lowest W value for the first s1Index.
          //			Is s2 a line?
          if (!inter.s2IsExtension) {
            //	Yes
            //		s1 is lowest W value, s2 is a line - unambiguous survivor.
            //		mark as survivor = 1
            inter.survives = 1;
            //		mark remaining rows for this s1 as survivor = -1
            markRemainingNonSurviving = true;
          }
          else { // No, s2 is not a line
           // Is s2 the lowest W value for that index?
            auto it =
              std::find_if(uniqueIntersections.begin(), uniqueIntersections.end(),
                [&inter](const Intersection& uniqueInter) {
                  return uniqueInter.s1Index == inter.s2Index && uniqueInter.s2Index == inter.s1Index;
                });

            if (it != uniqueIntersections.end()) { //row found
              //Yes, lowest W for s2
              //	s1 is lowest W value, s2 is lowest W value - unambiguous survivor.
              //	mark as survivor = 1

              // does the corresponding s2 row survive?
              auto it2 =
                std::find_if(intersections.begin(), intersections.end(),
                  [&inter](Intersection& inter2) {
                    return inter2.s1Index == inter.s2Index && inter2.s2Index == inter.s1Index;
                  });

              if (it2 != uniqueIntersections.end()) {

                if (it2->survives == -1) {
                  inter.survives = -1;
                  //std::cout << "I am lowest W for s1, " << it->s1Index << " - " << it->s2Index << " is not surviving:" << it->survives << " so I am non-surviving." << "\n";
                  DEBUG_LOG("I am lowest W for s1, " << it->s1Index << " - " << it->s2Index << " is not surviving:" << it->survives << " so I am non-surviving." << "\n");
                }
                else { //survives is 0 or 1
                  inter.survives = 1;
                  //	mark remaining rows for this s1 as survivor = -1
                  markRemainingNonSurviving = true;
                }
              }
            }
            else { //No, not found in uniqueIntersectiosn so not lowest W for s2
              //std::cout << "I am first row for s1, not lowest W for s2, so " << "\n";
              DEBUG_LOG("I am first row for s1, not lowest W for s2, so " << "\n");
              // is my corresponding row a survivor?
              auto it2 =
                std::find_if(intersections.begin(), intersections.end(),
                  [&inter](const Intersection& inter2) {
                    return inter2.s1Index == inter.s2Index && inter2.s2Index == inter.s1Index;
                  });
              if (it2 != intersections.end()) {
                if (it2->survives == -1){ // s2 was marked as non survivor
                  inter.survives = -1;
                }else if (it2->survives == 1){ // s2 was marked as survivor
                  inter.survives = 1;
                  markRemainingNonSurviving = true;
                }
                else if (it2->survives == 0) { // s2 was default 0
                  inter.survives = 1;
                  it2->survives = 1; // mark my s2 intersection
                  markRemainingNonSurviving = true;
                }
              }
            }
          }

          }
          else {
            //std::cout << "already processed, first row, , survives is: " << inter.survives << "\n";
            DEBUG_LOG("already processed, first row, , survives is: " << inter.survives << "\n");
          }




        }  //end for yes, first row of s1
        else { // No, not the first row for this s1Index.
          // Is the markRemainingNonSurviving flag set?

          if (inter.survives == 0) {

          if (markRemainingNonSurviving) {
            //	Yes
            //	cannot be a survivor, mark as survivor = -1.
            //std::cout << "markRemainingNonSurviving:" << markRemainingNonSurviving << " so I am non-surviving." << "\n";
            DEBUG_LOG("markRemainingNonSurviving:" << markRemainingNonSurviving << " so I am non-surviving." << "\n");
            inter.survives = -1;
            if (inter.s2IsExtension) { // for extension intersections...
            // set the corresponding s2 row.
              auto it =
                std::find_if(intersections.begin(), intersections.end(),
                  [&inter](const Intersection& inter2) {
                    return inter2.s1Index == inter.s2Index && inter2.s2Index == inter.s1Index;
                  });
              if (it != intersections.end()) {
                it->survives = -1;
                //std::cout << "set my corresponding s2 row " << it->s1Index << "-" << it->s2Index << " to non-surviving." << "\n";
                DEBUG_LOG("set my corresponding s2 row " << it->s1Index << "-" << it->s2Index << " to non-surviving." << "\n");
              }
            }
          }
          else { //	No, the markRemainingNonSurviving flag is not set
            // Cannot prove unambiguous survivor, see if it is a survivor anyway…
            // Is s2 a line?
            if (!inter.s2IsExtension) {
              //						Yes
              //							Cannot be a survivor with s1 not lowest W, mark as survivor = -1
              inter.survives = -1;
              //std::cout << "!inter.s2IsExtension:" << !inter.s2IsExtension << " a line, so I am non-surviving." << "\n";
              DEBUG_LOG("!inter.s2IsExtension:" << !inter.s2IsExtension << " a line, so I am non-surviving." << "\n");
            }
            else {
              //	No, s2 is not a line
              //  ** prior logic:	Is s2 its lowest W value?
              // Find the intersection from s2 direction, don't care about is it first row for that stroke or not.
              auto it =
                std::find_if(intersections.begin(), intersections.end(),
                  [&inter](const Intersection& inter2) {
                    return inter2.s1Index == inter.s2Index && inter2.s2Index == inter.s1Index;
                  });

              if (it != intersections.end()) { //row found
                // Is it already marked as non-surviving?
                if (it->survives == -1) { // is my corresponding row not a survivor?
                  //	s2 is not a survivor so neither am I								
                  //	No, neither s1 nor s2 are at their lowest W value
                  //	cannot be a survivor, mark as survivor = -1.
                  inter.survives = -1;
                  //std::cout << it->s1Index << " - " << it->s2Index << " is not surviving:" << it->survives << " so I am non-surviving." << "\n";
                  DEBUG_LOG(it->s1Index << " - " << it->s2Index << " is not surviving:" << it->survives << " so I am non-surviving." << "\n");
                }
                else { // corresponding row is a survivor or unconfirmed, so I am a survivor
                  // Do I need logic to determine if the intersected line row is the first surviving row for the stroke?
                  // Here I am assuming that if it is no already marked as non-surviving it is candidate for being a survivor.
                  //	**** Prior logic was: s1 not lowest W value, s2 is lowest W value - survivor.
                  //	mark as survivor = 1
                  inter.survives = 1;
                  // s2 also survives
                  it->survives = 1;
                  //	mark remaining rows for this s1 as survivor = -1
                  //	set markRemainingNonSurviving flag on.
                  markRemainingNonSurviving = true;
                  //std::cout << "mark as survivor, and set all remaining rows as non-survivors" << "\n";
                  DEBUG_LOG("mark as survivor, and set all remaining rows as non-survivors" << "\n");

                }
              }
              else {
                //std::cout << "Corresponding row " << inter.s2Index << "-" << inter.s1Index << " not found" << "\n";
                DEBUG_LOG("Corresponding row " << inter.s2Index << "-" << inter.s1Index << " not found" << "\n");
              }
            }
          }
          }
          else {
            //std::cout << "already processed, not first row, survives is: " << inter.survives << "\n";
            DEBUG_LOG("already processed, not first row, survives is: " << inter.survives << "\n");
          }
        }
      }
    
    //std::cout << "Pass 2 Results:" << "\n";
    DEBUG_LOG("Pass 2 Results:" << "\n");
    for (const auto& currIntersection : intersections) {
      //std::cout << "Intersection s1:" << currIntersection.s1Index << " isExt:" << currIntersection.s2IsExtension << " s2:" << currIntersection.s2Index << " w:" << currIntersection.w << " x:" << currIntersection.x << " y:" << currIntersection.y << " survives:" << currIntersection.survives << "\n";
      DEBUG_LOG("Intersection s1:" << currIntersection.s1Index << " isExt:" << currIntersection.s2IsExtension << " s2:" << currIntersection.s2Index << " s2W:" << currIntersection.w << " x:" << currIntersection.x << " y:" << currIntersection.y << " survives:" << currIntersection.survives << "\n");
    }
  }

/*
bool isSurvivingIntersection(std::vector<Intersection>&intersections, 
  std::vector<Intersection>& uniqueIntersections, 
  Intersection& currIntersection) {

  // take in a row from intersections, decide if it is a surviving intersection.
  // check the list of confirmed surviving intersections.
  // Confirmed? return true.
  //
  // unambiguous terminating conditions:
  //    s1 and s2 are their lowest W values, a perfect intersection.
  //    s1 is its lowest W value and s2 is a normal line, a perfect intersection.
  // 
  // If determined an intersection survives? 
  //     then add to the confirmed surviving intersections list.
  // 
  // No terminating condition? 
  //     s2 is not a confirmed intersection.
  //     make a recursive call until a terminating condition is reached.

  UINT s1Index = currIntersection.s1Index;
  bool s2IsExtension = currIntersection.s2IsExtension;
  UINT s2Index = currIntersection.s2Index;
  double w = currIntersection.w;
  double x = currIntersection.x;
  double y = currIntersection.y;
  int survives = currIntersection.survives;

  auto it =
      std::find_if(uniqueIntersections.begin(), uniqueIntersections.end(),
                   [&s1Index, &s2Index](const Intersection &inter) {
                     return inter.s1Index == s1Index && inter.s2Index == s2Index;
                   });

  if (it != uniqueIntersections.end()) {
    // this intersection has the lowest W value for the intersection
    if (!s2IsExtension) {
      // this intersection is with a line, , a perfect intersection
      currIntersection.survives = 1;
      return true;
    }
    else {
      if (s1Index > s2Index) { 
        // resolve duplicate lines by rejecting intersections from one direction, s1 higher than s2
        currIntersection.survives = -1;
        return false; 
      }
      it =
        std::find_if(uniqueIntersections.begin(), uniqueIntersections.end(),
          [&s1Index, &s2Index, &currIntersection](const Intersection& inter) {
              if (inter.s1Index == s2Index && inter.s2Index == s1Index) {
                currIntersection.survives = 1;
                return true;
              }
              else {
                currIntersection.survives = -1;
                return false;
              }
          });
      if (it != uniqueIntersections.end()) {
        // s2 has its lowest W value for this intersection, a perfect intersection
        currIntersection.survives = 1;
        return true;
      }
      else {
        // the current intersection is not in the uniqueIntersections list.
        // check further
        //return isSurvivingIntersection(intersections, uniqueIntersections, currIntersection);
        currIntersection.survives = 0;
        return false;
      }
    }
  }
  else {
    // check further
    currIntersection.survives = 0;
    return false;
  }

}

*/

//-----------------------------------------------------------------------------

/*! Draw lines for the line extension auto close method.
*/
static void drawLineExtensionAutocloses(TVectorImage* vi, TVectorRenderData& rd) {
  static TPalette* plt = 0;

  const int ROUNDINGFACTOR = 4;
  DEBUG_LOG("\n\n===================== drawLineExtensionAutocloses - begin ==================================================================\n\n");
  DEBUG_LOG("drawLineExtensionAutocloses, autoCloseFactor:" << AutocloseFactor << "\n");
  if (!plt) {
    plt = new TPalette();
    const TPixelRGBM32 lineExtensionColor(0, 0xFF, 0xFF, 0xFF / 2);
    plt->addStyle(lineExtensionColor);
  }

  std::vector<std::pair<int, double>> startPoints, endPoints;
  // calculate the points for the candidate closing lines
  // ToDo TomDoingArt - will this function call be needed?
  //getLineExtensionClosingPoints(vi->getBBox(), AutocloseFactor, vi, startPoints, endPoints);
  
  UINT strokeCount = vi->getStrokeCount();

  TVectorImage* vaux = new TVectorImage();

  std::vector <int> listOfExtensions;

  for (UINT i = 0; i < strokeCount; i++) {
    TStroke* s1 = vi->getStroke(i);
    // get start point. -----------------------------------------
    TThickPoint* startPoint = &s1->getThickPoint(0);
    const TThickQuadratic *startChunk = s1->getChunk(0);
    
    // Get the x,y from P0 and P1, calculate a startPointExtension set of coordinates from that.

    std::pair<double, double> P0;
    std::pair<double, double> P1;
    std::pair<double, double> P2;

    P0 = std::make_pair(startChunk->getThickP0().x, startChunk->getThickP0().y);
    P1 = std::make_pair(startChunk->getThickP1().x, startChunk->getThickP1().y);
    P2 = std::make_pair(startChunk->getThickP2().x, startChunk->getThickP2().y);

    std::pair<double, double> startExtensionP2;

    startExtensionP2 = extendQuadraticBezier(P0, P1, P2, AutocloseFactor, true);

    //startPoint->x;
    //startPoint->y;
    //TThickPoint* startPointPlusOne = &s1->getThickPoint(1);
    //startPointPlusOne->x;
    //startPointPlusOne->y;

    //TThickPoint* startPointExtension = new TThickPoint(startPoint->x + AutocloseFactor, startPoint->y + AutocloseFactor);
    TThickPoint* startPointExtension = new TThickPoint(startExtensionP2.first, startExtensionP2.second);

    // Get the x,y from P1 and P2, calculate an endPointExtension set of coordinates from that.

    // get end point. -------------------------------------------
    TThickPoint* endPoint = &s1->getThickPoint(s1->getControlPointCount() - 1);
    const TThickQuadratic* endChunk = s1->getChunk(s1->getChunkCount() - 1);
    
    // Get the x,y from P0 and P1, calculate an startPointExtension set of coordinates from that.

    //std::pair<double, double> P0;
    //std::pair<double, double> P1;
    //std::pair<double, double> P2;

    P0 = std::make_pair(endChunk->getThickP0().x, endChunk->getThickP0().y);
    P1 = std::make_pair(endChunk->getThickP1().x, endChunk->getThickP1().y);
    P2 = std::make_pair(endChunk->getThickP2().x, endChunk->getThickP2().y);

    std::pair<double, double> endExtensionP2;

    endExtensionP2 = extendQuadraticBezier(P0, P1, P2, AutocloseFactor, false);


    TThickPoint* endPointExtension = new TThickPoint(endExtensionP2.first, endExtensionP2.second);

    // add these points to vaux
    std::vector<TThickPoint> points(3);
    TThickPoint p1;
    
    TThickPoint p2;

    p1 = *startPoint;
    p2 = *startPointExtension;

    points[0] = p1;
    points[1] = 0.5 * (p1 + p2);
    points[2] = p2;
    points[0].thick = points[1].thick = points[2].thick = 0.0;
    TStroke* auxStroke = new TStroke(points);
    auxStroke->setStyle(2);
    //vaux->addStroke(auxStroke);
    listOfExtensions.push_back(vaux->addStroke(auxStroke));

    p1 = *endPoint;
    p2 = *endPointExtension;

    points[0] = p1;
    points[1] = 0.5 * (p1 + p2);
    points[2] = p2;
    points[0].thick = points[1].thick = points[2].thick = 0.0;
    auxStroke = new TStroke(points);
    auxStroke->setStyle(2);
    listOfExtensions.push_back(vaux->addStroke(auxStroke));
    
  } //end of adding all initial line extensions. *********************************************

  rd.m_palette = plt;

  
  
  // Calculate intersections *****************************************************************

  // Define a struct to hold the intersection data
  //struct Intersection {
  //  UINT s1Index;   
  //  bool s2IsExtension;
  //  UINT s2Index; 
  //  double w;        
  //  double x;
  //  double y;
  //};

  std::vector <Intersection> intersectionList;


//  TVectorImage::Imp myImage = vi->m_imp;

  //std::cout << "------------------------ bounding boxes for vaux strokes:" << "\n";
  DEBUG_LOG("------------------------ bounding boxes for vaux strokes:" << "\n");

  UINT vauxStrokeCount = vaux->getStrokeCount();

  for (UINT i = 0; i < vauxStrokeCount; i++) {
    TStroke* s2 = vaux->getStroke(i);

    TRectD s2BBox = s2->getCenterlineBBox();
    //std::cout << i << ", " << s2->getId() << "\n";
    DEBUG_LOG(i << ", " << s2->getId() << "\n");
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x0 << " " << s2BBox.y0 << "\n";
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x1 << " " << s2BBox.y1 << "\n";

  }

  //std::cout << "************************ bounding boxes for vi strokes:" << "\n";
  DEBUG_LOG("************************ bounding boxes for vi strokes:" << "\n");

  UINT viStrokeCount = vi->getStrokeCount();

  for (UINT i = 0; i < viStrokeCount; i++) {
    TStroke* s2 = vi->getStroke(i);

    TRectD s2BBox = s2->getCenterlineBBox();
    // std::cout << i << ", " << s2->getId() << "\n";
    DEBUG_LOG(i << ", " << s2->getId() << "\n");
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x0 << " " << s2BBox.y0 << "\n";
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x1 << " " << s2BBox.y1 << "\n";

  }

  TStroke* auxStroke;

  std::vector <int> listOfExtensionsToDelete;

  DEBUG_LOG("The extension stroke values:\n");
  bool checkUsingBBox = true;
  DEBUG_LOG("begin - Check extensions against regular lines. checkUsingBBox is:" << checkUsingBBox << "**********************************************************\n");

  for (UINT num : listOfExtensions) { // outer loop, the list of extensions
    auxStroke = vaux->getStroke(num);
    DEBUG_LOG(num << ", " << auxStroke->getId());
    //std::cout << " Control Point Count: " << auxStroke->getControlPointCount();
    //std::cout << " W at P0: " << auxStroke->getW(auxStroke->getControlPoint(0)) << " W at P1: " << auxStroke->getW(auxStroke->getControlPoint(1)) << " W at P2: " << auxStroke->getW(auxStroke->getControlPoint(2));
    DEBUG_LOG("\n");
    
    TRectD auxStrokeBBox = auxStroke->getCenterlineBBox();

    //std::cout << "my bounding box" << "\n";
    //std::cout << auxStroke->getId() << " " << auxStrokeBBox.x0 << " " << auxStrokeBBox.y0 << "\n";
    //std::cout << auxStroke->getId() << " " << auxStrokeBBox.x1 << " " << auxStrokeBBox.y1 << "\n";

    // std::cout << "bounding box for other strokes:" << "\n";

    for (UINT i = 0; i < viStrokeCount; i++) { // inner loop, go through all the other strokes to look for overlap
      TStroke* s2 = vi->getStroke(i);

      if (auxStroke->getId() == s2->getId()) continue; // if s2 is myself, ignore

        // do the strokes intersect?
        //auxStroke->isSelfLoop(); // check if this stroke is self loop
      std::vector<DoublePair> parIntersections;
      if (intersect(auxStroke, s2, parIntersections, checkUsingBBox)) {
        DEBUG_LOG("\t" << auxStroke->getId() << " intersects stroke:" << s2->getId());
        DEBUG_LOG(", number of intersectionList:" << parIntersections.size());
        
        // ToDo TomDoingArt - iterate through the intersections. Currently I am only processing the first one ******************************************************

        DEBUG_LOG(", W first:" << parIntersections.at(0).first << ", second:" << parIntersections.at(0).second);

        //std::cout << ", auxStroke x:" << auxStroke->getPointAtLength(parIntersections.at(0).first).x;
        //std::cout << ", y:" << auxStroke->getPointAtLength(parIntersections.at(0).first).y;
        //std::cout << ", s2 x:" << s2->getPointAtLength(parIntersections.at(0).second).x;
        //std::cout << ", y:" << s2->getPointAtLength(parIntersections.at(0).second).y;

        DEBUG_LOG(", auxStroke x:" << auxStroke->getPoint(parIntersections.at(0).first).x);
        DEBUG_LOG(", y:" << auxStroke->getPoint(parIntersections.at(0).first).y);
        DEBUG_LOG(", s2 x:" << s2->getPoint(parIntersections.at(0).second).x);
        DEBUG_LOG(", y:" << s2->getPoint(parIntersections.at(0).second).y);
        DEBUG_LOG("\n");


        //if (std::max(0.0, std::min(parIntersections.at(0).first, 1.0)) == 0 && (parIntersections.at(0).second == 0 || parIntersections.at(0).second == 1)) {
        if (std::round(parIntersections.at(0).first * ROUNDINGFACTOR) / ROUNDINGFACTOR == 0 && (std::round(parIntersections.at(0).second * ROUNDINGFACTOR) / ROUNDINGFACTOR == 0 || std::round(parIntersections.at(0).second * ROUNDINGFACTOR) / ROUNDINGFACTOR == 1)) {
          DEBUG_LOG("\t\tmy origin stroke, so ignore");
          DEBUG_LOG(", auxStroke x:" << auxStroke->getPoint(0).x);
          DEBUG_LOG(", y:" << auxStroke->getPoint(0).y);
          DEBUG_LOG(", s2 P0 x:" << s2->getPoint(0).x);
          DEBUG_LOG(", y:" << s2->getPoint(0).y);
          DEBUG_LOG(", s2 P2 x:" << s2->getPoint(1).x);
          DEBUG_LOG(", y:" << s2->getPoint(1).y);
          DEBUG_LOG("\n");

        }
        else {
          // Add the intersections to the intersection list

          // calculate distance between s2 origin and s2 origin
          double s1Oribinx = auxStroke->getPoint(0).x;
          double s1Originy = auxStroke->getPoint(0).y;
          double intersectionX = auxStroke->getPoint(parIntersections.at(0).first).x;
          double intersectionY = auxStroke->getPoint(parIntersections.at(0).first).y;
          double d = tdistance(TPointD(s1Oribinx, s1Originy), TPointD(intersectionX, intersectionY));

          DEBUG_LOG("    distance from s1 origin to intersection is:" << d << "\n");

          Intersection anIntersection = {
            num,
            false,
            i,
            parIntersections.at(0).first,
            auxStroke->getPoint(parIntersections.at(0).first).x,
            auxStroke->getPoint(parIntersections.at(0).first).y
          };

          intersectionList.push_back(anIntersection);

        }
        DEBUG_LOG("\n");


      } // end of: if (intersect(auxStroke, s2, parIntersections, checkUsingBBox))
      else {
        //std::cout << "checkUsingBBox is:" << checkUsingBBox << " and these strokes do not intersect\n";
        DEBUG_LOG("\t" << auxStroke->getId() << " does not intersect stroke:" << s2->getId());
        DEBUG_LOG(", auxStroke x:" << auxStroke->getPoint(0).x);
        DEBUG_LOG(", y:" << auxStroke->getPoint(0).y);
        DEBUG_LOG(", s2 P0 x:" << s2->getPoint(0).x);
        DEBUG_LOG(", y:" << s2->getPoint(0).y);
        DEBUG_LOG(", s2 P2 x:" << s2->getPoint(1).x);
        DEBUG_LOG(", y:" << s2->getPoint(1).y);
        DEBUG_LOG("\n");

      }
      //}
    } // inner loop of vi
  } // outer loop of listOfExtensions


  DEBUG_LOG("end - Check extensions against regular lines. ------------------------------------------------------------" << "\n");

  
  // print out the latest versions of vaux and vi

  //std::cout << "------------------------ bounding boxes for vaux strokes:" << "\n";
  DEBUG_LOG("------------------------ vaux strokes:" << "\n");

  vauxStrokeCount = vaux->getStrokeCount();

  for (UINT i = 0; i < vauxStrokeCount; i++) {
    TStroke* s2 = vaux->getStroke(i);

    TRectD s2BBox = s2->getCenterlineBBox();
    DEBUG_LOG(i << ", " << s2->getId() << "\n");
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x0 << " " << s2BBox.y0 << "\n";
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x1 << " " << s2BBox.y1 << "\n";

  }

  //std::cout << "************************ bounding boxes for vi strokes:" << "\n";
  DEBUG_LOG("************************ vi strokes:" << "\n");

  viStrokeCount = vi->getStrokeCount();

  for (UINT i = 0; i < viStrokeCount; i++) {
    TStroke* s2 = vi->getStroke(i);

    TRectD s2BBox = s2->getCenterlineBBox();
    DEBUG_LOG(i << ", " << s2->getId() << "\n");
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x0 << " " << s2BBox.y0 << "\n";
    //std::cout << i << ", " << s2->getId() << " " << s2BBox.x1 << " " << s2BBox.y1 << "\n";

  }

  // end of: print out the latest versions of vaux and vi
 

  // **********************************************************************************
  // Repeat the loops, this time for the extension to extension intersections         *
  // **********************************************************************************

//  bool checkUsingBBox = true;
  DEBUG_LOG("begin - Check extensions against other extensions. checkUsingBBox is : " << checkUsingBBox << "**********************************************************\n");

  for (UINT num : listOfExtensions) { // outer loop, the list of extensions
    auxStroke = vaux->getStroke(num);
    //std::cout << num << " Control Point Count: " << auxStroke->getControlPointCount();
    //std::cout << " stroke ID: " << auxStroke->getId() << ",";
    //std::cout << " W at P0: " << auxStroke->getW(auxStroke->getControlPoint(0)) << " W at P1: " << auxStroke->getW(auxStroke->getControlPoint(1)) << " W at P2: " << auxStroke->getW(auxStroke->getControlPoint(2)) << "\n";

    //TRectD auxStrokeBBox = auxStroke->getCenterlineBBox();

    //std::cout << "my bounding box" << "\n";
    //std::cout << auxStroke->getId() << " " << auxStrokeBBox.x0 << " " << auxStrokeBBox.y0 << "\n";
    //std::cout << auxStroke->getId() << " " << auxStrokeBBox.x1 << " " << auxStrokeBBox.y1 << "\n";

    for (UINT i = 0; i < vauxStrokeCount; i++){ // inner loop of lineExtensions
      TStroke* s2 = vaux->getStroke(i);
      
      //TRectD s2BBox = s2->getCenterlineBBox();
      //std::cout << s2->getId() << " " << s2BBox.x0 << " " << s2BBox.y0 << "\n";
      //std::cout << s2->getId() << " " << s2BBox.x1 << " " << s2BBox.y1 << "\n";

      if (auxStroke->getId() == s2->getId()) continue; // if s2 is myself, ignore

        // do the strokes intersect?
        //auxStroke->isSelfLoop(); // check if this stroke is self loop
        std::vector<DoublePair> parIntersections;
        if (intersect(auxStroke, s2, parIntersections, checkUsingBBox)) {
          DEBUG_LOG("\t" << auxStroke->getId() << " intersects stroke:" << s2->getId());
          DEBUG_LOG(", number of intersectionList:" << parIntersections.size()); 
          DEBUG_LOG(", W first:" << parIntersections.at(0).first << ", second:" << parIntersections.at(0).second);

          //std::cout << ", auxStroke x:" << auxStroke->getPointAtLength(parIntersections.at(0).first).x;
          //std::cout << ", y:" << auxStroke->getPointAtLength(parIntersections.at(0).first).y;
          //std::cout << ", s2 x:" << s2->getPointAtLength(parIntersections.at(0).second).x;
          //std::cout << ", y:" << s2->getPointAtLength(parIntersections.at(0).second).y;

          DEBUG_LOG(", auxStroke x:" << auxStroke->getPoint(parIntersections.at(0).first).x);
          DEBUG_LOG(", y:" << auxStroke->getPoint(parIntersections.at(0).first).y);
          DEBUG_LOG(", s2 x:" << s2->getPoint(parIntersections.at(0).second).x);
          DEBUG_LOG(", y:" << s2->getPoint(parIntersections.at(0).second).y);

          DEBUG_LOG("\n");

          // calculate distance between s2 origin and s2 origin
          double originS1x = auxStroke->getPoint(0).x;
          double originS1y = auxStroke->getPoint(0).y;
          double originS2x = s2->getPoint(0).x;
          double originS2y = s2->getPoint(0).y;
          double d = tdistance(TPointD(originS1x, originS1y), TPointD(originS2x, originS2y));

          if (d > AutocloseFactor) {

            DEBUG_LOG("Gap close distance " << d << " from s1 origin " << originS1x << "," << originS1y << " to s2 origin " << originS2x << "," << originS2y << " exceeds maximum distance " << AutocloseFactor << ".This intersection is ignored.\n");

          }else {
            Intersection anIntersection = {
                num,
                true,
                i,
                parIntersections.at(0).first,
                auxStroke->getPoint(parIntersections.at(0).first).x,
                auxStroke->getPoint(parIntersections.at(0).first).y};

            intersectionList.push_back(anIntersection);
          }
          
        } else {
          //std::cout << "checkUsingBBox is:" << checkUsingBBox << " and these strokes do not intersect\n";
        }
      //}
    } // inner loop of extensions
  } // outer loop

  DEBUG_LOG("end - Check extensions against other extensions. ---------------------------------------------------------------\n");


  /* 
  
  Loop through the intersection list.
  Remove intersections of each line extension except for the one which is closest to a W value of 0.
  
  */ 

  // Sorting by s1Index first, and w second if s1Index is equal
  std::sort(intersectionList.begin(), intersectionList.end(),
    [](const Intersection& a, const Intersection& b) {
      if (a.s1Index != b.s1Index) {
        return a.s1Index < b.s1Index; // Sort by s1Index
      }
      else {
        return a.w < b.w; // If s1Index is equal, sort by w
      }
    });
  

  // Step 2: Remove duplicates by keeping only the first occurrence of each s1Index
  std::vector<Intersection> uniqueIntersections;
  std::set<UINT> seenS1Indices;

  for (const auto& inter : intersectionList) {
    if (seenS1Indices.find(inter.s1Index) == seenS1Indices.end()) {
      // If the s1Index hasn't been seen before, add it to uniqueIntersections
      uniqueIntersections.push_back(inter);
      seenS1Indices.insert(inter.s1Index); // Mark this s1Index as seen
    }
  }

  DEBUG_LOG("Original list:" << "\n");
  for (Intersection currIntersection : intersectionList) {
    // Remove intersections of each line extension except for the one which is closest to a W value of 0.
    DEBUG_LOG("Intersection s1:" << currIntersection.s1Index << " isExt:" << currIntersection.s2IsExtension << " s2:" << currIntersection.s2Index << " s2W:" << currIntersection.w << " x:" << currIntersection.x << " y:" << currIntersection.y << " survives:" << currIntersection.survives << "\n");

  }

  DEBUG_LOG("Filtered list, lowest W value:" << "\n");
  for (const auto& currIntersection : uniqueIntersections) {
    DEBUG_LOG("Intersection s1:" << currIntersection.s1Index << " isExt:" << currIntersection.s2IsExtension << " s2:" << currIntersection.s2Index << " s2W:" << currIntersection.w << " x:" << currIntersection.x << " y:" << currIntersection.y << " survives:" << currIntersection.survives << "\n");
  }

  // ===================================================================================================================
  // Given the list of intersections, determine which are surviving intersections based on intersection precedence.

  std::vector<Intersection> survivingIntersections;

  determineSurvivingIntersections(intersectionList, uniqueIntersections);

  // ===================================================================================================================

  /*
  
  Loop through the surviving intersection list and create updated extension lines.
  
  */
  
  for (Intersection currIntersection : intersectionList) { // start of - outer loop, the list of intersections

    if(currIntersection.survives == 1){

    UINT s1Index;
    UINT s2Index;

    s1Index = currIntersection.s1Index;
    s2Index = currIntersection.s2Index;

    auxStroke = vaux->getStroke(currIntersection.s1Index);

    if (currIntersection.s2IsExtension) { // ******************* extension intersects extension **********************

      if (currIntersection.s1Index > currIntersection.s2Index) {
        DEBUG_LOG("mirror intersection, safe to ignore." << currIntersection.s1Index << " - " << currIntersection.s2Index << "\n");
      }
      else{

      TStroke* s2 = vaux->getStroke(currIntersection.s2Index);

      // Set P2 equal to the start endpoint P0 of s2. ********************

      const TThickQuadratic* s1StartChunk = auxStroke->getChunk(0);
      const TThickQuadratic* s2StartChunk = s2->getChunk(0);

      TThickPoint s1P0ThickPoint = s1StartChunk->getThickP0();
      TThickPoint s2P0ThickPoint = s2StartChunk->getThickP0();

      // add the new stroke.

      TThickPoint* startPoint = &s1P0ThickPoint;
      TThickPoint* endPoint = &s2P0ThickPoint;

      // add these points to vaux
      std::vector<TThickPoint> points(3);

      TThickPoint p1;
      TThickPoint p2;

      p1 = *startPoint;
      p2 = *endPoint;

      points[0] = p1;
      // Set P1 equal to 1/2 the distance between P0 and P2. *****************
      points[1] = 0.5 * (p1 + p2);
      points[2] = p2;
      points[0].thick = points[1].thick = points[2].thick = 0.0;
      TStroke* auxStroke = new TStroke(points);
      auxStroke->setStyle(2);
      int addedAt = vaux->addStroke(auxStroke);

      // add original strokes to the delete list.
      // do I care about order here, since the delete list is later sorted?
      if (currIntersection.s1Index > currIntersection.s2Index) {
        listOfExtensionsToDelete.push_back(currIntersection.s1Index);
        listOfExtensionsToDelete.push_back(currIntersection.s2Index);
        DEBUG_LOG("added " << currIntersection.s1Index << " to listOfExtensionsToDelete\n");
        DEBUG_LOG("added " << currIntersection.s2Index << " to listOfExtensionsToDelete\n");
      }
      else {
        listOfExtensionsToDelete.push_back(currIntersection.s2Index);
        listOfExtensionsToDelete.push_back(currIntersection.s1Index);
        DEBUG_LOG("added " << currIntersection.s2Index << " to listOfExtensionsToDelete\n");
        DEBUG_LOG("added " << currIntersection.s1Index << " to listOfExtensionsToDelete\n");
      }
      }
    }
    else { // ******************* extension intersects line **********************


      //Extension intersects Line
        //Set P2 equal to the point of intersection.
        //Set P1 equal to 1/2 the distance between P0 and P2.
        //add original line extension stroke to delete list.
      

      const TThickQuadratic* s1StartChunk = auxStroke->getChunk(0);

      TThickPoint s1P0ThickPoint = s1StartChunk->getThickP0();

      TThickPoint* startPoint = &s1P0ThickPoint;

      TThickPoint* endPoint = new TThickPoint(currIntersection.x, currIntersection.y);

      // add these points to vaux
      std::vector<TThickPoint> points(3);

      TThickPoint p0;
      TThickPoint p2;

      p0 = *startPoint;
      p2 = *endPoint;

      points[0] = p0;

      // Set P1 equal to 1/2 the distance between P0 and P2. *****************
      points[1] = 0.5 * (p0 + p2);
      points[2] = p2;
      points[0].thick = points[1].thick = points[2].thick = 0.0; // sets the thickness value to all the points to 0.0
      TStroke* auxStroke = new TStroke(points);
      auxStroke->setStyle(2);
      int addedAt = vaux->addStroke(auxStroke);

      DEBUG_LOG("added " << currIntersection.s1Index << " to listOfExtensionsToDelete\n");
      listOfExtensionsToDelete.push_back(currIntersection.s1Index);

    }
    }
  } // end of - outer loop, the list of intersections

  



  /*
  
  Loop through the delete list in descending order and remove the listed strokes from the line extensions.
  
  */

// Sort the vector first
  std::sort(listOfExtensionsToDelete.begin(), listOfExtensionsToDelete.end(), std::greater<int>());

  // Use std::unique to remove consecutive duplicates
  auto it = std::unique(listOfExtensionsToDelete.begin(), listOfExtensionsToDelete.end());

  // Erase the "extra" elements after std::unique
  listOfExtensionsToDelete.erase(it, listOfExtensionsToDelete.end());

  DEBUG_LOG("Deleting unwanted strokes, of which there are:" << listOfExtensionsToDelete.size() << "\n");
  for (UINT j = 0; j < listOfExtensionsToDelete.size(); j++) {
    DEBUG_LOG("Removing index:" << listOfExtensionsToDelete.at(j) << ", Id:" << vaux->getStroke(listOfExtensionsToDelete.at(j))->getId() << "\n");
    vaux->removeStroke(listOfExtensionsToDelete.at(j));
  }

  listOfExtensionsToDelete.clear(); // is this needed? The list will be reset during the next invocation of drawLineExtensionAutocloses()
    



  // temporarily disable fill check, to preserve the gap indicator color
  bool tCheckEnabledOriginal = rd.m_tcheckEnabled;
  rd.m_tcheckEnabled = false;
  // draw
  tglDraw(rd, vaux);
  // restore original value
  rd.m_tcheckEnabled = tCheckEnabledOriginal;
  delete vaux;
  DEBUG_LOG("===================== drawLineExtensionAutocloses - end ==================================================================\n");
}

//-----------------------------------------------------------------------------

static void buildLineExtensionAutocloseImage(
  TVectorImage* vaux, TVectorImage* vi,
  const std::vector<std::pair<int, double>>& startPoints,
  const std::vector<std::pair<int, double>>& endPoints) {
  DEBUG_LOG("buildLineExtensionAutocloseImage\n");
  for (UINT i = 0; i < startPoints.size(); i++) {
    TThickPoint p1 = vi->getStroke(startPoints[i].first)
      ->getThickPoint(startPoints[i].second);
    TThickPoint p2 =
      vi->getStroke(endPoints[i].first)->getThickPoint(endPoints[i].second);
    std::vector<TThickPoint> points(3);
    points[0] = p1;
    points[1] = 0.5 * (p1 + p2);
    points[2] = p2;
    points[0].thick = points[1].thick = points[2].thick = 0.0;
    TStroke* auxStroke = new TStroke(points);
    auxStroke->setStyle(2);
    vaux->addStroke(auxStroke);
  }
}

//-----------------------------------------------------------------------------

/*! Take image from \b Stage::Player \b data and recall the right method for
                this kind of image, for vector image recall \b onVectorImage(),
   for raster
                image recall \b onRasterImage() for toonz image recall \b
   onToonzImage().
*/
void RasterPainter::onImage(const Stage::Player &player) {
  if (m_singleColumnEnabled && !player.m_isCurrentColumn && !player.m_isMask)
    return;

  // Attempt Plastic-deformed drawing
  // For now generating icons of plastic-deformed image causes crash as
  // QOffscreenSurface is created outside the gui thread.
  // As a quick workaround, ignore the deformation if this is called from
  // non-gui thread (i.e. icon generator thread)
  // 12/1/2018 Now the scene icon is rendered without deformation either
  // since it causes unknown error with opengl contexts..
  TStageObject *obj =
      ::plasticDeformedObj(player, m_vs.m_plasticVisualSettings);
  if (obj && QThread::currentThread() == qGuiApp->thread() &&
      (!m_vs.m_forSceneIcon || m_vs.m_forReference)) {
    flushRasterImages();
    ::onPlasticDeformedImage(obj, player, m_vs, m_viewAff);
  } else {
    // Common image draw
    const TImageP &img = player.image();

    if (TVectorImageP vi = img)
      onVectorImage(vi.getPointer(), player);
    else if (TRasterImageP ri = img)
      onRasterImage(ri.getPointer(), player);
    else if (TToonzImageP ti = img)
      onToonzImage(ti.getPointer(), player);
    else if (TMeshImageP mi = img) {
      // Never draw mesh when building reference image
      if (m_vs.m_forReference) return;
      flushRasterImages();
      ::onMeshImage(mi.getPointer(), player, m_vs, m_viewAff);
    }
  }
}

//-----------------------------------------------------------------------------
/*! View a vector cell images.
\n	If onion-skin is active compute \b TOnionFader value.
                Create and boot a \b TVectorRenderData and recall \b tglDraw().
*/
void RasterPainter::onVectorImage(TVectorImage *vi,
                                  const Stage::Player &player) {
  flushRasterImages();

  // When loaded, vectorimages needs to have regions recomputed, but doing that
  // while loading them
  // is quite slow (think about loading whole scenes!..). They are recomputed
  // the first time they
  // are selected and shown on screen...except when playing back, to avoid
  // slowness!

  // (Daniele) This function should *NOT* be responsible of that.
  //           It's the *image itself* that should recalculate or initialize
  //           said data
  //           if queried about it and turns out it's not available...

  if (!player.m_isPlaying && player.m_isCurrentColumn)
    vi->recomputeRegionsIfNeeded();

  const Preferences &prefs = *Preferences::instance();

  TColorFunction *cf = 0, *guidedCf = 0;
  TPalette *vPalette = vi->getPalette();
  TPixel32 bgColor   = TPixel32::White;

  int tc        = (m_checkFlags && player.m_isCurrentColumn)
                      ? ToonzCheck::instance()->getChecks()
                      : 0;
  bool inksOnly = tc & ToonzCheck::eInksOnly;

  int oldFrame = vPalette->getFrame();
  vPalette->setFrame(player.m_frame);

  UCHAR useOpacity = player.m_opacity;
  if (player.m_isLightTableEnabled && !player.m_isCurrentColumn)
    useOpacity *= 0.30;

  if (player.m_onionSkinDistance != c_noOnionSkin) {
    TPixel32 frontOnionColor, backOnionColor;

    if (player.m_onionSkinDistance != 0 &&
        (!player.m_isShiftAndTraceEnabled ||
         Preferences::instance()->areOnionColorsUsedForShiftAndTraceGhosts())) {
      prefs.getOnionData(frontOnionColor, backOnionColor, inksOnly);
      bgColor =
          (player.m_onionSkinDistance < 0) ? backOnionColor : frontOnionColor;
    }

    double m[4] = {1.0, 1.0, 1.0, 1.0}, c[4];

    // Weighted addition to RGB and matte multiplication
    m[3] = 1.0 -
           ((player.m_onionSkinDistance == 0)
                ? 0.1
                : OnionSkinMask::getOnionSkinFade(player.m_onionSkinDistance));
    if (player.m_isLightTableEnabled && !player.m_isCurrentColumn) m[3] *= 0.30;

    c[0] = (1.0 - m[3]) * bgColor.r, c[1] = (1.0 - m[3]) * bgColor.g,
    c[2] = (1.0 - m[3]) * bgColor.b;
    c[3] = 0.0;

    cf = new TGenericColorFunction(m, c);
  } else if (player.m_filterColor != TPixel::Black) {
    TPixel32 colorScale = player.m_filterColor;
    colorScale.m        = useOpacity;
    cf                  = new TColumnColorFilterFunction(colorScale);
  } else if (useOpacity < 255)
    cf = new TTranspFader(useOpacity / 255.0);

  TVectorRenderData rd(m_viewAff * player.m_placement, TRect(), vPalette, cf,
                       true  // alpha enabled
                       );

  rd.m_drawRegions           = !inksOnly;
  rd.m_inkCheckEnabled       = tc & ToonzCheck::eInk;
  rd.m_ink1CheckEnabled      = tc & ToonzCheck::eInk1;
  rd.m_paintCheckEnabled     = tc & ToonzCheck::ePaint;
  rd.m_blackBgEnabled        = tc & ToonzCheck::eBlackBg;
  rd.m_colorCheckIndex       = ToonzCheck::instance()->getColorIndex();
  rd.m_show0ThickStrokes     = prefs.getShow0ThickLines();
  rd.m_regionAntialias       = prefs.getRegionAntialias();
  rd.m_animatedGuidedDrawing = prefs.getAnimatedGuidedDrawing();
  if (player.m_onionSkinDistance != 0 &&
      (player.m_isCurrentColumn || player.m_isCurrentXsheetLevel)) {
    if (player.m_isGuidedDrawingEnabled == 3         // show guides on all
        || (player.m_isGuidedDrawingEnabled == 1 &&  // show guides on closest
            (player.m_onionSkinDistance == player.m_firstBackOnionSkin ||
             player.m_onionSkinDistance == player.m_firstFrontOnionSkin)) ||
        (player.m_isGuidedDrawingEnabled == 2 &&  // show guides on farthest
         (player.m_onionSkinDistance == player.m_onionSkinBackSize ||
          player.m_onionSkinDistance == player.m_onionSkinFrontSize)) ||
        (player.m_isEditingLevel &&  // fix for level editing mode sending extra
                                     // players
         player.m_isGuidedDrawingEnabled == 2 &&
         player.m_onionSkinDistance == player.m_lastBackVisibleSkin)) {
      rd.m_showGuidedDrawing = player.m_isGuidedDrawingEnabled > 0;
      int currentStrokeCount = 0;
      int totalStrokes       = vi->getStrokeCount();
      TXshSimpleLevel *sl    = player.m_sl;

      if (sl) {
        TImageP image          = sl->getFrame(player.m_currentFrameId, false);
        TVectorImageP vecImage = image;
        if (vecImage) currentStrokeCount = vecImage->getStrokeCount();
        if (currentStrokeCount < 0) currentStrokeCount = 0;
        if (player.m_guidedFrontStroke != -1 &&
            (player.m_onionSkinDistance == player.m_onionSkinFrontSize ||
             player.m_onionSkinDistance == player.m_firstFrontOnionSkin))
          rd.m_indexToHighlight = player.m_guidedFrontStroke;
        else if (player.m_guidedBackStroke != -1 &&
                 (player.m_onionSkinDistance == player.m_onionSkinBackSize ||
                  player.m_onionSkinDistance == player.m_firstBackOnionSkin))
          rd.m_indexToHighlight = player.m_guidedBackStroke;
        else if (currentStrokeCount < totalStrokes)
          rd.m_indexToHighlight = currentStrokeCount;

        double guidedM[4] = {1.0, 1.0, 1.0, 1.0}, guidedC[4];
        TPixel32 bgColor  = TPixel32::Blue;
        guidedM[3] =
            1.0 -
            ((player.m_onionSkinDistance == 0)
                 ? 0.1
                 : OnionSkinMask::getOnionSkinFade(player.m_onionSkinDistance));

        guidedC[0] = (1.0 - guidedM[3]) * bgColor.r,
        guidedC[1] = (1.0 - guidedM[3]) * bgColor.g,
        guidedC[2] = (1.0 - guidedM[3]) * bgColor.b;
        guidedC[3] = 0.0;

        guidedCf      = new TGenericColorFunction(guidedM, guidedC);
        rd.m_guidedCf = guidedCf;
      }
    }
  }

  if (tc & (ToonzCheck::eTransparency | ToonzCheck::eGap)) {
    TPixel dummy;
    rd.m_tcheckEnabled = true;

    if (rd.m_blackBgEnabled)
      prefs.getTranspCheckData(rd.m_tCheckInk, dummy, rd.m_tCheckPaint);
    else
      prefs.getTranspCheckData(dummy, rd.m_tCheckInk, rd.m_tCheckPaint);
  }

  if (m_vs.m_colorMask != 0) {
    glColorMask((m_vs.m_colorMask & TRop::RChan) ? GL_TRUE : GL_FALSE,
                (m_vs.m_colorMask & TRop::GChan) ? GL_TRUE : GL_FALSE,
                (m_vs.m_colorMask & TRop::BChan) ? GL_TRUE : GL_FALSE, GL_TRUE);
  }
  TVectorImageP viDelete;
  if (tc & ToonzCheck::eGap) {
    viDelete = vi->clone();
    vi       = viDelete.getPointer();
    vi->selectFill(vi->getBBox(), 0, 1, true, true, false);
  }

  TStroke *guidedStroke = 0;
  if (m_maskLevel > 0)
    tglDrawMask(rd, vi);
  else
    tglDraw(rd, vi, &guidedStroke);

  //if (tc & ToonzCheck::eAutoclose) drawAutocloses(vi, rd); // Commented out by TomDoingArt

  if (tc & ToonzCheck::eAutoclose) drawLineExtensionAutocloses(vi, rd);

  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  vPalette->setFrame(oldFrame);

  delete cf;
  delete guidedCf;

  if (guidedStroke) m_guidedStrokes.push_back(guidedStroke);
}

//-----------------------------------------------------

/*! Create a \b Node and put it in \b m_nodes.
*/
void RasterPainter::onRasterImage(TRasterImage *ri,
                                  const Stage::Player &player) {
  TRasterP r = ri->getRaster();

  TAffine aff;
  aff = m_viewAff * player.m_placement * player.m_dpiAff;
  aff = TTranslation(m_dim.lx * 0.5, m_dim.ly * 0.5) * aff *
        TTranslation(-r->getCenterD() +
                     convert(ri->getOffset()));  // this offset is !=0 only if
                                                 // in cleanup camera test mode

  TRectD bbox = TRectD(0, 0, m_dim.lx, m_dim.ly);
  bbox *= convert(m_clipRect);
  if (bbox.isEmpty()) return;

  UCHAR useOpacity = player.m_opacity;
  if (player.m_isLightTableEnabled && !player.m_isCurrentColumn)
    useOpacity *= 0.30;

  int alpha                 = 255;
  Node::OnionMode onionMode = Node::eOnionSkinNone;
  if (player.m_onionSkinDistance != c_noOnionSkin) {
    // GetOnionSkinFade va bene per il vettoriale mentre il raster funziona al
    // contrario
    // 1 opaco -> 0 completamente trasparente
    // inverto quindi il risultato della funzione stando attento al caso 0
    // (in cui era scolpito il valore 0.9)
    double onionSkiFade = player.m_onionSkinDistance == 0
                              ? 0.9
                              : (1.0 - OnionSkinMask::getOnionSkinFade(
                                           player.m_onionSkinDistance));
    if (player.m_isLightTableEnabled && !player.m_isCurrentColumn)
      onionSkiFade *= 0.30;
    alpha               = tcrop(tround(onionSkiFade * 255.0), 0, 255);
    if (player.m_isShiftAndTraceEnabled &&
        !Preferences::instance()->areOnionColorsUsedForShiftAndTraceGhosts())
      onionMode = Node::eOnionSkinNone;
    else {
      onionMode =
          (player.m_onionSkinDistance > 0)
              ? Node::eOnionSkinFront
              : ((player.m_onionSkinDistance < 0) ? Node::eOnionSkinBack
                                                  : Node::eOnionSkinNone);
    }
  } else if (useOpacity < 255)
    alpha             = useOpacity;
  TXshSimpleLevel *sl = player.m_sl;
  bool doPremultiply  = false;
  bool whiteTransp    = false;
  if (sl) {
    LevelProperties *levelProp = sl->getProperties();
    if (levelProp->doPremultiply())
      doPremultiply = true;
    else if (levelProp->whiteTransp())
      whiteTransp = true;
  }

  bool ignoreAlpha =
      (Preferences::instance()->isIgnoreAlphaonColumn1Enabled() &&
       player.m_column == 0 &&
       isSubsheetChainOnColumn0(sl->getScene()->getTopXsheet(), player.m_xsh,
                                player.m_frame));

  m_nodes.push_back(Node(r, 0, alpha, aff, ri->getSavebox(), bbox,
                         player.m_frame, player.m_isCurrentColumn, onionMode,
                         doPremultiply, whiteTransp, ignoreAlpha,
                         player.m_filterColor));
}

//-----------------------------------------------------------------------------
/*! Create a \b Node and put it in \b m_nodes.
*/
void RasterPainter::onToonzImage(TToonzImage *ti, const Stage::Player &player) {
  TRasterCM32P r = ti->getRaster();
  if (!ti->getPalette()) return;

  TAffine aff = m_viewAff * player.m_placement * player.m_dpiAff;
  aff         = TTranslation(m_dim.lx / 2.0, m_dim.ly / 2.0) * aff *
        TTranslation(-r->getCenterD());

  TRectD bbox = TRectD(0, 0, m_dim.lx, m_dim.ly);
  bbox *= convert(m_clipRect);
  if (bbox.isEmpty()) return;

  UCHAR useOpacity = player.m_opacity;
  if (player.m_isLightTableEnabled && !player.m_isCurrentColumn)
    useOpacity *= 0.30;

  int alpha                 = 255;
  Node::OnionMode onionMode = Node::eOnionSkinNone;
  if (player.m_onionSkinDistance != c_noOnionSkin) {
    // GetOnionSkinFade is good for the vector while the raster works at the
    //    Opposite 1 opaque -> 0 completely transparent
    //    I therefore reverse the result of the function by being attentive to
    //    case 0
    //    (where the value 0.9 was carved)
    double onionSkiFade = player.m_onionSkinDistance == 0
                              ? 0.9
                              : (1.0 - OnionSkinMask::getOnionSkinFade(
                                           player.m_onionSkinDistance));
    if (player.m_isLightTableEnabled && !player.m_isCurrentColumn)
      onionSkiFade *= 0.30;
    alpha               = tcrop(tround(onionSkiFade * 255.0), 0, 255);

    if (player.m_isShiftAndTraceEnabled &&
        !Preferences::instance()->areOnionColorsUsedForShiftAndTraceGhosts())
      onionMode = Node::eOnionSkinNone;
    else {
      onionMode =
          (player.m_onionSkinDistance > 0)
              ? Node::eOnionSkinFront
              : ((player.m_onionSkinDistance < 0) ? Node::eOnionSkinBack
                                                  : Node::eOnionSkinNone);
    }

  } else if (useOpacity < 255)
    alpha = useOpacity;

  m_nodes.push_back(Node(r, ti->getPalette(), alpha, aff, ti->getSavebox(),
                         bbox, player.m_frame, player.m_isCurrentColumn,
                         onionMode, false, false, false, player.m_filterColor));
}

//**********************************************************************************************
//    OpenGLPainter  implementation
//**********************************************************************************************

OpenGlPainter::OpenGlPainter(const TAffine &viewAff, const TRect &rect,
                             const ImagePainter::VisualSettings &vs,
                             bool isViewer, bool alphaEnabled)
    : Visitor(vs)
    , m_viewAff(viewAff)
    , m_clipRect(rect)
    , m_camera3d(false)
    , m_phi(0)
    , m_maskLevel(0)
    , m_isViewer(isViewer)
    , m_alphaEnabled(alphaEnabled)
    , m_paletteHasChanged(false)
    , m_minZ(0)
    , m_singleColumnEnabled(false) {}

//-----------------------------------------------------------------------------

void OpenGlPainter::onImage(const Stage::Player &player) {
  if (m_singleColumnEnabled && !player.m_isCurrentColumn && !player.m_isMask)
    return;

  if (player.m_z < m_minZ) m_minZ = player.m_z;

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glPushMatrix();

  if (m_camera3d)
    glTranslated(
        0, 0,
        player.m_z);  // Ok, move object along z as specified in the player

  // Attempt Plastic-deformed drawing
  if (TStageObject *obj =
          ::plasticDeformedObj(player, m_vs.m_plasticVisualSettings))
    ::onPlasticDeformedImage(obj, player, m_vs, m_viewAff);
  else if (const TImageP &image = player.image()) {
    if (TVectorImageP vi = image)
      onVectorImage(vi.getPointer(), player);
    else if (TRasterImageP ri = image)
      onRasterImage(ri.getPointer(), player);
    else if (TToonzImageP ti = image)
      onToonzImage(ti.getPointer(), player);
    else if (TMeshImageP mi = image)
      onMeshImage(mi.getPointer(), player, m_vs, m_viewAff);
  }

  glPopMatrix();
  glPopAttrib();
}

//-----------------------------------------------------------------------------

void OpenGlPainter::onVectorImage(TVectorImage *vi,
                                  const Stage::Player &player) {
  if (m_camera3d && (player.m_onionSkinDistance == c_noOnionSkin ||
                     player.m_onionSkinDistance == 0)) {
    const TRectD &bbox = player.m_placement * player.m_dpiAff * vi->getBBox();
    draw3DShadow(bbox, player.m_z, m_phi);
  }

  TColorFunction *cf = 0;
  TOnionFader fader;

  TPalette *vPalette = vi->getPalette();

  int oldFrame = vPalette->getFrame();
  vPalette->setFrame(player.m_frame);  // Hehe. Should be locking
                                       // vPalette's mutex here...
  if (player.m_onionSkinDistance != c_noOnionSkin) {
    TPixel32 bgColor = TPixel32::White;
    fader            = TOnionFader(
        bgColor, OnionSkinMask::getOnionSkinFade(player.m_onionSkinDistance));
    cf = &fader;
  }

  TVectorRenderData rd =
      isViewer() ? TVectorRenderData(TVectorRenderData::ViewerSettings(),
                                     m_viewAff * player.m_placement, m_clipRect,
                                     vPalette)
                 : TVectorRenderData(TVectorRenderData::ProductionSettings(),
                                     m_viewAff * player.m_placement, m_clipRect,
                                     vPalette);

  rd.m_alphaChannel = m_alphaEnabled;
  rd.m_is3dView     = m_camera3d;

  if (m_maskLevel > 0)
    tglDrawMask(rd, vi);
  else
    tglDraw(rd, vi);

  vPalette->setFrame(oldFrame);
}

//-----------------------------------------------------------------------------

void OpenGlPainter::onRasterImage(TRasterImage *ri,
                                  const Stage::Player &player) {
  if (m_camera3d && (player.m_onionSkinDistance == c_noOnionSkin ||
                     player.m_onionSkinDistance == 0)) {
    TRectD bbox(ri->getBBox());

    // Since bbox is in image coordinates, adjust to level coordinates first
    bbox -= ri->getRaster()->getCenterD() - convert(ri->getOffset());

    // Then, to world coordinates
    bbox = player.m_placement * player.m_dpiAff * bbox;

    draw3DShadow(bbox, player.m_z, m_phi);
  }

  bool tlvFlag = player.m_sl && player.m_sl->getType() == TZP_XSHLEVEL;

  if (tlvFlag &&
      m_paletteHasChanged)  // o.o! Raster images here - should never be
    assert(false);          // dealing with tlv stuff!

  bool premultiplied = tlvFlag;  // xD
  static std::vector<char>
      matteChan;  // Wtf this is criminal. Although probably this
                  // stuff is used only in the main thread... hmmm....
  TRaster32P r = (TRaster32P)ri->getRaster();
  r->lock();

  if (c_noOnionSkin != player.m_onionSkinDistance) {
    double fade =
        0.5 - 0.45 * (1 - 1 / (1 + 0.15 * abs(player.m_onionSkinDistance)));
    if ((int)matteChan.size() < r->getLx() * r->getLy())
      matteChan.resize(r->getLx() * r->getLy());

    int k = 0;
    for (int y = 0; y < r->getLy(); ++y) {
      TPixel32 *pix    = r->pixels(y);
      TPixel32 *endPix = pix + r->getLx();

      while (pix < endPix) {
        matteChan[k++] = pix->m;
        pix->m         = (int)(pix->m * fade);
        pix++;
      }
    }

    premultiplied = false;  // pfff
  }

  TAffine aff = player.m_dpiAff;

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  tglEnableBlending(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  GLRasterPainter::drawRaster(m_viewAff * player.m_placement * aff *
                                  TTranslation(convert(ri->getOffset())),
                              ri, premultiplied);

  glPopAttrib();

  if (c_noOnionSkin != player.m_onionSkinDistance) {
    int k = 0;
    for (int y = 0; y < r->getLy(); ++y) {
      TPixel32 *pix    = r->pixels(y);
      TPixel32 *endPix = pix + r->getLx();

      while (pix < endPix) pix++->m = matteChan[k++];
    }
  }

  r->unlock();
}

//-----------------------------------------------------------------------------

void OpenGlPainter::onToonzImage(TToonzImage *ti, const Stage::Player &player) {
  if (m_camera3d && (player.m_onionSkinDistance == c_noOnionSkin ||
                     player.m_onionSkinDistance == 0)) {
    TRectD bbox(ti->getBBox());

    bbox -= ti->getRaster()->getCenterD();
    bbox = player.m_placement * player.m_dpiAff * bbox;

    draw3DShadow(bbox, player.m_z, m_phi);
  }

  TRasterCM32P ras = ti->getRaster();
  TRaster32P ras32(ras->getSize());
  ras32->fill(TPixel32(0, 0, 0, 0));

  // onionSkin
  TRop::quickPut(ras32, ras, ti->getPalette(), TAffine());
  TAffine aff = player.m_dpiAff;

  TRasterImageP ri(ras32);

  GLRasterPainter::drawRaster(m_viewAff * player.m_placement * aff, ri, true);
}

//-----------------------------------------------------------------------------

void OpenGlPainter::beginMask() {
  ++m_maskLevel;
  TStencilControl::instance()->beginMask();
}
void OpenGlPainter::endMask() {
  --m_maskLevel;
  TStencilControl::instance()->endMask();
}
void OpenGlPainter::enableMask(TStencilControl::MaskType maskType) {
  TStencilControl::instance()->enableMask(maskType);
}
void OpenGlPainter::disableMask() {
  TStencilControl::instance()->disableMask();
}

//**********************************************************************************************
//    Plastic functions  implementation
//**********************************************************************************************

namespace {

TStageObject *plasticDeformedObj(const Stage::Player &player,
                                 const PlasticVisualSettings &pvs) {
  struct locals {
    static inline bool isDeformableMeshColumn(
        TXshColumn *column, const PlasticVisualSettings &pvs) {
      return (column->getColumnType() == TXshColumn::eMeshType) &&
             (pvs.m_showOriginalColumn != column);
    }

    static inline bool isDeformableMeshLevel(TXshSimpleLevel *sl) {
      return sl && (sl->getType() == MESH_XSHLEVEL);
    }
  };  // locals

  if (pvs.m_applyPlasticDeformation && player.m_column >= 0) {
    // Check whether the player's column is a direct stage-schematic child of a
    // mesh object
    TStageObject *playerObj =
        player.m_xsh->getStageObject(TStageObjectId::ColumnId(player.m_column));
    assert(playerObj);

    const TStageObjectId &parentId = playerObj->getParent();
    if (parentId.isColumn() && playerObj->getParentHandle()[0] != 'H') {
      TXshColumn *parentCol = player.m_xsh->getColumn(parentId.getIndex());

      if (locals::isDeformableMeshColumn(parentCol, pvs) &&
          !locals::isDeformableMeshLevel(player.m_sl)) {
        const SkDP &sd = player.m_xsh->getStageObject(parentId)
                             ->getPlasticSkeletonDeformation();

        const TXshCell &parentCell =
            player.m_xsh->getCell(player.m_frame, parentId.getIndex());

        if (parentCell.getFrameId().isStopFrame()) return 0;

        TXshSimpleLevel *parentSl = parentCell.getSimpleLevel();

        if (sd && locals::isDeformableMeshLevel(parentSl)) return playerObj;
      }
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------

void onMeshImage(TMeshImage *mi, const Stage::Player &player,
                 const ImagePainter::VisualSettings &vs,
                 const TAffine &viewAff) {
  assert(mi);

  static const double soMinColor[4] = {0.0, 0.0, 0.0,
                                        0.6};  // Translucent black
  static const double soMaxColor[4] = {1.0, 1.0, 1.0,
                                        0.6};  // Translucent white
  static const double rigMinColor[4] = {0.0, 1.0, 0.0,
                                        0.6};  // Translucent green
  static const double rigMaxColor[4] = {1.0, 0.0, 0.0, 0.6};  // Translucent red

  bool doOnionSkin    = (player.m_onionSkinDistance != c_noOnionSkin);
  bool onionSkinImage = doOnionSkin && (player.m_onionSkinDistance != 0);
  bool drawMeshes =
      vs.m_plasticVisualSettings.m_drawMeshesWireframe && !onionSkinImage;
  bool drawSO = vs.m_plasticVisualSettings.m_drawSO && !onionSkinImage;
  bool drawRigidity =
      vs.m_plasticVisualSettings.m_drawRigidity && !onionSkinImage;

  // Currently skipping onion skinned meshes
  if (onionSkinImage) return;

  // Build dpi
  TPointD meshSlDpi(player.m_sl->getDpi(player.m_fid, 0));
  assert(meshSlDpi.x != 0.0 && meshSlDpi.y != 0.0);

  // Build reference change affines

  const TAffine &worldMeshToMeshAff =
      TScale(meshSlDpi.x / Stage::inch, meshSlDpi.y / Stage::inch);
  const TAffine &meshToWorldMeshAff =
      TScale(Stage::inch / meshSlDpi.x, Stage::inch / meshSlDpi.y);
  const TAffine &worldMeshToWorldAff = player.m_placement;

  const TAffine &meshToWorldAff = worldMeshToWorldAff * meshToWorldMeshAff;

  // Prepare OpenGL
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);

  // Push mesh coordinates
  glPushMatrix();
  tglMultMatrix(viewAff * meshToWorldAff);

  // Fetch deformation
  PlasticSkeletonDeformation *deformation = 0;
  double sdFrame;

  if (vs.m_plasticVisualSettings.m_applyPlasticDeformation &&
      player.m_column >= 0) {
    TXshColumn *column = player.m_xsh->getColumn(player.m_column);

    if (column != vs.m_plasticVisualSettings.m_showOriginalColumn) {
      TStageObject *playerObj = player.m_xsh->getStageObject(
          TStageObjectId::ColumnId(player.m_column));

      deformation = playerObj->getPlasticSkeletonDeformation().getPointer();
      sdFrame     = playerObj->paramsTime(player.m_frame);
    }
  }

  UCHAR useOpacity = player.m_opacity;
  if (player.m_isLightTableEnabled && !player.m_isCurrentColumn)
    useOpacity *= 0.30;

  if (deformation) {
    // Retrieve the associated plastic deformers data (this may eventually
    // update the deforms)
    const PlasticDeformerDataGroup *dataGroup =
        PlasticDeformerStorage::instance()->process(
            sdFrame, mi, deformation, deformation->skeletonId(sdFrame),
            worldMeshToMeshAff);

    // Draw faces first
    if (drawSO)
      tglDrawSO(*mi, (double *)soMinColor, (double *)soMaxColor, dataGroup,
                true);

    if (drawRigidity)
      tglDrawRigidity(*mi, (double *)rigMinColor, (double *)rigMaxColor,
                      dataGroup, true);

    // Draw edges next
    if (drawMeshes) {
      glColor4d(0.0, 1.0, 0.0, 0.7 * useOpacity / 255.0);  // Green
      tglDrawEdges(*mi, dataGroup);  // The mesh must be deformed
    }
  } else {
    // Draw un-deformed data

    // Draw faces first
    if (drawSO) tglDrawSO(*mi, (double *)soMinColor, (double *)soMaxColor);

    if (drawRigidity)
      tglDrawRigidity(*mi, (double *)rigMinColor, (double *)rigMaxColor);

    // Just draw the mesh image next
    if (drawMeshes) {
      glColor4d(0.0, 1.0, 0.0, 0.7 * useOpacity / 255.0);  // Green
      tglDrawEdges(*mi);
    }
  }

  // Cleanup OpenGL
  glPopMatrix();

  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_BLEND);
}

//-----------------------------------------------------------------------------

//! Applies Plastic deformation of the specified player's stage object.
void onPlasticDeformedImage(TStageObject *playerObj,
                            const Stage::Player &player,
                            const ImagePainter::VisualSettings &vs,
                            const TAffine &viewAff) {
  bool doOnionSkin    = (player.m_onionSkinDistance != c_noOnionSkin);
  bool onionSkinImage = doOnionSkin && (player.m_onionSkinDistance != 0);

  // Deal with color scaling due to transparency / onion skin
  double pixScale[4] = {1.0, 1.0, 1.0, 1.0};

  UCHAR useOpacity = player.m_opacity;
  if (player.m_isLightTableEnabled && !player.m_isCurrentColumn)
    useOpacity *= 0.30;

  if (doOnionSkin) {
    if (onionSkinImage) {
      TPixel32 frontOnionColor, backOnionColor;
      bool inksOnly;

      Preferences::instance()->getOnionData(frontOnionColor, backOnionColor,
                                            inksOnly);

      const TPixel32 &refColor =
          (player.m_onionSkinDistance < 0) ? backOnionColor : frontOnionColor;

      pixScale[3] =
          1.0 - OnionSkinMask::getOnionSkinFade(player.m_onionSkinDistance);
      pixScale[0] =
          (refColor.r / 255.0) * pixScale[3];  // refColor is not premultiplied
      pixScale[1] = (refColor.g / 255.0) * pixScale[3];
      pixScale[2] = (refColor.b / 255.0) * pixScale[3];
    }
  } else if (useOpacity < 255) {
    pixScale[3] = useOpacity / 255.0;
    pixScale[0] = pixScale[1] = pixScale[2] = 0.0;
  }

  // Build the Mesh-related data
  const TXshCell &cell =
      player.m_xsh->getCell(player.m_frame, playerObj->getParent().getIndex());

  TXshSimpleLevel *meshLevel = cell.getSimpleLevel();
  const TFrameId &meshFid    = cell.getFrameId();

  const TMeshImageP &mi = meshLevel->getFrame(meshFid, false);
  if (!mi) return;

  // Build deformation-related data
  TStageObject *parentObj =
      player.m_xsh->getStageObject(playerObj->getParent());
  assert(playerObj);

  const PlasticSkeletonDeformationP &deformation =
      parentObj->getPlasticSkeletonDeformation().getPointer();
  assert(deformation);

  double sdFrame = parentObj->paramsTime(player.m_frame);

  // Build dpis

  TPointD meshSlDpi(meshLevel->getDpi(meshFid, 0));
  assert(meshSlDpi.x != 0.0 && meshSlDpi.y != 0.0);

  TPointD slDpi(player.m_sl ? player.m_sl->getDpi(player.m_fid, 0) : TPointD());
  if (slDpi.x == 0.0 || slDpi.y == 0.0 ||
      player.m_sl->getType() == PLI_XSHLEVEL)
    slDpi.x = slDpi.y = Stage::inch;

  // Build reference transforms

  const TAffine &worldTextureToWorldMeshAff =
      playerObj->getLocalPlacement(player.m_frame);
  const TAffine &worldTextureToWorldAff = player.m_placement;

  if (fabs(worldTextureToWorldMeshAff.det()) < 1e-6)
    return;  // Skip near-singular mesh/texture placements

  const TAffine &worldMeshToWorldTextureAff = worldTextureToWorldMeshAff.inv();
  const TAffine &worldMeshToWorldAff =
      worldTextureToWorldAff * worldMeshToWorldTextureAff;

  const TAffine &meshToWorldMeshAff =
      TScale(Stage::inch / meshSlDpi.x, Stage::inch / meshSlDpi.y);
  const TAffine &worldMeshToMeshAff =
      TScale(meshSlDpi.x / Stage::inch, meshSlDpi.y / Stage::inch);
  const TAffine &worldTextureToTextureAff = TScale(
      slDpi.x / Stage::inch,
      slDpi.y /
          Stage::inch);  // ::getDpiScale().inv() should be used instead...

  const TAffine &meshToWorldAff   = worldMeshToWorldAff * meshToWorldMeshAff;
  const TAffine &meshToTextureAff = worldTextureToTextureAff *
                                    worldMeshToWorldTextureAff *
                                    meshToWorldMeshAff;

  // Retrieve a drawable texture from the player's simple level
  const DrawableTextureDataP &texData = player.texture();
  if (!texData) return;

  // Retrieve the associated plastic deformers data (this may eventually update
  // the deforms)
  const PlasticDeformerDataGroup *dataGroup =
      PlasticDeformerStorage::instance()->process(
          sdFrame, mi.getPointer(), deformation.getPointer(),
          deformation->skeletonId(sdFrame), worldMeshToMeshAff);
  assert(dataGroup);

  // Set up OpenGL stuff
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_LINE_SMOOTH);

  // Push mesh coordinates
  glPushMatrix();

  tglMultMatrix(viewAff * meshToWorldAff);

  glEnable(GL_TEXTURE_2D);

  // Applying modulation by the specified transparency parameter

  glColor4d(pixScale[3], pixScale[3], pixScale[3], pixScale[3]);
  tglDraw(*mi, *texData, meshToTextureAff, *dataGroup);

  glDisable(GL_TEXTURE_2D);

  if (onionSkinImage) {
    glBlendFunc(GL_ONE, GL_ONE);

    // Add Onion skin color. Observe that this way we don't consider blending
    // with the texture's
    // alpha - to obtain that, there is no simple way...

    double k = (1.0 - pixScale[3]);
    glColor4d(k * pixScale[0], k * pixScale[1], k * pixScale[2], 0.0);
    tglDrawFaces(*mi, dataGroup);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }

  glPopMatrix();

  glDisable(GL_LINE_SMOOTH);
  glDisable(GL_BLEND);
  assert(glGetError() == GL_NO_ERROR);
}

}  // namespace
