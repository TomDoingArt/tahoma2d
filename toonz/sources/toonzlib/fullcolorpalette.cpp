

#include "toonz/fullcolorpalette.h"
#include "toonz/tscenehandle.h"
#include "toonz/toonzscene.h"
#include "toonz/toonzfolders.h"
#include "tsystem.h"
#include "tstream.h"
#include "tpalette.h"

//==================================================
//
// FullColorPalette
//
//==================================================

FullColorPalette::FullColorPalette()
    : m_fullcolorPalettePath("+palettes\\Raster_Drawing_Palette.tpl")
    , m_palette(0) {}

//----------------------------------------------------

FullColorPalette *FullColorPalette::instance() {
  static FullColorPalette _instance;
  return &_instance;
}

//----------------------------------------------------

FullColorPalette::~FullColorPalette() { clear(); }

//----------------------------------------------------

void FullColorPalette::clear() {
  if (m_palette) m_palette->release();
  m_palette = 0;
}

//----------------------------------------------------

TPalette *FullColorPalette::getPalette(ToonzScene *scene) {
  if (m_palette) return m_palette;
  m_palette = new TPalette();
  m_palette->addRef();
  TFilePath fullPath = scene->decodeFilePath(m_fullcolorPalettePath);
  if (!TSystem::doesExistFileOrLevel(fullPath)) {
    // For the French who have the old name of the headstock
    // The old one will be loaded but saved with the new name!
    TFilePath app("+palettes\\fullcolorPalette.tpl");
    fullPath = scene->decodeFilePath(app);

    // If fullcolorPalette not found, look fora default raster palette is
    // defined
    if (!TSystem::doesExistFileOrLevel(fullPath))
      fullPath = ToonzFolder::getMyPalettesDir() + "raster_default.tpl";
  }
  if (TSystem::doesExistFileOrLevel(fullPath)) {
    TPalette *app = new TPalette();
    TIStream is(fullPath);
    is >> app;
    m_palette->assign(app);
    delete app;
  }
  m_palette->setPaletteName(L"Raster Drawing Palette");

  return m_palette;
}

//----------------------------------------------------

void FullColorPalette::savePalette(ToonzScene *scene) {
  if (!m_palette || !m_palette->getDirtyFlag()) return;

  TFilePath fullPath = scene->decodeFilePath(m_fullcolorPalettePath);
  if (TSystem::touchParentDir(fullPath)) {
    if (TSystem::doesExistFileOrLevel(fullPath))
      TSystem::removeFileOrLevel(fullPath);
    TOStream os(fullPath);
    os << m_palette;
    m_palette->setDirtyFlag(false);
  }
}
